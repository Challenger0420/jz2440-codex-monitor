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
    uint32_t value;
    uint32_t max_value;
    if (field->length == 0) return 0;
    max_value = (1U << field->length) - 1U;
    value = (pixel >> field->offset) & max_value;
    return (uint8_t)((value * 255U + max_value / 2U) / max_value);
}

int main(void)
{
    static const int sample_values[] = {0, 8, 72, 84, 92, 99, 100};
    struct framebuffer fb;
    size_t pixels = (size_t)SCREEN_W * SCREEN_H;
    size_t i;
    int row;
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

    fill_rect(&fb, 0, 0, SCREEN_W, SCREEN_H, pack_rgb(&fb.var, 0, 0, 0));
    draw_text(&fb, 8, 4, &font_large_number, "0123456789%", 2,
              pack_rgb(&fb.var, 205, 220, 235));
    for (row = 0; row < 7; ++row)
        draw_percentage(&fb, 0, 200, 42 + row * 32, sample_values[row],
                        CARD_PERCENT_SCALE, CARD_PERCENT_SPACING,
                        pack_rgb(&fb.var, 205, 220, 235));

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
