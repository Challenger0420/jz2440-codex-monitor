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
    struct framebuffer fb;
    struct quota_state state = { 92, 1789039695U, 84, 1789448757U, 3,
                                 1789037940U, 480, 1789037940U, 1789037940U, 1 };
    size_t pixels = (size_t)SCREEN_W * SCREEN_H;
    size_t i;
#ifdef _WIN32
    /* Keep PPM header LF-only; otherwise text-mode CRLF shifts Pillow's payload offset. */
    _setmode(1, _O_BINARY);
#endif
    fb.var.xres = SCREEN_W; fb.var.yres = SCREEN_H; fb.var.bits_per_pixel = 16;
    fb.var.red.offset = 11; fb.var.red.length = 5;
    fb.var.green.offset = 5; fb.var.green.length = 6;
    fb.var.blue.offset = 0; fb.var.blue.length = 5;
    fb.fix.line_length = SCREEN_W * 2U; fb.fix.smem_len = (uint32_t)(pixels * 2U);
    fb.memory = (unsigned char *)calloc(pixels, 2U);
    if (fb.memory == NULL) return 1;
    draw_screen(&fb, &state, state.sync_target_time);
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
