//
// Created by neuroil on 2026/10/26.
//

#ifndef TOY_KERNEL_L_A_C_SYMBOLS_H
#define TOY_KERNEL_L_A_C_SYMBOLS_H

// 物理内存范围
extern char  _phy_kernel_base[];
extern char  _vir_kernel_base[];
extern char  _kernel_offset[];
extern char _kernel_end[];


// 内核堆空间：[_heap_start, _heap_end)
// 用完返回 ENOMEM
extern char _heap_start[];
extern char _heap_end[];


#endif //TOY_KERNEL_L_A_C_SYMBOLS_H
