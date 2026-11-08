//
// Created by neuroil on 2026/10/16.
//

#include <assert.h>

#include "uart.h"
#include "power_off.h"
#include "interupt.h"
#include "mem.h"

#include "test.h"


void timer_interrupt_test() {
    extern void trap_entry(void);

    // S 模式陷阱向量（Direct 模式；trap_entry 按 4 字节对齐）
    asm volatile("csrw stvec, %0" :: "r"((uint32_t)trap_entry));

    // 设定第一次中断的时间点：mtime >= stimecmp 时置起 STIP
    write_stimecmp(read_mtime() + TIMER_INTERVAL);

    // sie.STIE (bit 5)
    asm volatile("csrs sie, %0" :: "r"(1UL << 5));

    // sstatus.SIE (bit 1)
    asm volatile("csrs sstatus, %0" :: "r"(1UL << 1));
}

void kernel_main() {
    uart_put_string("Hello, Kernel.\n");

    // init
    pmm_init();

    // do some work
    uint64_t time = read_mtime();
    uart_put_string("time: ");
    print_uint64(time);
    uart_put_char('\n');

    // timer_interrupt_test();
    // uint64_t now = read_mtime();
    // uint64_t sep = TIMER_INTERVAL * 2;
    // while (read_mtime() <= now + sep) {
    //     asm volatile("wfi");
    // }

    pt_init_top_pgt(kernel_mem.page_table);

    test_print_pte(kernel_mem.page_table, VPN_1(0xDEADBEEF), VPN_0(0xDEADBEEF));

    //
    uart_put_string("\nend\n");
    machine_power_off();
}
