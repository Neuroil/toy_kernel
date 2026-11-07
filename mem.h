//
// Created by neuroil on 2026/11/7.
//

#ifndef TOY_KERNEL_MEM_H
#define TOY_KERNEL_MEM_H

#include "page_table.h"
#include "pmm.h"

struct KernelMem {
    struct Page*        pages;
    struct Pmm*         pmm;
    struct PageTable*   page_table;
};

extern struct KernelMem kernel_mem;

#endif //TOY_KERNEL_MEM_H
