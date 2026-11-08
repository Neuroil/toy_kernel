//
// Created by neuroil on 2026/10/26.
//

#include <stddef.h>
#include <stdint.h>


#include "pmm.h"
#include "page.h"
#include "config.h"
#include "lac_symbols.h"

struct Pmm pmm;

struct Page pages[NPAGES];
uint32_t    bitmap[BITMAP_WORDS];
size_t      free_pages;



// 按2的幂对齐
#define ALIGN_DOWN(x, a)  ((x) & ~((uintptr_t)(a) - 1))
#define ALIGN_UP(x, a)    (((x) + (uintptr_t)(a) - 1) & ~((uintptr_t)(a) - 1))

static inline uintptr_t __pa(uintptr_t va) { return va + (uintptr_t)_kernel_offset; }
static inline uintptr_t __va(uintptr_t pa) { return pa - (uintptr_t)_kernel_offset; }


static inline int  bit_get(size_t i) { return (pmm.bitmap[i >> 5] >> (i & 31)) & 1u; }
static inline void bit_set(size_t i) { pmm.bitmap[i >> 5] |= (1u << (i & 31)); }
static inline void bit_clear(size_t i) { pmm.bitmap[i >> 5] &= ~(1u << (i & 31)); }

// 页号 = 地址 / 页大小
// 地址 = 基址 + 页号 * 页大小
static inline size_t    pa_to_idx(uintptr_t pa) { return (pa - PHYS_BASE) >> PAGE_SHIFT; }
static inline uintptr_t idx_to_pa(size_t i) { return PHYS_BASE + ((uintptr_t)i << PAGE_SHIFT); }


// 遮蔽物理空间
static void mark_used(uintptr_t start, uintptr_t end) {
    // 按页大小对齐
    start = __pa(ALIGN_DOWN(start, PAGE_SIZE));
    end   = __pa(ALIGN_UP(end, PAGE_SIZE));

    // 取较大的范围
    if (start < PHYS_BASE) start = PHYS_BASE;
    if (end > PHYS_END) end = PHYS_END;

    for (uintptr_t p = start; p < end; p += PAGE_SIZE) {
        size_t i = pa_to_idx(p);
        if (!bit_get(i)) {
            bit_set(i);
            pmm.free_pages--;
        }
    }
}

void pmm_init() {
    // 初始化成员
    pmm.bitmap = bitmap;
    pmm.pages = pages;

    for (size_t i = 0; i < BITMAP_WORDS; i++) pmm.bitmap[i] = 0;
    pmm.free_pages = NPAGES;

    // 扣掉内核镜像
    mark_used((uintptr_t)_vir_kernel_base, (uintptr_t)_kernel_end);

    // 其他
}

uintptr_t pmm_alloc_page() {
    if (pmm.free_pages == 0) return 0;

    for (size_t i = 0; i < BITMAP_WORDS; i++) {
        if (pmm.bitmap[i] == 0xFFFFFFFFu) continue;

        for (int b = 0; b < 32; b++) {
            if (!(pmm.bitmap[i] & (1u << b))) {
                pmm.bitmap[i] |= (1u << b);
                pmm.free_pages--;
                return idx_to_pa((i << 5) + (size_t)b);
            }
        }
    }
    return 0;
}

void pmm_free_page(uintptr_t pa) {
    if (pa < PHYS_BASE || pa >= PHYS_END) return;
    if (pa & (PAGE_SIZE - 1)) return;

    size_t i = pa_to_idx(pa);

    if (!bit_get(i)) return;
    bit_clear(i);

    ++pmm.free_pages;
}