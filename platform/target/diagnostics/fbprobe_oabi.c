#include <linux/fb.h>

typedef unsigned int uint32_t;
typedef unsigned long size_t;

static long sys_open(const char *path, int flags, int mode)
{
    register const char *r0 __asm__("r0") = path;
    register int r1 __asm__("r1") = flags;
    register int r2 __asm__("r2") = mode;

    __asm__ volatile("swi 0x900005"
                     : "+r"(r0)
                     : "r"(r1), "r"(r2)
                     : "memory");
    return (long)r0;
}

static long sys_close(int fd)
{
    register long r0 __asm__("r0") = fd;
    __asm__ volatile("swi 0x900006" : "+r"(r0) : : "memory");
    return r0;
}

static long sys_write(int fd, const void *buffer, size_t length)
{
    register long r0 __asm__("r0") = fd;
    register const void *r1 __asm__("r1") = buffer;
    register size_t r2 __asm__("r2") = length;

    __asm__ volatile("swi 0x900004"
                     : "+r"(r0)
                     : "r"(r1), "r"(r2)
                     : "memory");
    return r0;
}

static long sys_ioctl(int fd, unsigned long request, void *argument)
{
    register long r0 __asm__("r0") = fd;
    register unsigned long r1 __asm__("r1") = request;
    register void *r2 __asm__("r2") = argument;

    __asm__ volatile("swi 0x900036"
                     : "+r"(r0)
                     : "r"(r1), "r"(r2)
                     : "memory");
    return r0;
}

static size_t string_length(const char *text)
{
    size_t length = 0;
    while (text[length] != '\0')
        ++length;
    return length;
}

static void print_string(const char *text)
{
    sys_write(1, text, string_length(text));
}

static void print_u32(uint32_t value)
{
    char digits[11];
    size_t count = 0;
    uint32_t quotient;

    if (value == 0) {
        print_string("0");
        return;
    }
    while (value != 0) {
        quotient = 0;
        while (value >= 10) {
            value -= 10;
            ++quotient;
        }
        digits[count++] = (char)('0' + value);
        value = quotient;
    }
    while (count != 0) {
        char digit = digits[--count];
        sys_write(1, &digit, 1);
    }
}

static void print_field(const char *name, const struct fb_bitfield *field)
{
    print_string(name);
    print_string(".offset=");
    print_u32(field->offset);
    print_string("\n");
    print_string(name);
    print_string(".length=");
    print_u32(field->length);
    print_string("\n");
}

int main(void)
{
    int fd;
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;

    fd = (int)sys_open("/dev/fb0", 0, 0);
    if (fd < 0) {
        print_string("open /dev/fb0 failed\n");
        return 1;
    }
    if (sys_ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
        print_string("FBIOGET_VSCREENINFO failed\n");
        sys_close(fd);
        return 1;
    }
    if (sys_ioctl(fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        print_string("FBIOGET_FSCREENINFO failed\n");
        sys_close(fd);
        return 1;
    }

    print_string("xres="); print_u32(var.xres); print_string("\n");
    print_string("yres="); print_u32(var.yres); print_string("\n");
    print_string("xres_virtual="); print_u32(var.xres_virtual); print_string("\n");
    print_string("yres_virtual="); print_u32(var.yres_virtual); print_string("\n");
    print_string("bits_per_pixel="); print_u32(var.bits_per_pixel); print_string("\n");
    print_string("line_length="); print_u32(fix.line_length); print_string("\n");
    print_string("smem_len="); print_u32(fix.smem_len); print_string("\n");
    print_string("visual="); print_u32(fix.visual); print_string("\n");
    print_field("red", &var.red);
    print_field("green", &var.green);
    print_field("blue", &var.blue);
    print_field("transp", &var.transp);
    sys_close(fd);
    return 0;
}
