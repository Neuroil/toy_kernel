//
// Created by neuroil on 2026/10/31.
//

#include "lac_symbols.h"

#include <stdint.h>

#include "pmm.h"

#define PTE_V   (1 << 0)
#define PTE_R   (1 << 1)
#define PTE_W   (1 << 2)
#define PTE_X   (1 << 3)
#define PTE_G   (1 << 5)
#define PTE_A   (1 << 6)
#define PTE_D   (1 << 7)

static uint32_t early_root_pt[1024] __attribute__((aligned(4096), section(".early_pt")));

static uint32_t make_mega_page_pte(uint32_t pa, uint32_t flags) {
    uint32_t ppn1 = pa >> 22;
    return (ppn1 << 20) | flags | PTE_V | PTE_A | PTE_D;
}

static void update_satp(const volatile uint32_t* pa_root) {
    uint32_t ppn  = (uint32_t)pa_root >> 12; // PPN
    uint32_t satp = (1UL << 31) | ppn;       // MODE=1 | PPN
    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    asm volatile("sfence.vma" ::: "memory");
}

void build_early_page_table(void) {
    volatile uint32_t* pa_early_root_pt =
            (volatile uint32_t*)((uint32_t)early_root_pt + (uint32_t)_kernel_offset);
    for (int i = 0; i < 1024; ++i) pa_early_root_pt[i] = 0;

    uint32_t rwx = PTE_R | PTE_W | PTE_X | PTE_G;

    // 恒等映射 0x80000000 - 0x807FFFFF
    pa_early_root_pt[0x80000000 >> 22] = make_mega_page_pte(0x80000000, rwx);
    pa_early_root_pt[0x80400000 >> 22] = make_mega_page_pte(0x80400000, rwx);

    // 高映射 0xC0000000 -> 0x80000000
    pa_early_root_pt[0xC0000000 >> 22] = make_mega_page_pte(0x80000000, rwx);
    pa_early_root_pt[0xC0400000 >> 22] = make_mega_page_pte(0x80400000, rwx);

    // 低 4MB 恒等映射
    // QEMU virt 的 test 设备在 0x00100000，用于关机
    pa_early_root_pt[0x00100000 >> 22] = make_mega_page_pte(0x00000000, PTE_R | PTE_W | PTE_G);

    // MMIO 恒等映射
    // UART
    pa_early_root_pt[0x10000000 >> 22] = make_mega_page_pte(0x10000000, rwx);
    // CLINT
    pa_early_root_pt[0x02000000 >> 22] = make_mega_page_pte(0x02000000, rwx);
    // PLIC
    pa_early_root_pt[0x0C000000 >> 22] = make_mega_page_pte(0x0C000000, rwx);

    update_satp(pa_early_root_pt);
}
