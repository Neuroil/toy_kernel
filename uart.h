//
// uart.h —— UART0 输出，kernel.c 与 syscalls.c 共用
//
#pragma once

#ifndef TOY_KERNEL_UART_H
#define TOY_KERNEL_UART_H

#include <stdint.h>

#include "config.h"

static inline void uart_put_char(unsigned char c) {
    auto volatile thr = (volatile unsigned char *)UART_THR;
    auto volatile lsr = (volatile unsigned char *)UART_LSR;

    while ((*lsr & UART_LSR_THRE) == 0) {

    }
    *thr = c;
}

static inline void uart_put_string(const char *s) {
    while (*s) {
        if (*s == '\n') uart_put_char('\r');
        uart_put_char(*(s++));
    }
}

static inline void print_uint64(uint64_t val) {
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

static inline void print_binary(uintptr_t val) {
    int bit_cnt = sizeof(val) * 8;
    for (int i = bit_cnt - 1; i >= 0; --i) {
        uart_put_char(((val >> i) & 1) ? '1' : '0');
    }
    // uart_put_char('\n');
}

#endif /* TOY_KERNEL_UART_H */
