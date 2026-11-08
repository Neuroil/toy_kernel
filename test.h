//
// Created by neuroil on 2026/11/7.
//

#ifndef TOY_KERNEL_TEST_H
#define TOY_KERNEL_TEST_H
#include "page_table.h"
#include "uart.h"

#define GET_PTE_V(f)        (((f) & (1 << 0)) >> 0)
#define GET_PTE_R(f)        (((f) & (1 << 1)) >> 1)
#define GET_PTE_W(f)        (((f) & (1 << 2)) >> 2)
#define GET_PTE_X(f)        (((f) & (1 << 3)) >> 3)
#define GET_PTE_G(f)        (((f) & (1 << 5)) >> 5)
#define GET_PTE_A(f)        (((f) & (1 << 6)) >> 6)
#define GET_PTE_D(f)        (((f) & (1 << 7)) >> 7)

#define GET_PTE_FLAGS(p)    ((p) & 0x3FF)

#define PPN_1(pa)           ((pa) >> 22)
#define PPN(pa)             ((pa) >> 12)

#define VPN_1(va)           ((va) >> 22)
#define VPN_0(va)           (((va) >> 12) & 0x3FF)

static inline void test_print_pte(const struct PageTable *pt, uint32_t vpn1, int32_t vpn0) {
    uint32_t pte = get_pte(pt, vpn1, vpn0);
    uart_put_string("pte: ");
    print_uint64(pte);
    uart_put_char('\n');
    uart_put_string("pte - binary: ");
    print_binary(pte);
    uart_put_char('\n');
}

#endif //TOY_KERNEL_TEST_H
