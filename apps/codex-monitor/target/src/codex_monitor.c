#define _GNU_SOURCE

#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#ifndef HOST_PREVIEW
#include <termios.h>
#endif
#include <unistd.h>

#include "ui_layout.h"
#include "font_data.h"

struct framebuffer {
    int fd;
    unsigned char *memory;
    size_t mapped_length;
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
};

struct quota_state {
    int five_hour_remaining;
    uint32_t five_hour_reset;
    int weekly_remaining;
    uint32_t weekly_reset;
    int reset_cards;
    uint32_t sync_timestamp;
    int timezone_offset_minutes;
    uint32_t sync_target_time;
    uint32_t last_receive_target;
    int valid;
};

enum monitor_status {
    MONITOR_WAIT = 0,
    MONITOR_LIVE = 1,
    MONITOR_STALE = 2
};

#ifndef HOST_PREVIEW
struct input_backend {
    int fd;
    int is_serial;
    int have_saved_termios;
    struct termios saved_termios;
};
#endif

static uint32_t pack_component(uint8_t value, const struct fb_bitfield *field)
{
    uint32_t max_value;
    if (field->length == 0) return 0;
    max_value = (1U << field->length) - 1U;
    return (((uint32_t)value * max_value + 127U) / 255U) << field->offset;
}

static uint32_t pack_rgb(const struct fb_var_screeninfo *var,
                         uint8_t red, uint8_t green, uint8_t blue)
{
    return pack_component(red, &var->red) | pack_component(green, &var->green) |
           pack_component(blue, &var->blue);
}

static int fb_open(struct framebuffer *fb)
{
    fb->fd = open("/dev/fb0", O_RDWR);
    if (fb->fd < 0) return -1;
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var) < 0 ||
        ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fix) < 0 ||
        fb->var.bits_per_pixel != 16) {
        close(fb->fd);
        return -1;
    }
    fb->mapped_length = fb->fix.smem_len;
    fb->memory = mmap(NULL, fb->mapped_length, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fb->fd, 0);
    if (fb->memory == MAP_FAILED) {
        close(fb->fd);
        return -1;
    }
    return 0;
}

static void fb_close(struct framebuffer *fb)
{
    if (fb->memory != MAP_FAILED) munmap(fb->memory, fb->mapped_length);
    if (fb->fd >= 0) close(fb->fd);
}

static void draw_pixel(struct framebuffer *fb, int x, int y, uint32_t color)
{
    uint16_t pixel;
    unsigned char *address;
    if (x < 0 || y < 0 || (uint32_t)x >= fb->var.xres || (uint32_t)y >= fb->var.yres) return;
    address = fb->memory + (size_t)y * fb->fix.line_length + (size_t)x * 2U;
    pixel = (uint16_t)color;
    memcpy(address, &pixel, sizeof(pixel));
}

static uint8_t rgb565_component(uint16_t pixel, const struct fb_bitfield *field)
{
    uint32_t max_value;
    uint32_t value;
    if (field->length == 0) return 0;
    max_value = (1U << field->length) - 1U;
    value = (pixel >> field->offset) & max_value;
    return (uint8_t)((value * 255U + max_value / 2U) / max_value);
}

static uint8_t blend_component(uint8_t background, uint8_t foreground, uint8_t alpha)
{
    uint32_t inverse = 255U - alpha;
    return (uint8_t)((background * inverse + foreground * alpha + 127U) / 255U);
}

static void draw_alpha_pixel(struct framebuffer *fb, int x, int y, uint32_t color,
                             uint8_t alpha)
{
    uint16_t destination;
    unsigned char *address;
    uint8_t red, green, blue;
    if (alpha == 0) return;
    if (alpha == 255) { draw_pixel(fb, x, y, color); return; }
    if (x < 0 || y < 0 || (uint32_t)x >= fb->var.xres || (uint32_t)y >= fb->var.yres) return;
    address = fb->memory + (size_t)y * fb->fix.line_length + (size_t)x * 2U;
    memcpy(&destination, address, sizeof(destination));
    red = blend_component(rgb565_component(destination, &fb->var.red),
                          rgb565_component((uint16_t)color, &fb->var.red), alpha);
    green = blend_component(rgb565_component(destination, &fb->var.green),
                            rgb565_component((uint16_t)color, &fb->var.green), alpha);
    blue = blend_component(rgb565_component(destination, &fb->var.blue),
                           rgb565_component((uint16_t)color, &fb->var.blue), alpha);
    draw_pixel(fb, x, y, pack_rgb(&fb->var, red, green, blue));
}

static void fill_rect(struct framebuffer *fb, int x, int y, int width, int height,
                      uint32_t color)
{
    int yy, xx;
    for (yy = y; yy < y + height; ++yy)
        for (xx = x; xx < x + width; ++xx) draw_pixel(fb, xx, yy, color);
}

static void draw_line(struct framebuffer *fb, int x0, int y0, int x1, int y1,
                      uint32_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        draw_pixel(fb, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        if (2 * error >= dy) { error += dy; x0 += sx; }
        if (2 * error <= dx) { error += dx; y0 += sy; }
    }
}

static void draw_rect(struct framebuffer *fb, int x, int y, int width, int height,
                      uint32_t color)
{
    draw_line(fb, x, y, x + width - 1, y, color);
    draw_line(fb, x, y, x, y + height - 1, color);
    draw_line(fb, x + width - 1, y, x + width - 1, y + height - 1, color);
    draw_line(fb, x, y + height - 1, x + width - 1, y + height - 1, color);
}

static const struct font_glyph *font_find(const struct font_face *font, char character)
{
    uint8_t index;
    for (index = 0; index < font->count; ++index)
        if (font->glyphs[index].code == (uint8_t)character) return &font->glyphs[index];
    return NULL;
}

static int measure_text(const struct font_face *font, const char *text, int spacing)
{
    int width = 0, count = 0;
    while (*text != '\0') {
        const struct font_glyph *glyph = font_find(font, *text++);
        if (glyph != NULL) width += glyph->advance;
        ++count;
    }
    if (count > 1) width += (count - 1) * spacing;
    return width;
}

static int glyph_pixel_visible(const struct font_glyph *glyph, int row, int column)
{
    if (glyph->format == FONT_BITMAP_ALPHA)
        return glyph->data[row * glyph->width + column] != 0;
    return (glyph->data[row * ((glyph->width + 7) / 8) + column / 8] &
            (uint8_t)(0x80U >> (column % 8))) != 0;
}

/* Bounds are measured from the actual stored pixels, not nominal font metrics. */
static int measure_text_bounds(const struct font_face *font, const char *text, int spacing,
                               int *top, int *bottom)
{
    int have_pixels = 0;
    (void)spacing;
    *top = 0;
    *bottom = 0;
    while (*text != '\0') {
        const struct font_glyph *glyph = font_find(font, *text++);
        if (glyph != NULL) {
            int row, column;
            for (row = 0; row < glyph->height; ++row) {
                for (column = 0; column < glyph->width; ++column) {
                    if (glyph_pixel_visible(glyph, row, column)) {
                        int pixel_top = row;
                        int pixel_bottom = row + 1;
                        if (!have_pixels || pixel_top < *top) *top = pixel_top;
                        if (!have_pixels || pixel_bottom > *bottom) *bottom = pixel_bottom;
                        have_pixels = 1;
                    }
                }
            }
        }
    }
    return have_pixels;
}

static int centered_text_y(const struct font_face *font, const char *text, int spacing,
                           int box_y, int box_height)
{
    int top, bottom;
    if (!measure_text_bounds(font, text, spacing, &top, &bottom)) return box_y;
    return box_y + (box_height - (bottom - top)) / 2 - top;
}

static void draw_text(struct framebuffer *fb, int x, int y, const struct font_face *font,
                      const char *text, int spacing, uint32_t color)
{
    while (*text != '\0') {
        const struct font_glyph *glyph = font_find(font, *text++);
        if (glyph != NULL) {
            int row, column;
            int stride = (glyph->width + 7) / 8;
            for (row = 0; row < glyph->height; ++row)
                for (column = 0; column < glyph->width; ++column)
                    if (glyph->format == FONT_BITMAP_ALPHA) {
                        draw_alpha_pixel(fb, x + column, y + row,
                                         color, glyph->data[row * glyph->width + column]);
                    } else if (glyph->data[row * stride + column / 8] &
                               (uint8_t)(0x80U >> (column % 8))) {
                        draw_pixel(fb, x + column, y + row, color);
                    }
            x += glyph->advance + spacing;
        }
    }
}

static void draw_progress_bar(struct framebuffer *fb, int x, int y, int width,
                              int height, int percentage, uint32_t fill_color,
                              uint32_t border_color)
{
    int fill_width;
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    fill_width = (width - 2) * percentage / 100;
    draw_rect(fb, x, y, width, height, border_color);
    fill_rect(fb, x + 1, y + 1, fill_width, height - 2, fill_color);
}

static int number_text(char *out, uint32_t value)
{
    char reverse[12];
    int count = 0, i;
    do { reverse[count++] = (char)('0' + value % 10U); value /= 10U; } while (value != 0);
    for (i = 0; i < count; ++i) out[i] = reverse[count - i - 1];
    out[count] = '\0';
    return count;
}

static void copy_text(char *out, const char *in)
{
    while ((*out++ = *in++) != '\0') { }
}

static uint32_t update_age_seconds(const struct quota_state *state, uint32_t target_now)
{
    if (!state->valid || target_now < state->last_receive_target) return 0;
    return target_now - state->last_receive_target;
}

static enum monitor_status monitor_status_for(const struct quota_state *state,
                                              uint32_t target_now)
{
    if (!state->valid) return MONITOR_WAIT;
    return update_age_seconds(state, target_now) > 120U ? MONITOR_STALE : MONITOR_LIVE;
}

static void format_updated(char *out, const struct quota_state *state, uint32_t target_now)
{
    uint32_t age = update_age_seconds(state, target_now);
    int length;
    if (!state->valid) { copy_text(out, "UPDATED ---"); return; }
    copy_text(out, "UPDATED ");
    if (age < 60U) {
        length = number_text(out + 8, age);
        out[8 + length] = 'S'; out[9 + length] = ' '; out[10 + length] = 'A';
        out[11 + length] = 'G'; out[12 + length] = 'O'; out[13 + length] = '\0';
    } else if (age < 3600U) {
        length = number_text(out + 8, age / 60U);
        out[8 + length] = 'M'; out[9 + length] = ' '; out[10 + length] = 'A';
        out[11 + length] = 'G'; out[12 + length] = 'O'; out[13 + length] = '\0';
    } else {
        length = number_text(out + 8, age / 3600U);
        out[8 + length] = 'H'; out[9 + length] = ' '; out[10 + length] = 'A';
        out[11 + length] = 'G'; out[12 + length] = 'O'; out[13 + length] = '\0';
    }
}

static void local_date_time(uint32_t timestamp, int timezone_minutes,
                            uint32_t *month, uint32_t *day,
                            uint32_t *hour, uint32_t *minute)
{
    uint32_t seconds = timestamp + (uint32_t)(timezone_minutes * 60);
    uint32_t days = seconds / 86400U;
    uint32_t day_seconds = seconds - days * 86400U;
    uint32_t z = days + 719468U;
    uint32_t era = z / 146097U;
    uint32_t doe = z - era * 146097U;
    uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    uint32_t mp = (5U * doy + 2U) / 153U;
    *day = doy - (153U * mp + 2U) / 5U + 1U;
    *month = mp + (mp < 10U ? 3U : 0U) - (mp < 10U ? 0U : 9U);
    *hour = day_seconds / 3600U;
    *minute = (day_seconds % 3600U) / 60U;
}

static void draw_header(struct framebuffer *fb, const struct quota_state *state,
                        uint32_t target_now, uint32_t black, uint32_t text)
{
    char clock_text[20];
    const char *status_text;
    uint32_t status_color;
    uint32_t month, day, hour, minute, display_time;
    uint32_t elapsed = 0;
    int status_x, clock_x;
    enum monitor_status status = monitor_status_for(state, target_now);
    fill_rect(fb, 0, 0, (int)fb->var.xres, HEADER_H, black);
    draw_text(fb, TITLE_X, TITLE_Y, &font_header, "CODEX MONITOR", TITLE_SPACING, text);
    if (status == MONITOR_WAIT) {
        status_text = "WAIT";
        status_color = pack_rgb(&fb->var, COLOR_MUTED_R, COLOR_MUTED_G, COLOR_MUTED_B);
    } else if (status == MONITOR_STALE) {
        status_text = "STALE";
        status_color = pack_rgb(&fb->var, COLOR_WARNING_R, COLOR_WARNING_G, COLOR_WARNING_B);
    } else {
        status_text = "LIVE";
        status_color = pack_rgb(&fb->var, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    }
    status_x = SCREEN_W - HEADER_RIGHT_MARGIN - measure_text(&font_label, status_text, HEADER_TEXT_SPACING);
    draw_text(fb, status_x, HEADER_CLOCK_Y, &font_label, status_text,
              HEADER_TEXT_SPACING, status_color);
    if (!state->valid) return;
    if (target_now >= state->sync_target_time) elapsed = target_now - state->sync_target_time;
    display_time = state->sync_timestamp + elapsed;
    local_date_time(display_time, state->timezone_offset_minutes, &month, &day, &hour, &minute);
    clock_text[0] = (char)('0' + month / 10U); clock_text[1] = (char)('0' + month % 10U);
    clock_text[2] = '/'; clock_text[3] = (char)('0' + day / 10U); clock_text[4] = (char)('0' + day % 10U);
    clock_text[5] = ' '; clock_text[6] = (char)('0' + hour / 10U); clock_text[7] = (char)('0' + hour % 10U);
    clock_text[8] = ':'; clock_text[9] = (char)('0' + minute / 10U); clock_text[10] = (char)('0' + minute % 10U);
    clock_text[11] = '\0';
    clock_x = status_x - HEADER_STATUS_GAP - measure_text(&font_label, clock_text, HEADER_TEXT_SPACING);
    draw_text(fb, clock_x, HEADER_CLOCK_Y, &font_label, clock_text,
              HEADER_TEXT_SPACING, text);
}

static uint32_t quota_color(const struct fb_var_screeninfo *var, int remaining,
                            uint32_t normal, uint32_t yellow, uint32_t red)
{
    if (remaining < 0) return pack_rgb(var, 120, 135, 150);
    if (remaining < 20) return red;
    if (remaining <= 50) return yellow;
    return normal;
}

static void draw_percentage(struct framebuffer *fb, int card_x, int card_width, int y,
                            int value, int scale, int spacing, uint32_t color)
{
    char text[8];
    int length, text_width;
    if (value < 0) {
        text[0] = '-'; text[1] = '-'; text[2] = '\0'; length = 2;
    } else {
        length = number_text(text, (uint32_t)value);
        text[length++] = '%'; text[length] = '\0';
    }
    (void)scale;
    text_width = measure_text(&font_large_number, text, spacing);
    draw_text(fb, card_x + (card_width - text_width) / 2, y, &font_large_number,
              text, spacing, color);
}

static void draw_reset_time(struct framebuffer *fb, int x, int y, uint32_t timestamp,
                            int timezone_minutes, uint32_t color)
{
    char result[20], number[12];
    uint32_t month, day, hour, minute;
    int length, day_length;
    if (timestamp == 0) { draw_text(fb, x, y, &font_reset_time, "--:--", 2, color); return; }
    local_date_time(timestamp, timezone_minutes, &month, &day, &hour, &minute);
    result[0] = (char)('0' + hour / 10U); result[1] = (char)('0' + hour % 10U);
    result[2] = ':'; result[3] = (char)('0' + minute / 10U); result[4] = (char)('0' + minute % 10U); result[5] = '\0';
    if (x >= RIGHT_CARD_X) {
        length = number_text(number, month); memcpy(result, number, (size_t)length);
        result[length++] = '/'; day_length = number_text(number, day);
        memcpy(result + length, number, (size_t)day_length); length += day_length;
        result[length++] = ' '; result[length++] = (char)('0' + hour / 10U); result[length++] = (char)('0' + hour % 10U);
        result[length++] = ':'; result[length++] = (char)('0' + minute / 10U); result[length++] = (char)('0' + minute % 10U); result[length] = '\0';
    }
    draw_text(fb, x, y, &font_reset_time, result, 2, color);
}

static void draw_footer(struct framebuffer *fb, const struct quota_state *state,
                        uint32_t target_now, uint32_t text, uint32_t border)
{
    char cards[12], updated[24];
    int footer_value_x, footer_text_y, footer_value_y, updated_x;
    fill_rect(fb, FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H,
              pack_rgb(&fb->var, COLOR_FOOTER_R, COLOR_FOOTER_G, COLOR_FOOTER_B));
    draw_rect(fb, FOOTER_X, FOOTER_Y, FOOTER_W, FOOTER_H, border);
    footer_text_y = centered_text_y(&font_label, "RESET CARDS:", FOOTER_TEXT_SPACING,
                                    FOOTER_Y, FOOTER_H);
    footer_value_y = centered_text_y(&font_reset_time, "3", FOOTER_TEXT_SPACING,
                                     FOOTER_Y, FOOTER_H);
    draw_text(fb, FOOTER_TEXT_X, footer_text_y, &font_label, "RESET CARDS:",
              FOOTER_TEXT_SPACING, pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
    footer_value_x = FOOTER_TEXT_X + measure_text(&font_label, "RESET CARDS:", FOOTER_TEXT_SPACING) + 8;
    if (state->reset_cards < 0) {
        footer_value_y = centered_text_y(&font_reset_time, "--", FOOTER_TEXT_SPACING,
                                         FOOTER_Y, FOOTER_H);
        draw_text(fb, footer_value_x, footer_value_y, &font_reset_time, "--",
                  FOOTER_TEXT_SPACING, text);
    } else {
        number_text(cards, (uint32_t)state->reset_cards);
        footer_value_y = centered_text_y(&font_reset_time, cards, FOOTER_TEXT_SPACING,
                                         FOOTER_Y, FOOTER_H);
        draw_text(fb, footer_value_x, footer_value_y, &font_reset_time, cards,
                  FOOTER_TEXT_SPACING, text);
    }
    format_updated(updated, state, target_now);
    updated_x = FOOTER_UPDATED_RIGHT_X - measure_text(&font_label, updated, FOOTER_TEXT_SPACING);
    draw_text(fb, updated_x, FOOTER_UPDATED_Y, &font_label, updated, FOOTER_TEXT_SPACING,
              pack_rgb(&fb->var, COLOR_MUTED_R, COLOR_MUTED_G, COLOR_MUTED_B));
}

static void draw_screen(struct framebuffer *fb, const struct quota_state *state,
                        uint32_t target_now)
{
    uint32_t black = pack_rgb(&fb->var, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B);
    uint32_t text = pack_rgb(&fb->var, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    uint32_t green = pack_rgb(&fb->var, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    uint32_t blue = pack_rgb(&fb->var, COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
    uint32_t yellow = pack_rgb(&fb->var, COLOR_WARNING_R, COLOR_WARNING_G, COLOR_WARNING_B);
    uint32_t red = pack_rgb(&fb->var, COLOR_ERROR_R, COLOR_ERROR_G, COLOR_ERROR_B);
    uint32_t border = pack_rgb(&fb->var, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B);
    uint32_t left_color, right_color;
    fill_rect(fb, 0, 0, SCREEN_W, SCREEN_H, black);
    draw_header(fb, state, target_now, black, text);
    if (!state->valid) {
        draw_text(fb, WAITING_X, WAITING_Y, &font_label, "WAITING FOR DATA...",
                  WAITING_SPACING,
                  pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
    } else {
        left_color = quota_color(&fb->var, state->five_hour_remaining, green, yellow, red);
        right_color = quota_color(&fb->var, state->weekly_remaining, blue, yellow, red);
        draw_rect(fb, LEFT_CARD_X, CARD_Y, CARD_W, CARD_H, border);
        draw_rect(fb, RIGHT_CARD_X, CARD_Y, CARD_W, CARD_H, border);
        draw_text(fb, LEFT_CARD_X + CARD_LABEL_X_INSET, CARD_LABEL_Y, &font_label, "5-HOUR QUOTA",
                  CARD_TEXT_SPACING, pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
        draw_percentage(fb, LEFT_CARD_X, CARD_W, CARD_PERCENT_Y, state->five_hour_remaining,
                        CARD_PERCENT_SCALE, CARD_PERCENT_SPACING, left_color);
        draw_progress_bar(fb, LEFT_CARD_X + CARD_BAR_X_INSET, CARD_BAR_Y, CARD_BAR_W,
                          CARD_BAR_H, state->five_hour_remaining, left_color, border);
        draw_text(fb, LEFT_CARD_X + CARD_LABEL_X_INSET, CARD_RESET_Y, &font_label, "RESET",
                  CARD_TEXT_SPACING, pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
        draw_reset_time(fb, LEFT_CARD_X + CARD_LABEL_X_INSET, CARD_RESET_VALUE_Y,
                        state->five_hour_reset, state->timezone_offset_minutes, text);
        draw_text(fb, RIGHT_CARD_X + CARD_LABEL_X_INSET, CARD_LABEL_Y, &font_label, "WEEKLY QUOTA",
                  CARD_TEXT_SPACING, pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
        draw_percentage(fb, RIGHT_CARD_X, CARD_W, CARD_PERCENT_Y, state->weekly_remaining,
                        CARD_PERCENT_SCALE, CARD_PERCENT_SPACING, right_color);
        draw_progress_bar(fb, RIGHT_CARD_X + CARD_BAR_X_INSET, CARD_BAR_Y, CARD_BAR_W,
                          CARD_BAR_H, state->weekly_remaining, right_color, border);
        draw_text(fb, RIGHT_CARD_X + CARD_LABEL_X_INSET, CARD_RESET_Y, &font_label, "RESET",
                  CARD_TEXT_SPACING, pack_rgb(&fb->var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B));
        draw_reset_time(fb, RIGHT_CARD_X + CARD_LABEL_X_INSET, CARD_RESET_VALUE_Y,
                        state->weekly_reset, state->timezone_offset_minutes, text);
    }
    draw_footer(fb, state, target_now, text, border);
}

static int parse_integer(const char *text, int length, long *value)
{
    int i = 0, negative = 0;
    long result = 0;
    if (length == 0) return 0;
    if (text[0] == '-') { negative = 1; i = 1; }
    if (i == length) return 0;
    for (; i < length; ++i) {
        if (text[i] < '0' || text[i] > '9') return 0;
        result = result * 10L + (long)(text[i] - '0');
        if (result > 2147483647L) return 0;
    }
    *value = negative ? -result : result;
    return 1;
}

static int frame_field(const char *frame, int length, const char *key, int key_length,
                       long *value, int *present)
{
    int cursor = 1;
    *present = 0;
    while (cursor < length - 1) {
        int token_start = cursor, token_end, equals, i;
        while (cursor < length - 1 && frame[cursor] != '|' && frame[cursor] != '>') ++cursor;
        token_end = cursor; equals = token_start;
        while (equals < token_end && frame[equals] != '=') ++equals;
        if (equals < token_end && equals - token_start == key_length) {
            for (i = 0; i < key_length; ++i) if (frame[token_start + i] != key[i]) break;
            if (i == key_length) { *present = 1; return parse_integer(frame + equals + 1, token_end - equals - 1, value); }
        }
        if (cursor < length - 1 && frame[cursor] == '|') ++cursor;
    }
    return 0;
}

static int parse_quota_frame(const char *frame, int length,
                             struct quota_state *state, uint32_t target_now)
{
    struct quota_state parsed = *state;
    long value;
    int present, rc_valid, first_end = 1;
    if (length < 8 || frame[0] != '<' || frame[length - 1] != '>') return 0;
    while (first_end < length - 1 && frame[first_end] != '|') ++first_end;
    if (first_end - 1 != 4 || frame[1] != 'C' || frame[2] != 'Q' || frame[3] != 'M' || frame[4] != '1') return 0;
    if (!frame_field(frame, length, "5H", 2, &value, &present) || !present || value < 0 || value > 100) return 0;
    parsed.five_hour_remaining = (int)value;
    if (!frame_field(frame, length, "5HR", 3, &value, &present) || !present || value < 0) return 0;
    parsed.five_hour_reset = (uint32_t)value;
    if (!frame_field(frame, length, "W", 1, &value, &present) || !present || value < 0 || value > 100) return 0;
    parsed.weekly_remaining = (int)value;
    if (!frame_field(frame, length, "WR", 2, &value, &present) || !present || value < 0) return 0;
    parsed.weekly_reset = (uint32_t)value;
    if (!frame_field(frame, length, "NOW", 3, &value, &present) || !present || value < 0) return 0;
    parsed.sync_timestamp = (uint32_t)value;
    if (!frame_field(frame, length, "TZ", 2, &value, &present) || !present || value < -1440 || value > 1440) return 0;
    parsed.timezone_offset_minutes = (int)value;
    rc_valid = frame_field(frame, length, "RC", 2, &value, &present);
    if (present) {
        if (!rc_valid || value < -1) return 0;
        parsed.reset_cards = (int)value;
    } else parsed.reset_cards = -1;
    parsed.sync_target_time = target_now; parsed.last_receive_target = target_now; parsed.valid = 1;
    *state = parsed;
    return 1;
}

#ifndef HOST_PREVIEW
static void input_backend_close(struct input_backend *backend);

static int serial_send_request(struct input_backend *input)
{
    static const char request[] = "<CQMREQ|V=1>\n";
    size_t sent = 0;
    while (sent < sizeof(request) - 1U) {
        ssize_t count = write(input->fd, request + sent, sizeof(request) - 1U - sent);
        if (count <= 0) return -1;
        sent += (size_t)count;
    }
    return 0;
}

static void process_input(struct framebuffer *fb, struct quota_state *state,
                          char *frame, int *length, int *overflow,
                          uint32_t target_now, struct input_backend *input,
                          int *request_outstanding)
{
    unsigned char bytes[64];
    int count = (int)read(input->fd, bytes, sizeof(bytes)), i;
    if (count <= 0) return;
    for (i = 0; i < count; ++i) {
        unsigned char byte = bytes[i];
        if (byte == '\n') {
            if (!*overflow && *length > 0) {
                if (frame[*length - 1] == '\r') --*length;
                frame[*length] = '\0';
                if ((*length == 6 && frame[0] == '<' && frame[1] == 'Q' && frame[2] == 'u' &&
                     frame[3] == 'i' && frame[4] == 't' && frame[5] == '>') ||
                    (*length == 9 && frame[0] == '<' && frame[1] == 'C' && frame[2] == 'Q' &&
                    frame[3] == 'M' && frame[4] == 'Q' && frame[5] == 'U' && frame[6] == 'I' &&
                    frame[7] == 'T' && frame[8] == '>')) {
                    fb_close(fb); input_backend_close(input); _exit(0);
                }
                if (parse_quota_frame(frame, *length, state, target_now)) {
                    *request_outstanding = 0;
                    draw_screen(fb, state, target_now);
                }
            }
            *length = 0; *overflow = 0;
        } else if (*length < 255 && !*overflow) frame[(*length)++] = (char)byte;
        else *overflow = 1;
    }
}

static int serial_backend_open(struct input_backend *backend, const char *path)
{
    struct termios settings;
    int flags;

    backend->fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (backend->fd < 0) return -1;
    backend->is_serial = 1;
    backend->have_saved_termios = 0;
    if (ioctl(backend->fd, TCGETS, &settings) < 0) goto fail;
    backend->saved_termios = settings;
    backend->have_saved_termios = 1;

    settings.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                          INLCR | IGNCR | ICRNL | IXON | IXOFF | IXANY);
    settings.c_oflag &= ~OPOST;
    settings.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    settings.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB);
#ifdef CRTSCTS
    settings.c_cflag &= ~CRTSCTS;
#endif
    settings.c_cflag |= CS8 | CLOCAL | CREAD;
    settings.c_ispeed = B115200;
    settings.c_ospeed = B115200;
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;
    if (ioctl(backend->fd, TCSETS, &settings) < 0) goto fail;

    flags = fcntl(backend->fd, F_GETFL, 0L);
    if (flags < 0 || fcntl(backend->fd, F_SETFL, flags | O_NONBLOCK) < 0) goto fail;
    return 0;

fail:
    if (backend->have_saved_termios) ioctl(backend->fd, TCSETS, &backend->saved_termios);
    close(backend->fd);
    backend->fd = -1;
    return -1;
}

static void input_backend_close(struct input_backend *backend)
{
    if (backend->fd < 0) return;
    if (backend->is_serial && backend->have_saved_termios)
        ioctl(backend->fd, TCSETS, &backend->saved_termios);
    close(backend->fd);
    backend->fd = -1;
}
#endif

static uint32_t board_time_seconds(void)
{
    time_t value = time(NULL);
    return value > 0 ? (uint32_t)value : 0;
}

static int argument_equals(const char *left, const char *right)
{
    while (*left != '\0' && *left == *right) { ++left; ++right; }
    return *left == '\0' && *right == '\0';
}

#ifndef HOST_PREVIEW
int main(int argc, char **argv)
{
    struct framebuffer fb = { .fd = -1, .memory = MAP_FAILED };
    struct input_backend input = { .fd = STDIN_FILENO, .is_serial = 0,
                                   .have_saved_termios = 0 };
    struct quota_state state = { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 };
    char frame[256];
    int length = 0, overflow = 0, last_status = -1, last_updated_age = -1;
    int request_outstanding = 0;
    uint32_t target_now = board_time_seconds(), last_header_minute = 0, last_request_target = 0;
    if (argc == 2 && argument_equals(argv[1], "--stdin")) {
        input.fd = STDIN_FILENO;
    } else if (argc == 3 && argument_equals(argv[1], "--serial")) {
        if (serial_backend_open(&input, argv[2]) < 0) { perror("serial backend"); return 2; }
    } else if (argc != 1) {
        perror("invalid monitor arguments");
        return 2;
    }
    if (fb_open(&fb) < 0) { perror("framebuffer open"); input_backend_close(&input); return 1; }
    if (target_now == 0) target_now = 1;
    if (fcntl(input.fd, F_SETFL, fcntl(input.fd, F_GETFL, 0L) | O_NONBLOCK) < 0) { perror("input flags"); fb_close(&fb); input_backend_close(&input); return 1; }
    draw_screen(&fb, &state, target_now);
    if (input.is_serial) {
        serial_send_request(&input);
        request_outstanding = 1;
        last_request_target = target_now;
    }
    for (;;) {
        uint32_t display_time = 0;
        enum monitor_status status;
        uint32_t updated_age;
        target_now = board_time_seconds();
        if (target_now == 0) target_now = state.last_receive_target + 1U;
        process_input(&fb, &state, frame, &length, &overflow, target_now, &input,
                      &request_outstanding);
        if (input.is_serial) {
            uint32_t request_interval = request_outstanding || !state.valid ? 5U : 60U;
            if (last_request_target == 0 ||
                (target_now >= last_request_target && target_now - last_request_target >= request_interval)) {
                serial_send_request(&input);
                request_outstanding = 1;
                last_request_target = target_now;
            }
        }
        status = monitor_status_for(&state, target_now);
        updated_age = update_age_seconds(&state, target_now);
        if (state.valid && target_now >= state.sync_target_time)
            display_time = state.sync_timestamp + target_now - state.sync_target_time;
        if (status != (enum monitor_status)last_status ||
            (state.valid && (int)updated_age != last_updated_age) ||
            (!state.valid && last_updated_age != -1) ||
            (state.valid && display_time / 60U != last_header_minute)) {
            uint32_t black = pack_rgb(&fb.var, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B),
                     text = pack_rgb(&fb.var, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B),
                     border = pack_rgb(&fb.var, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B);
            draw_header(&fb, &state, target_now, black, text);
            draw_footer(&fb, &state, target_now, text, border);
            last_status = (int)status;
            last_updated_age = state.valid ? (int)updated_age : -1;
            last_header_minute = display_time / 60U;
        }
        { struct timespec pause = { 1, 0 }; nanosleep(&pause, NULL); }
    }
}
#endif
