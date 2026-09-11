#include <unistd.h>

int main(void)
{
    static const char message[] = "HELLO JZ2440\n";
    return (write(STDOUT_FILENO, message, sizeof(message) - 1) < 0) ? 1 : 0;
}
