//
// Created by neuroil on 2026/10/26.
//

#include <stddef.h>
#include <stdint.h>


#include "pmm.h"
#include "lac_symbols.h"

// 全局唯一的一份 PMM 状态（头文件里只有 extern 声明）
Page pages[NPAGES];
uint32_t bitmap[BITMAP_WORDS];
size_t free_pages;

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
            free_pages--;
        }
    }
}

void pmm_init(void) {
    for (size_t i = 0; i < BITMAP_WORDS; i++) bitmap[i] = 0;
    free_pages = NPAGES;

    // 扣掉内核镜像
    mark_used((uintptr_t)_vir_kernel_base, (uintptr_t)_kernel_end);

    // 其他
}

uintptr_t pmm_alloc_page() {
    if (free_pages == 0) return 0;

    for (size_t i = 0; i < BITMAP_WORDS; i++) {
        if (bitmap[i] == 0xFFFFFFFFu) continue;

        for (int b = 0; b < 32; b++) {
            if (!(bitmap[i] & (1u << b))) {
                bitmap[i] |= (1u << b);
                free_pages--;
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

    ++free_pages;
}