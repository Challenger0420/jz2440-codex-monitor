#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

struct timespec;

FILE *stderr = (FILE *)0;

static size_t string_length(const char *text)
{
    size_t length = 0;
    while (text[length] != '\0')
        ++length;
    return length;
}

#define OABI_SYSCALL3(number, a0, a1, a2) \
    ({ register long r0 __asm__("r0") = (long)(a0); \
       register long r1 __asm__("r1") = (long)(a1); \
       register long r2 __asm__("r2") = (long)(a2); \
       __asm__ volatile("swi " #number : "+r"(r0) : "r"(r1), "r"(r2) : "memory"); \
       r0; })

int open(const char *path, int flags, ...)
{
    (void)flags;
    return (int)OABI_SYSCALL3(0x900005, path, flags, 0);
}

int close(int fd)
{
    return (int)OABI_SYSCALL3(0x900006, fd, 0, 0);
}

void _exit(int status)
{
    OABI_SYSCALL3(0x900001, status, 0, 0);
    for (;;) { }
}

int fcntl(int fd, int command, ...)
{
    va_list arguments;
    long argument;

    va_start(arguments, command);
    argument = va_arg(arguments, long);
    va_end(arguments);
    return (int)OABI_SYSCALL3(0x900037, fd, command, argument);
}

ssize_t read(int fd, void *buffer, size_t length)
{
    return (ssize_t)OABI_SYSCALL3(0x900003, fd, buffer, length);
}

int ioctl(int fd, unsigned long request, ...)
{
    va_list arguments;
    void *argument;

    va_start(arguments, request);
    argument = va_arg(arguments, void *);
    va_end(arguments);
    return (int)OABI_SYSCALL3(0x900036, fd, request, argument);
}

void *mmap(void *address, size_t length, int prot, int flags, int fd, off_t offset)
{
    register long r0 __asm__("r0") = (long)address;
    register long r1 __asm__("r1") = (long)length;
    register long r2 __asm__("r2") = prot;
    register long r3 __asm__("r3") = flags;
    register long r4 __asm__("r4") = fd;
    register long r5 __asm__("r5") = (long)(offset >> 12);

    __asm__ volatile("swi 0x9000c0"
                     : "+r"(r0)
                     : "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
                     : "memory");
    return (void *)r0;
}

int munmap(void *address, size_t length)
{
    return (int)OABI_SYSCALL3(0x90005b, address, length, 0);
}

time_t time(time_t *result)
{
    long value = OABI_SYSCALL3(0x90000d, result, 0, 0);
    if (result != 0 && value >= 0)
        *result = (time_t)value;
    return (time_t)value;
}

int nanosleep(const struct timespec *request, struct timespec *remaining)
{
    return (int)OABI_SYSCALL3(0x9000a2, request, remaining, 0);
}

void *memcpy(void *destination, const void *source, size_t length)
{
    unsigned char *to = (unsigned char *)destination;
    const unsigned char *from = (const unsigned char *)source;
    size_t index;

    for (index = 0; index < length; ++index)
        to[index] = from[index];
    return destination;
}

void *memset(void *destination, int value, size_t length)
{
    unsigned char *bytes = (unsigned char *)destination;
    size_t index;

    for (index = 0; index < length; ++index)
        bytes[index] = (unsigned char)value;
    return destination;
}

void *__memcpy_chk(void *destination, const void *source, size_t length, size_t destination_size)
{
    (void)destination_size;
    return memcpy(destination, source, length);
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

int getchar(void)
{
    unsigned char character;
    return read(STDIN_FILENO, &character, 1) == 1 ? character : -1;
}

void perror(const char *message)
{
    static const char suffix[] = "\n";
    OABI_SYSCALL3(0x900004, STDERR_FILENO, message, string_length(message));
    OABI_SYSCALL3(0x900004, STDERR_FILENO, suffix, sizeof(suffix) - 1);
}

int fprintf(FILE *stream, const char *format, ...)
{
    (void)stream;
    (void)format;
    return -1;
}

int __fprintf_chk(FILE *stream, int flag, const char *format, ...)
{
    (void)stream;
    (void)flag;
    (void)format;
    return -1;
}

static unsigned long divide_unsigned(unsigned long dividend,
                                     unsigned long divisor,
                                     unsigned long *remainder)
{
    unsigned long quotient = 0;
    unsigned int bit;

    if (divisor == 0) {
        if (remainder != 0)
            *remainder = 0;
        return 0;
    }
    for (bit = 31; bit != (unsigned int)-1; --bit) {
        /* Check before shifting; this prevents 32-bit overflow in divisor << bit. */
        if ((dividend >> bit) >= divisor) {
            dividend -= divisor << bit;
            quotient |= 1UL << bit;
        }
    }
    if (remainder != 0)
        *remainder = dividend;
    return quotient;
}

unsigned long __aeabi_uidiv(unsigned long dividend, unsigned long divisor)
{
    unsigned long remainder;
    return divide_unsigned(dividend, divisor, &remainder);
}

unsigned long __udivsi3(unsigned long dividend, unsigned long divisor)
{
    return __aeabi_uidiv(dividend, divisor);
}

unsigned long __umodsi3(unsigned long dividend, unsigned long divisor)
{
    unsigned long remainder;
    divide_unsigned(dividend, divisor, &remainder);
    return remainder;
}

long __divsi3(long dividend, long divisor)
{
    int negative = (dividend < 0) != (divisor < 0);
    unsigned long left = dividend < 0 ? (unsigned long)-dividend : (unsigned long)dividend;
    unsigned long right = divisor < 0 ? (unsigned long)-divisor : (unsigned long)divisor;
    unsigned long quotient = divide_unsigned(left, right, 0);
    return negative ? -(long)quotient : (long)quotient;
}

long __modsi3(long dividend, long divisor)
{
    int negative = dividend < 0;
    unsigned long left = dividend < 0 ? (unsigned long)-dividend : (unsigned long)dividend;
    unsigned long right = divisor < 0 ? (unsigned long)-divisor : (unsigned long)divisor;
    unsigned long remainder;
    divide_unsigned(left, right, &remainder);
    return negative ? -(long)remainder : (long)remainder;
}

unsigned long long __aeabi_uidivmod(unsigned long dividend, unsigned long divisor)
{
    unsigned long remainder;
    unsigned long quotient = divide_unsigned(dividend, divisor, &remainder);
    return ((unsigned long long)remainder << 32) | quotient;
}
