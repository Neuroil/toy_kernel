//
// Created by neuroil on 2026/10/16.
//

#include "uart.h"
#include "power_off.h"

void kernel_main() {
    my_print("Hello, Kernel.\n");

    // init

    // do some work

    //
    machine_power_off();
}
