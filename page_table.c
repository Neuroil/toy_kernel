//
// Created by neuroil on 2026/10/31.
//

#include "lac_symbols.h"
#include "page_table.h"

#include <stdint.h>




uint32_t early_root_pt[1024] __attribute__((aligned(4096), section(".early_pt")));

// uint32_t root_pt[1024] __attribute__((aligned(4096)));

void build_early_page_table(void) {
    volatile uint32_t* pa_early_root_pt = pa(early_root_pt);
    for (int i = 0; i < 1024; ++i) pa_early_root_pt[i] = 0;

    uint32_t rwx = PTE_R | PTE_W | PTE_X | PTE_G;

    // 恒等映射 0x80000000 - 0x807FFFFF
    pa_early_root_pt[VPN_1(0x80000000)] = make_mega_page_pte(0x80000000, rwx);
    pa_early_root_pt[VPN_1(0x80400000)] = make_mega_page_pte(0x80400000, rwx);

    // 高映射 0xC0000000 -> 0x80000000
    pa_early_root_pt[VPN_1(0xC0000000)] = make_mega_page_pte(0x80000000, rwx);
    pa_early_root_pt[VPN_1(0xC0400000)] = make_mega_page_pte(0x80400000, rwx);

    // 低 4MB 恒等映射
    // QEMU virt 的 test 设备在 0x00100000，用于关机
    pa_early_root_pt[VPN_1(0x00100000)] = make_mega_page_pte(0x00000000, PTE_R | PTE_W | PTE_G);

    // MMIO 恒等映射
    // UART
    pa_early_root_pt[VPN_1(0x10000000)] = make_mega_page_pte(0x10000000, rwx);
    // CLINT
    pa_early_root_pt[VPN_1(0x02000000)] = make_mega_page_pte(0x02000000, rwx);
    // PLIC
    pa_early_root_pt[VPN_1(0x0C000000)] = make_mega_page_pte(0x0C000000, rwx);

    update_satp(pa_early_root_pt);
}

static void reset_early_pt(void){}
static void init_mem_mapping(void){}

void init_top_pgt(void) {
    // 重新映射并移除非必需部分
    reset_early_pt();
    // 释放无用内存
    init_mem_mapping();
    // 迁移新页表
    // update_satp(pa(root_pt));
}
