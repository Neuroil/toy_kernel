//
// Created by neuroil on 2026/11/6.
//

#ifndef TOY_KERNEL_PAGE_TABLE_H
#define TOY_KERNEL_PAGE_TABLE_H

#include "lac_symbols.h"
#include "pmm.h"

#include <stdint.h>
#include <assert.h>


extern uint32_t early_root_pt[1024];
// extern uint32_t root_pt[1024];

// ====== DEFINITION ======

struct PageTable {
    uint32_t*   root_pt;
    struct Pmm* pmm;
};

void pt_init_top_pgt(struct PageTable* pt);

// ret >= 0 -> success
// ret <  0 -> failure
int8_t build_page_pte(const struct PageTable *pt, uint32_t pa, uint32_t va, uint32_t flags);

void pt_build_early_page_table(void);

// vpn0 = -1 即只查询根页表
// 一旦填写vpn0即意味着调用者保证二级页表地址有效
static inline uint32_t get_pte(const struct PageTable *pt, uint32_t vpn1, int32_t vpn0) {
    assert(vpn1 <= 1024);
    uint32_t pte = pt->root_pt[vpn1];
    if (vpn0 < 0) return pte;

    // 舍弃高2位
    // PTE里存的是物理地址，MMU开启后需换算成高地址别名
    uint32_t l2_pa = (pte & 0xFFFFFC00) << 2;
    // 现在为线性+偏移，暂时手动换算
    const uint32_t* l2_pt = (uint32_t*)(l2_pa - (uint32_t)_kernel_offset);
    return l2_pt[vpn0];
}

#endif //TOY_KERNEL_PAGE_TABLE_H
