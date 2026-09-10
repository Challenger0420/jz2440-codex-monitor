/* Minimal ARM OABI probe: write(1, message, 13), then exit(0). */

        .syntax unified
        .arm
        .section .text
        .global _start
        .type _start, %function

_start:
        mov     r0, #1
        ldr     r1, message_address
        mov     r2, #13
        swi     0x900004              @ OABI __NR_write
        mov     r0, #0
        swi     0x900001              @ OABI __NR_exit

        .align  2
message_address:
        .word   message

        .section .rodata
message:
        .ascii  "HELLO JZ2440\n"
