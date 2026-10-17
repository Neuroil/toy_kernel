//
// uart.h —— UART0 输出，kernel.c 与 syscalls.c 共用
//
#pragma once

#include <stdio.h>

#define UART0_BASE 0x10000000UL
#define UART_THR   (UART0_BASE + 0x00)
#define UART_LSR   (UART0_BASE + 0x05)
#define UART_LSR_THRE 0x20

#ifndef TOY_KERNEL_UART_H
#define TOY_KERNEL_UART_H

inline void uart_put_char(unsigned char c) {
    auto volatile thr = (volatile unsigned char *)UART_THR;
    auto volatile lsr = (volatile unsigned char *)UART_LSR;

    while ((*lsr & UART_LSR_THRE) == 0) {

    }
    *thr = c;
}

inline void uart_put_string(const char *s) {
    while (*s) {
        if (*s == '\n') uart_put_char('\r');
        uart_put_char(*(s++));
    }
}

inline void my_print(const char *s) {
    uart_put_string(s);
}

#endif /* TOY_KERNEL_UART_H */
