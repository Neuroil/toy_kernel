OUTPUT_ARCH("riscv")
ENTRY("_start")

/* 栈空间大小 - 64kb */
_stack_size = 0x10000;
_stack_size = (_stack_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 堆空间大小 - 256kb */
_heap_size = 0x40000;
_heap_size = (_heap_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 内存大小 */
MEMORY {
    RAM : ORIGIN = 0x80000000, LENGTH = 128M
}

/* 内存布局 */
SECTIONS {
    /* 起始位置 */
    . = ORIGIN(RAM);

    /* 代码段 */
    .text : ALIGN(4) {
        *(.text.boot)
        *(.text)
        *(.text.*)
    } > RAM

    /* 只读数据段 */
    .rodata : ALIGN(8) {
        *(.rodata)
        *(.rodata.*)
        *(.srodata)
        *(.srodata.*)
    } > RAM

    /* 数据段 */
    .data : ALIGN(8) {
        *(.data)
        *(.data.*)
    } > RAM

    /* 小数据段：gp 相对寻址 */
    .sdata : ALIGN(8) {
        __global_pointer$ = . + 0x800;
        *(.sdata)
        *(.sdata.*)
    } > RAM

    .sbss : ALIGN(8) {
        __bss_start = .;
        *(.sbss)
        *(.sbss.*)
    } > RAM

    /* 未初始化段 */
    .bss : ALIGN(8) {
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    /* 数据段结束位置 */
    _data_end = .;

    /* 堆空间  */
    _heap_start = ALIGN(16);
    _heap_end   = _heap_start + _heap_size;

    /* 栈空间 */
    _stack_top    = (ORIGIN(RAM) + LENGTH(RAM)) & ~0xf;
    _stack_bottom = _stack_top - _stack_size;

    ASSERT(_heap_end <= _stack_bottom,
           "RAM overflow: heap collides with stack!")

}