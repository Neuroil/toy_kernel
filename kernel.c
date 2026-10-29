//
// Created by neuroil on 2026/10/16.
//

#include <assert.h>

#include "uart.h"
#include "power_off.h"
#include "interupt.h"
#include "pmm.h"


void timer_interrupt_test() {
    uint32_t hartid;
    asm volatile("csrr %0, mhartid" : "=r"(hartid));

    extern void trap_entry(void);
    asm volatile("csrw mtvec, %0" :: "r"((uint32_t)trap_entry));

    // 设定第一次中断的时间点
    uint64_t now = read_mtime();
    write_mtimecmp(hartid, now + TIMER_INTERVAL);

    // mie.MTIE (bit 7)
    asm volatile("csrs mie, %0" :: "r"(1UL << 7));

    // mstatus.MIE (bit 3)
    asm volatile("csrs mstatus, %0" :: "r"(1UL << 3));
}

void kernel_main() {
    uart_put_string("Hello, Kernel.\n");

    // init
    pmm_init();

    // do some work
    uint64_t time = read_mtime();
    print_uint64(time);
    uart_put_char('\n');

    // timer_interrupt_test();
    // for (;;){}

    uintptr_t new_page = pmm_alloc_page();
    assert(new_page != 0);
    assert(bit_get(pa_to_idx(new_page)) == 1);

    print_uint64(new_page);
    uart_put_char('\n');

    pmm_free_page(new_page);
    assert(bit_get(pa_to_idx(new_page)) == 0);

    //
    uart_put_string("\nend\n");
    machine_power_off();
}
