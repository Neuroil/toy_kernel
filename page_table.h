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


// ====== TOOLS ======

#define PTE_V               (1 << 0)
#define PTE_R               (1 << 1)
#define PTE_W               (1 << 2)
#define PTE_X               (1 << 3)
#define PTE_G               (1 << 5)
#define PTE_A               (1 << 6)
#define PTE_D               (1 << 7)

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
#define VPN_0(va)           (((va) >> 12) & 0x3FF000)

// 早期未建立根页表，需要手算偏移量
static inline volatile uint32_t* pa(volatile uint32_t* va) {
    return (volatile uint32_t*)((uint32_t)va + (uint32_t)_kernel_offset);
}

static inline void update_satp(const volatile uint32_t* pa_root) {
    uint32_t ppn  = (uint32_t)pa_root >> 12; // PPN
    uint32_t satp = (1UL << 31) | ppn;       // MODE=1 | PPN
    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    asm volatile("sfence.vma" ::: "memory");
}

// vpn0 = -1 即只查询根页表
// 一旦填写vpn0即意味着调用者保证二级页表地址有效
static inline uint32_t get_pte(const uint32_t* pt, uint32_t vpn1, int32_t vpn0) {
    assert(vpn1 <= 1024);
    uint32_t pte = pt[vpn1];
    if (vpn0 < 0) return pte;

    // 舍弃高2位
    const uint32_t* l2_pt = (uint32_t*)((pte & 0xFFFFFC00) << 2);
    return l2_pt[vpn0];
}

// ====== MAKE ======

static inline uint32_t make_mega_page_pte(uint32_t pa, uint32_t flags) {
    assert((pa & ((1u << 22) - 1)) == 0);
    return (PPN_1(pa) << 20) | flags | PTE_V | PTE_A | PTE_D;
}

static inline uint32_t make_ptr_pte(uint32_t pa) {
    assert((pa & ((1u << 12) - 1)) == 0);
    return (PPN(pa) << 10) | PTE_V;
}

static inline uint32_t make_l2_page_pte(uint32_t pa, uint32_t flags) {
    assert((pa & ((1u << 12) - 1)) == 0);
    return (PPN(pa) << 10) | flags | PTE_V | PTE_A | PTE_D;
}

// ====== INIT ======

static inline void init_pt_as_0(volatile uint32_t* pa_pt) {
    for (int i = 0; i < 1024; ++i) pa_pt[i] = 0;
}

void pt_init_top_pgt(void);

// ====== BUILD ======

// ret >= 0 -> success
// ret <  0 -> failure
static inline int8_t build_l2_pt(uint32_t* root_pt, uint32_t pa, uint32_t va) {
    if (GET_PTE_V(
        GET_PTE_FLAGS(root_pt[VPN_1(va)])
    ) != 0)
        return -1;

    root_pt[VPN_1(va)] = make_ptr_pte(pa);
    return 0;
}

// ret >= 0 -> success
// ret <  0 -> failure
int8_t build_page_pte(const uint32_t* root_pt, uint32_t pa, uint32_t va, uint32_t flags);


void pt_build_early_page_table(void);

// ====== FREE ======

// 目前就是最简单的移除
static inline void remove_ptr_pte(uint32_t* root_pt, uint32_t va) {
    root_pt[VPN_1(va)] = 0;
}

static inline void remove_l2_pte(const uint32_t* root_pt, uint32_t va) {
    uint32_t* l2_pt = (uint32_t*)((root_pt[VPN_1(va)] & 0xFFFFFC00) << 2) ;
    l2_pt[VPN_0(va)] = 0;
}

#endif //TOY_KERNEL_PAGE_TABLE_H
