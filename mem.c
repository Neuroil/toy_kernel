//
// Created by neuroil on 2026/11/7.
//

#include "mem.h"

// 全局唯一的一份 PMM 状态（头文件里只有 extern 声明）
struct Page pages[NPAGES];
uint32_t bitmap[BITMAP_WORDS];
size_t free_pages;

