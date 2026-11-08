//
// Created by neuroil on 2026/10/31.
//

#include "lac_symbols.h"
#include "page_table.h"

#include <stdint.h>
#include <sys/types.h>


uint32_t early_root_pt[1024] __attribute__((aligned(4096), section(".early_pt")));

// uint32_t root_pt[1024] __attribute__((aligned(4096)));


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
#define VPN_0(va)           (((va) >> 12) & 0x3FF)

// 早期未建立根页表，需要手算偏移量
static inline volatile uint32_t* __pa(volatile uint32_t *va) {
    return (volatile uint32_t*)((uint32_t)va + (uint32_t)_kernel_offset);
}

static inline volatile uint32_t* __va(volatile uint32_t *pa) {
    return (volatile uint32_t*)((uint32_t)pa - (uint32_t)_kernel_offset);
}

static inline void update_satp(const volatile uint32_t *pa_root) {
    uint32_t ppn  = (uint32_t)pa_root >> 12; // PPN
    uint32_t satp = (1UL << 31) | ppn;       // MODE=1 | PPN
    asm volatile("csrw satp, %0" :: "r"(satp) : "memory");
    asm volatile("sfence.vma" ::: "memory");
}

// ====== DETAILS ======
// ====== make ======

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


// ====== init ======

static inline void init_pt_as_0(volatile uint32_t *pt_addr) {
    for (int i = 0; i < 1024; ++i) pt_addr[i] = 0;
}


// ====== build ======

// ret >= 0 -> success
// ret <  0 -> failure
static inline int8_t build_l2_pt(struct PageTable *pt, uint32_t l2_pa, uint32_t va) {
    if (GET_PTE_V(
        GET_PTE_FLAGS(pt->root_pt[VPN_1(va)])
    ) != 0)
        return -1;

    pt->root_pt[VPN_1(va)] = make_ptr_pte(l2_pa);
    return 0;
}


// ====== free ======

// 目前就是最简单的移除
static inline void remove_rt_pte(struct PageTable *pt, uint32_t va) {
    pt->root_pt[VPN_1(va)] = 0;
}

static inline void remove_l2_pte(const struct PageTable *pt, uint32_t va) {
    uint32_t *l2_pt  = (uint32_t*)((pt->root_pt[VPN_1(va)] & 0xFFFFFC00) << 2);
    l2_pt[VPN_0(va)] = 0;
}


// ====== IMPLEMENT ======

void pt_build_early_page_table(void) {
    volatile uint32_t *pa_early_root_pt = __pa(early_root_pt);
    init_pt_as_0(pa_early_root_pt);

    uint32_t rwx = PTE_R | PTE_W | PTE_X | PTE_G;
    uint32_t pkb = (uint32_t)_phy_kernel_base;

    // 恒等映射 0x80000000 - 0x807FFFFF
    pa_early_root_pt[VPN_1(pkb)]          = make_mega_page_pte(pkb, rwx);
    pa_early_root_pt[VPN_1(pkb+0x400000)] = make_mega_page_pte(pkb + 0x400000, rwx);

    // 高映射 0xC0000000 -> 0x80000000
    pa_early_root_pt[VPN_1(0xC0000000)] = make_mega_page_pte(pkb, rwx);
    pa_early_root_pt[VPN_1(0xC0400000)] = make_mega_page_pte(pkb + 0x400000, rwx);

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

int8_t build_page_pte(const struct PageTable *pt, uint32_t pa, uint32_t va, uint32_t flags) {
    volatile uint32_t *l2_pt = __va((uint32_t*)((pt->root_pt[VPN_1(va)] & 0xFFFFFC00) << 2));

    if (GET_PTE_V(GET_PTE_FLAGS(l2_pt[VPN_0(va)])) != 0) return -1;

    l2_pt[VPN_0(va)] = make_l2_page_pte(pa, flags);
    return 0;
}

// 重新映射并移除非必需部分
static void reset_early_pt(struct PageTable *pt) {
    uint32_t pkb = (uint32_t)_phy_kernel_base;

    // 移除内核恒等映射
    remove_rt_pte(pt, pkb);
    remove_rt_pte(pt, pkb + 0x400000);

    // 重新建立内核区域精细映射
    uintptr_t l2_pa = pmm_alloc_page();
    assert(l2_pa != 0);

    // l2_pa换算成高地址别名才能访问
    init_pt_as_0(__va((uint32_t*)l2_pa));

    build_l2_pt(pt, l2_pa, 0xDEADBEEF);

    uint32_t rwx = PTE_R | PTE_W | PTE_X;
    build_page_pte(pt, 0x86000000, 0xDEADBEEF, rwx);
}

// 释放无用内存
static void init_mem_mapping(void) {}

void pt_init_top_pgt(struct PageTable *pt) {
    reset_early_pt(pt);
    init_mem_mapping();
    // 迁移新页表
    // update_satp(pa(root_pt));
}
