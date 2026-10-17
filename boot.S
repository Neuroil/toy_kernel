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
    bnez s0, _park

    /* 设置栈指针 */
    la sp, _stack_top

    /* 设置全局指针 gp */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop

    /* 清零 BSS */
    la t0, __bss_start
    la t1, __bss_end
    bgeu t0, t1, 2f
1:
    sw zero, 0(t0)
    addi t0, t0, 4
    bltu t0, t1, 1b
2:

    /* 传递 DTB 给 kernel_main */
    mv a0, s1
    call kernel_main

_park:
    wfi
    j _park
