//
// Created by neuroil on 2026/10/17.
//

/*
 *
 * CSR 编号           名称          作用
 * 0x300	        mstatus	        M 模式状态（中断使能、特权模式等）
 * 0x301	        misa	        机器 ISA 和扩展信息
 * 0x304	        mie	            机器中断使能
 * 0x305	        mtvec	        陷阱向量基地址
 * 0x340	        mscratch	    机器暂存寄存器
 * 0x341	        mepc	        异常程序计数器
 * 0x342	        mcause	        陷阱原因
 * 0x343	        mtval	        陷阱附加值
 * 0x344	        mip	            机器中断挂起状态
 * 0xF14	        mhartid	        当前 hart 的 ID
 *
 */

#include <stdint.h>
#include "config.h"
#include "uart.h"

static inline uint64_t read_mtime() {
    auto hi = (volatile uint32_t *) CLINT_MTIME_HI;
    auto lo = (volatile uint32_t *) CLINT_MTIME_LO;
    uint32_t h1, l, h2;

    do {
        h1 = *hi;
        l = *lo;
        h2 = *hi;
    } while (h1 != h2);

    return ((uint64_t)h1 << 32 | l);
}

/* Sstc：mtime >= stimecmp 时置起 STIP。RV32 上 64 位 CSR 分高低两半写 */
static inline void write_stimecmp(uint64_t value) {
    uint32_t hi = (uint32_t)(value >> 32);
    uint32_t lo = (uint32_t)value;

    // 先写低半字 0xffffffff，保证中途不会出现一个已经过期的时间点；
    // 高半字写完后，再写低半字让完整 64 位值生效
    asm volatile("csrw stimecmp, %0" :: "r"(0xffffffffu));
    asm volatile("csrw stimecmph, %0" :: "r"(hi));
    asm volatile("csrw stimecmp, %0" :: "r"(lo));
}

void trap_c_handler(uint32_t *regs) {
    (void)regs;
    uint32_t scause;
    asm volatile("csrr %0, scause" : "=r"(scause));

    if (scause == MCAUSE_STI) {
        /* 写一个新的 stimecmp 清掉本次 STIP */
        write_stimecmp(read_mtime() + TIMER_INTERVAL);

        static int six_seven = 67;
        // do sth
        // 调度器
        uart_put_string("interrupt: ");
        if ((++six_seven & 1) == 1) print_uint64(67);
        else print_uint64(6767);
        uart_put_char('\n');

    }
    // 其他异常/中断
    // 读取磁盘

}