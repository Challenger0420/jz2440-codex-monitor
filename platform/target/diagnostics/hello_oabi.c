typedef unsigned long size_t;

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

int main(void)
{
    static const char message[] = "HELLO JZ2440\n";
    return sys_write(1, message, sizeof(message) - 1) < 0;
}
