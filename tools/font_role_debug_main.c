#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#define HOST_PREVIEW 1
#include "../src/codex_monitor.c"

static uint8_t unpack_output_component(uint16_t pixel, const struct fb_bitfield *field)
{
    uint32_t max_value;
    uint32_t value;
    if (field->length == 0) return 0;
    max_value = (1U << field->length) - 1U;
    value = (pixel >> field->offset) & max_value;
    return (uint8_t)((value * 255U + max_value / 2U) / max_value);
}

int main(void)
{
    struct framebuffer fb;
    size_t pixels = (size_t)SCREEN_W * SCREEN_H;
    size_t i;
    uint32_t black, text, label, green, blue;
#ifdef _WIN32
    _setmode(1, _O_BINARY);
#endif
    fb.var.xres = SCREEN_W; fb.var.yres = SCREEN_H; fb.var.bits_per_pixel = 16;
    fb.var.red.offset = 11; fb.var.red.length = 5;
    fb.var.green.offset = 5; fb.var.green.length = 6;
    fb.var.blue.offset = 0; fb.var.blue.length = 5;
    fb.fix.line_length = SCREEN_W * 2U; fb.fix.smem_len = (uint32_t)(pixels * 2U);
    fb.memory = (unsigned char *)calloc(pixels, 2U);
    if (fb.memory == NULL) return 1;

    black = pack_rgb(&fb.var, 0, 0, 0);
    text = pack_rgb(&fb.var, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B);
    label = pack_rgb(&fb.var, COLOR_LABEL_R, COLOR_LABEL_G, COLOR_LABEL_B);
    green = pack_rgb(&fb.var, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    blue = pack_rgb(&fb.var, COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
    fill_rect(&fb, 0, 0, SCREEN_W, SCREEN_H, black);

    draw_text(&fb, 8, 3, &font_label, "HEADER", 1, label);
    draw_text(&fb, 8, 17, &font_header, "CODEX MONITOR", 2, text);

    draw_text(&fb, 8, 48, &font_label, "LABEL", 1, label);
    draw_text(&fb, 8, 62, &font_label, "5-HOUR QUOTA", 2, text);
    draw_text(&fb, 8, 77, &font_label, "WEEKLY QUOTA", 2, text);
    draw_text(&fb, 8, 92, &font_label, "RESET", 2, text);
    draw_text(&fb, 8, 107, &font_label, "RESET CARDS", 2, text);

    draw_text(&fb, 170, 48, &font_label, "LARGE NUMBER", 1, label);
    draw_text(&fb, 170, 62, &font_large_number, "0123456789%", 2, text);
    draw_text(&fb, 170, 98, &font_large_number, "92%", 3, green);
    draw_text(&fb, 260, 98, &font_large_number, "84%", 3, blue);

    draw_text(&fb, 8, 143, &font_label, "RESET TIME", 1, label);
    draw_text(&fb, 8, 157, &font_reset_time, "19:28", 2, text);
    draw_text(&fb, 8, 184, &font_reset_time, "9/15 13:05", 2, text);

    printf("P6\n%d %d\n255\n", SCREEN_W, SCREEN_H);
    for (i = 0; i < pixels; ++i) {
        uint16_t pixel;
        unsigned char *address = fb.memory + i * 2U;
        uint8_t rgb[3];
        memcpy(&pixel, address, sizeof(pixel));
        rgb[0] = unpack_output_component(pixel, &fb.var.red);
        rgb[1] = unpack_output_component(pixel, &fb.var.green);
        rgb[2] = unpack_output_component(pixel, &fb.var.blue);
        fwrite(rgb, 1, sizeof(rgb), stdout);
    }
    free(fb.memory);
    return 0;
}
