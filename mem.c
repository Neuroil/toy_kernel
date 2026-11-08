//
// Created by neuroil on 2026/11/7.
//

#include "mem.h"

struct PageTable early_pt = {early_root_pt, &pmm};

struct KernelMem kernel_mem = {
    .pmm = &pmm,
    .page_table = &early_pt
};
