//
// Created by neuroil on 2026/10/17.
//

#ifndef TOY_KERNEL_POWER_OFF_H
#define TOY_KERNEL_POWER_OFF_H

#include <stdint.h>

#define TEST_BASE 0x100000UL

typedef volatile uint32_t mmio32_reg_t ;

static inline void mmio_write32(mmio32_reg_t* addr, uint32_t val) {
    *addr = val;
}

#define MMIO32(addr) ((mmio32_reg_t *)(uintptr_t)(addr))

static inline void machine_power_off() {
    mmio_write32(MMIO32(TEST_BASE), 0x5555);
    for (;;) __asm__ volatile("wfi");
}

static inline void machine_restart() {
    mmio_write32(MMIO32(TEST_BASE), 0x7777);
    for (;;) __asm__ volatile("wfi");
}

static inline void machine_exit_with_code(int code) {
    mmio_write32(MMIO32(TEST_BASE), (uint32_t)(code << 16) | 0x3333);
    for (;;) __asm__ volatile("wfi");
}

#endif //TOY_KERNEL_POWER_OFF_H
