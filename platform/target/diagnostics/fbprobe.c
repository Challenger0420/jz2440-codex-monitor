#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

static void print_field(const char *name, const struct fb_bitfield *field)
{
    printf("%s.offset=%u\n", name, field->offset);
    printf("%s.length=%u\n", name, field->length);
    printf("%s.msb_right=%u\n", name, field->msb_right);
}

int main(void)
{
    int fd = open("/dev/fb0", O_RDONLY);
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;

    if (fd < 0) {
        perror("open /dev/fb0");
        return 1;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
        perror("FBIOGET_VSCREENINFO");
        close(fd);
        return 1;
    }
    if (ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("FBIOGET_FSCREENINFO");
        close(fd);
        return 1;
    }

    printf("xres=%u\n", var.xres);
    printf("yres=%u\n", var.yres);
    printf("xres_virtual=%u\n", var.xres_virtual);
    printf("yres_virtual=%u\n", var.yres_virtual);
    printf("bits_per_pixel=%u\n", var.bits_per_pixel);
    printf("line_length=%u\n", fix.line_length);
    printf("smem_len=%u\n", fix.smem_len);
    printf("visual=%u\n", fix.visual);
    print_field("red", &var.red);
    print_field("green", &var.green);
    print_field("blue", &var.blue);
    print_field("transp", &var.transp);

    close(fd);
    return 0;
}
