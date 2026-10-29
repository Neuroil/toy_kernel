//
// Created by neuroil on 2026/10/27.
//

#ifndef TOY_KERNEL_PAGE_H
#define TOY_KERNEL_PAGE_H

#include <stdatomic.h>
#include <stdint.h>

typedef struct {
    atomic_int ref_count;
} Page;

#endif //TOY_KERNEL_PAGE_H
