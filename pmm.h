//
// Created by neuroil on 2026/10/26.
//

#ifndef TOY_KERNEL_PMM_H
#define TOY_KERNEL_PMM_H

#include <stddef.h>
#include <stdint.h>


struct Pmm {
    struct Page *pages;
    uint32_t *   bitmap;
    size_t       free_pages;
};

extern struct Pmm pmm;

void      pmm_init();
uintptr_t pmm_alloc_page();
void      pmm_free_page(uintptr_t pa);


#endif //TOY_KERNEL_PMM_H
