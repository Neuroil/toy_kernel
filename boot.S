.section .text.boot
.global _start
.align 2

_start:
    /*
        传递给内核的初始参数:
         - a0 -> hartid
         - a1 -> DTB 地址
    */
    addi s0, a0, 0          # s0 = hartid
    addi s1, a1, 0          # s1 = DTB 地址

    /* 仅 hartid_0 继续执行 */
    bnez s0, _spin

    /* 设置物理栈指针 */
    lui  sp, %hi(_m_stack_top)
    addi sp, sp, %lo(_m_stack_top)

    /* 配置PMP可达空间 */
    lui  t0, %hi(_m_stack_top)
    addi t0, t0, %lo(_m_stack_top)
    srli t0, t0, 2          /* pmpaddr = PA >> 2 */
    csrw pmpaddr0, t0
    li t0, 0x0F             /* TOR, R/W/X, 对 S/U 生效 */
    csrw pmpcfg0, t0

    /* 设置S模式入口 */
    la t0, _s_start         /* PC-relative，MMU 关时算出物理地址 */
    csrw mepc, t0

    li t0, 0x1800
    csrc mstatus, t0
    li t0, 0x800
    csrs mstatus, t0

    /* 把S模式定时器中断委托给S模式 */
    li t0, (1 << 5)         /* mideleg.STIE */
    csrs mideleg, t0

    /* 打开Sstc，允许S模式写stimecmp：
     * menvcfg.STCE 是 bit 63，RV32 写高半字 menvcfgh 的 bit 31 */
    li t0, 0x80000000
    csrw menvcfgh, t0

    /* 允许S模式访问time/stimecmp
     * mcounteren.TM = bit 1 */
    li t0, 2
    csrs mcounteren, t0

    /* 调试用trap */
    la t0, _early_trap
    csrw mtvec, t0

    mret

_s_start:
    /* 内核栈顶 */
    la sp, _stack_top
    call build_early_page_table


    lui t0, %hi(_high_entry)
    addi t0, t0, %lo(_high_entry)
    jr t0

_high_entry:
    la sp, _stack_top

    /* 设置全局指针 gp */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    /* 清零 BSS */
    la t0, __bss_start
    la t1, __bss_end
    bgeu t0, t1, all_zero
continue:
    sw zero, 0(t0)
    addi t0, t0, 4
    bltu t0, t1, continue
all_zero:
    /* 传递 DTB 给 kernel_main */
    mv a0, s1
    call kernel_main

_spin:
    wfi
    j _spin

_early_trap:
    csrr t0, mcause
    csrr t1, mepc
    li   t2, 0x80090000
    sw   t0, 0(t2)
    sw   t1, 4(t2)
    j _spin
