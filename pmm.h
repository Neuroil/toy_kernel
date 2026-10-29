//
// Created by neuroil on 2026/10/26.
//

#ifndef TOY_KERNEL_PMM_H
#define TOY_KERNEL_PMM_H

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "lac_symbols.h"
#include "page.h"

// 按2的幂对齐
#define ALIGN_DOWN(x, a)  ((x) & ~((uintptr_t)(a) - 1))
#define ALIGN_UP(x, a)    (((x) + (uintptr_t)(a) - 1) & ~((uintptr_t)(a) - 1))

static inline uintptr_t __pa(uintptr_t va) { return va + (uintptr_t)_kernel_offset; }
static inline uintptr_t __va(uintptr_t pa) { return pa - (uintptr_t)_kernel_offset; }

// 管理每一页的元数据
extern Page pages[NPAGES];
// 位图管理是否被占用
extern uint32_t bitmap[BITMAP_WORDS];
// 余量
extern size_t free_pages;

static inline int  bit_get(size_t i)      { return (bitmap[i >> 5] >> (i & 31)) & 1u; }
static inline void bit_set(size_t i)      { bitmap[i >> 5] |=  (1u << (i & 31)); }
static inline void bit_clear(size_t i)    { bitmap[i >> 5] &= ~(1u << (i & 31)); }

// 页号 = 地址 / 页大小
// 地址 = 基址 + 页号 * 页大小
static inline size_t pa_to_idx(uintptr_t pa) { return (pa - PHYS_BASE) >> PAGE_SHIFT; }
static inline uintptr_t idx_to_pa(size_t i)  { return PHYS_BASE + ((uintptr_t)i << PAGE_SHIFT); }

void pmm_init();

uintptr_t pmm_alloc_page();
void pmm_free_page(uintptr_t pa);





#endif //TOY_KERNEL_PMM_H
