/* Entry point for freestanding OABI programs. */

        .syntax unified
        .arm
        .section .text
        .global _start
        .type _start, %function

_start:
        ldr     r0, [sp]        @ argc from the Linux initial stack
        add     r1, sp, #4      @ argv points immediately after argc
        bl      main
        swi     0x900001              @ OABI __NR_exit
1:
        b       1b
