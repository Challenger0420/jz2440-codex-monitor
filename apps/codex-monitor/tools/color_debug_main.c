#include <stdint.h>
#include <stdio.h>

#define HOST_PREVIEW 1
#include "../target/src/codex_monitor.c"

static uint8_t unpack_component(uint16_t pixel, const struct fb_bitfield *field)
{
    uint32_t max_value = (1U << field->length) - 1U;
    return (uint8_t)((((pixel >> field->offset) & max_value) * 255U + max_value / 2U) / max_value);
}

int main(void)
{
    struct fb_var_screeninfo var = {0};
    uint32_t green, blue;
    var.red.offset = 11; var.red.length = 5;
    var.green.offset = 5; var.green.length = 6;
    var.blue.offset = 0; var.blue.length = 5;
    green = pack_rgb(&var, COLOR_GREEN_R, COLOR_GREEN_G, COLOR_GREEN_B);
    blue = pack_rgb(&var, COLOR_BLUE_R, COLOR_BLUE_G, COLOR_BLUE_B);
    printf("green packed=0x%04x rgb=%u,%u,%u\n", (unsigned)green,
           unpack_component((uint16_t)green, &var.red),
           unpack_component((uint16_t)green, &var.green),
           unpack_component((uint16_t)green, &var.blue));
    printf("blue packed=0x%04x rgb=%u,%u,%u\n", (unsigned)blue,
           unpack_component((uint16_t)blue, &var.red),
           unpack_component((uint16_t)blue, &var.green),
           unpack_component((uint16_t)blue, &var.blue));
    {
        struct framebuffer fb = {0};
        struct quota_state state = {92, 1789039695U, 84, 1789448757U, 3,
                                    1789037940U, 480, 1789037940U, 1789037940U, 1};
        fb.var = var; fb.var.xres = SCREEN_W; fb.var.yres = SCREEN_H;
        fb.fix.line_length = SCREEN_W * 2U;
        fb.memory = (unsigned char *)calloc((size_t)SCREEN_W * SCREEN_H, 2U);
        draw_screen(&fb, &state, state.sync_target_time);
        {
            uint16_t pixel;
            memcpy(&pixel, fb.memory + (125U * SCREEN_W + 80U) * 2U, sizeof(pixel));
            printf("screen left raw=0x%04x rgb=%u,%u,%u\n", (unsigned)pixel,
                   unpack_component(pixel, &var.red), unpack_component(pixel, &var.green),
                   unpack_component(pixel, &var.blue));
        }
        free(fb.memory);
    }
    return 0;
}
