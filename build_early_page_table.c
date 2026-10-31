//
// Created by neuroil on 2026/10/31.
//

#include "lac_symbols.h"

#include <stdint.h>

#define PTE_V   (1 << 0)
#define PTE_R   (1 << 1)
#define PTE_W   (1 << 2)
#define PTE_X   (1 << 3)
#define PTE_G   (1 << 5)
#define PTE_A   (1 << 6)
#define PTE_D   (1 << 7)


static uint32_t make_mega_page_pte(uint32_t pa, uint32_t flags) {
    // 物理地址高 12 位
    uint32_t ppn1 = pa >> 22;
    return (ppn1 << 20) | flags | PTE_V | PTE_A | PTE_D;
}

void build_early_page_table(void) {
    volatile uint32_t *root =
     (volatile uint32_t *)((uint32_t)_early_root_pt + (uint32_t)_kernel_offset);
    for (int i = 0; i < 1024; i++) root[i] = 0;

    uint32_t rwx = PTE_R | PTE_W | PTE_X | PTE_G;

    // 恒等映射 0x80000000 - 0x807FFFFF
    root[0x80000000 >> 22] = make_mega_page_pte(0x80000000, rwx);
    root[0x80400000 >> 22] = make_mega_page_pte(0x80400000, rwx);

    // 高映射 0xC0000000 → 0x80000000
    root[0xC0000000 >> 22] = make_mega_page_pte(0x80000000, rwx);
    root[0xC0400000 >> 22] = make_mega_page_pte(0x80400000, rwx);

    // 低 4MB 恒等映射：QEMU virt 的 test 设备在 0x00100000，用于关机
    root[0x00100000 >> 22] = make_mega_page_pte(0x00000000, PTE_R | PTE_W | PTE_G);

    // MMIO 恒等映射（每个 4MB 区域一个条目）
    // UART
    root[0x10000000 >> 22] = make_mega_page_pte(0x10000000, rwx);
    // CLINT
    root[0x02000000 >> 22] = make_mega_page_pte(0x02000000, rwx);
    // PLIC
    root[0x0C000000 >> 22] = make_mega_page_pte(0x0C000000, rwx);
}