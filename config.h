//
// Created by neuroil on 2026/10/26.
//

#ifndef TOY_KERNEL_CONFIG_H
#define TOY_KERNEL_CONFIG_H

// ====== hardware ======

#define PHYS_BASE       0x80000000UL
#define PHYS_SIZE       (128UL * 1024 * 1024)
#define PHYS_END        (PHYS_BASE + PHYS_SIZE)

// ====== page ======

#define PAGE_SIZE       4096UL
//页偏移
#define PAGE_SHIFT      12
// 页数
#define NPAGES          (PHYS_SIZE / PAGE_SIZE)
// 至少需要的字节数
#define BITMAP_WORDS    ((NPAGES + 31) / 32)       /* 1024 */

// ====== uart ======
#define UART0_BASE      0x10000000UL
#define UART_THR        (UART0_BASE + 0x00)
#define UART_LSR        (UART0_BASE + 0x05)
#define UART_LSR_THRE   0x20

// ====== power ======
#define TEST_BASE       0x100000UL

// ====== interruption ======
#define CLINT_BASE      0x02000000UL
#define CLINT_MTIME_LO  (CLINT_BASE + 0xBFF8)
#define CLINT_MTIME_HI  (CLINT_BASE + 0xBFFC)
#define MCAUSE_STI      0x80000005UL
#define TIMER_INTERVAL  0x00010000

#endif //TOY_KERNEL_CONFIG_H
