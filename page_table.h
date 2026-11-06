//
// Created by neuroil on 2026/11/6.
//

#ifndef TOY_KERNEL_PAGE_TABLE_H
#define TOY_KERNEL_PAGE_TABLE_H

#include "lac_symbols.h"
#include <stdint.h>
#include <assert.h>

#define PTE_V       (1 << 0)
#define PTE_R       (1 << 1)
#define PTE_W       (1 << 2)
#define PTE_X       (1 << 3)
#define PTE_G       (1 << 5)
#define PTE_A       (1 << 6)
#define PTE_D       (1 << 7)

#define PPN_1(pa)   ((pa) >> 22)
#define PPN(pa)     ((pa) >> 12)

#define VPN_1(va)   ((va) >> 22)
#define VPN_0(va)   (((va) >> 12) & 0x3FF000)

extern uint32_t early_root_pt[1024];
// extern uint32_t root_pt[1024];

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

static inline volatile uint32_t* pa(volatile uint32_t* va) {
    return (volatile uint32_t*)((uint32_t)va + (uint32_t)_kernel_offset);
}

static inline void update_satp(const volatile uint32_t* pa_root) {
    uint32_t ppn  = (uint32_t)pa_root >> 12; // PPN
    uint32_t satp = (1UL << 31) | ppn;       // MODE=1 | PPN
    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    asm volatile("sfence.vma" ::: "memory");
}

void build_early_page_table(void);
void init_top_pgt(void);

inline void build_l2_page_pte(uint32_t l2_pa, uint32_t pa, uint32_t va, uint32_t flags) {
    /*
     *  if (l2_pte not exist) {
     *      root[VPN_1(va)] = make_ptr_pte(l2_pa);
     *      init_l2_pt(va);
     *  }
     *
     *  root[VPN_0(va)] = make_l2_page_pte(pa, flags);
     */
};



#endif //TOY_KERNEL_PAGE_TABLE_H
