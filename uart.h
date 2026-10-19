//
// uart.h —— UART0 输出，kernel.c 与 syscalls.c 共用
//
#pragma once

#ifndef TOY_KERNEL_UART_H
#define TOY_KERNEL_UART_H

#define UART0_BASE 0x10000000UL
#define UART_THR   (UART0_BASE + 0x00)
#define UART_LSR   (UART0_BASE + 0x05)
#define UART_LSR_THRE 0x20

#include <stdint.h>

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

inline void print_uint64(uint64_t val) {
    // uint64_t 最大 20 位十进制数 + \0
    char buf[21];
    int i = 20;
    buf[i] = '\0';

    if (val == 0) {
        uart_put_char('0');
        return;
    }

    while (val > 0) {
        buf[--i] = '0' + (val % 10);
        val /= 10;
    }

    while (buf[i] != '\0') {
        uart_put_char(buf[i++]);
    }
}

#endif /* TOY_KERNEL_UART_H */
