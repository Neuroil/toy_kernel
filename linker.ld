OUTPUT_ARCH("riscv")
ENTRY("_start")

/* 每个核心栈空间大小 - 16*8kb */
_stack_size = 0x20000;
_stack_size = (_stack_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 堆空间大小 - 256kb */
_heap_size = 0x40000;
_heap_size = (_heap_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 权限设置 */

/* 内存布局 */
SECTIONS {

    _phy_kernel_base = 0x80000000;
    _vir_kernel_base = 0xC0000000;
    _kernel_offset   =  _phy_kernel_base - _vir_kernel_base;

    /* 起始位置 */
    . = _vir_kernel_base;

    /* 代码段 */
    . = ALIGN(4);
    .text : AT(ADDR(.text) + _kernel_offset) {
        *(.text.boot)
        *(.text)
        *(.text.*)
    }

    /* 只读数据段 */
    . = ALIGN(8);
    .rodata : AT(ADDR(.rodata) + _kernel_offset) {
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
    }

    /* 数据段 */
    . = ALIGN(8);
    .data : AT(ADDR(.data) + _kernel_offset) {
        *(.data)
        *(.data.*)
    }

    /* 小数据段：gp 相对寻址 */
    . = ALIGN(8);
    .sdata : AT(ADDR(.sdata) + _kernel_offset) {
        __global_pointer$ = . + 0x800;
        *(.sdata)
        *(.sdata.*)
    }

    . = ALIGN(8);
    .sbss : AT(ADDR(.sbss) + _kernel_offset) {
        __bss_start = .;
        *(.sbss)
        *(.sbss.*)
    }

    /* 未初始化段 */
    . = ALIGN(8);
    .bss : AT(ADDR(.bss) + _kernel_offset) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /* 数据段结束位置 */
    _data_end = .;


    /* 内核栈空间 */
    . = ALIGN(16);
    .stack : AT(ADDR(.stack) + _kernel_offset) {
        _stack_bottom = .;
        . += _stack_size;
        _stack_top = .;
    }

    /* 内核堆空间  */
    . = ALIGN(16);
    .heap : AT(ADDR(.heap) + _kernel_offset) {
        _heap_start = .;
        . += _heap_size;
        _heap_end   = .;
    }


    /* kernel结束的位置 */
    _kernel_end = .;

    ASSERT(_kernel_end <= 0xffffffff,
           "RAM overflow: kernel reach 4 GiB")

}
