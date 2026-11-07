//
// Created by neuroil on 2026/11/7.
//

#ifndef TOY_KERNEL_TEST_H
#define TOY_KERNEL_TEST_H
#include "page_table.h"
#include "uart.h"

static inline void test_print_pte(const uint32_t* pt, uint32_t vpn1, int32_t vpn0) {
    uint32_t pte = get_pte(pt, vpn1, vpn0);
    uart_put_string("pte: ");
    print_uint64(pte);
    uart_put_char('\n');
    uart_put_string("pte - binary: ");
    print_binary(pte);
    uart_put_char('\n');
}

#endif //TOY_KERNEL_TEST_H
