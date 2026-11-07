//
// Created by neuroil on 2026/10/31.
//

#include "lac_symbols.h"
#include "page_table.h"

#include <stdint.h>




uint32_t early_root_pt[1024] __attribute__((aligned(4096), section(".early_pt")));

// uint32_t root_pt[1024] __attribute__((aligned(4096)));

void pt_build_early_page_table(void) {
    volatile uint32_t* pa_early_root_pt = pa(early_root_pt);
    init_pt_as_0(pa_early_root_pt);

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

int8_t build_page_pte(const uint32_t* root_pt, uint32_t pa, uint32_t va, uint32_t flags) {
    uint32_t* l2_pt = (uint32_t*)((root_pt[VPN_1(va)] & 0xFFFFFC00) << 2) ;

    if (GET_PTE_V(GET_PTE_FLAGS(l2_pt[VPN_0(va)])) != 0) return -1;

    l2_pt[VPN_0(va)] = make_l2_page_pte(pa, flags);
    return 0;
}

// 重新映射并移除非必需部分
static void reset_early_pt(void) {

}

// 释放无用内存
static void init_mem_mapping(void){}

void pt_init_top_pgt(void) {
    reset_early_pt();
    init_mem_mapping();
    // 迁移新页表
    // update_satp(pa(root_pt));
}
