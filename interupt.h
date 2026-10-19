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

#define CLINT_BASE 0x02000000UL
#define CLINT_MTIME_LO  (CLINT_BASE + 0xBFF8)
#define CLINT_MTIME_HI  (CLINT_BASE + 0xBFFC)

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

#define CLINT_MTIMECMP(hart) (CLINT_BASE + 0x4000 + 8 * (hart))

static inline void write_mtimecmp(uint32_t hartid, uint64_t value) {
    auto lp = (volatile uint32_t *) CLINT_MTIMECMP(hartid);
    auto hp = (volatile uint32_t *) (CLINT_MTIMECMP(hartid) + 0x4);

    *lp = 0xffffffff;
    *hp = (uint32_t)(value >> 32);
    *lp = (uint32_t)(value & 0xffffffff);
}

#define MCAUSE_MTI  0x80000007UL
#define TIMER_INTERVAL 0x00010000

void trap_c_handler(uint32_t *regs) {
    (void)regs;
    uint32_t mcause;
    asm volatile("csrr %0, mcause" : "=r"(mcause));

    if (mcause == MCAUSE_MTI) {
        uint32_t hartid;
        asm volatile("csrr %0, mhartid" : "=r"(hartid));

        uint64_t now = read_mtime();
        write_mtimecmp(hartid, now + TIMER_INTERVAL);

        static int six_seven = 67;
        // do sth
        uart_put_string("interrupt: ");
        if ((++six_seven & 1) == 1) print_uint64(67);
        else print_uint64(6767);
        uart_put_char('\n');

    }
    // 其他异常/中断
}