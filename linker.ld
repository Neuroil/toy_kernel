OUTPUT_ARCH("riscv")
ENTRY("_start")

/* 每个核心栈空间大小 - 16*8kb */
_stack_size = 0x20000;
_stack_size = (_stack_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 堆空间大小 - 256kb */
_heap_size = 0x40000;
_heap_size = (_heap_size + 15) & ~15; /* 强制按 16 字节对齐 */

/* 独立物理栈设置 - 16*8kb */
/* 栈顶对齐 */
_m_stack_top = 0x88000000;
_m_stack_size = 0x20000;
_m_stack_size = (_m_stack_size + 15) & ~15; /* 同上 */

/* 内存布局 */
SECTIONS {

    _phy_kernel_base = 0x80000000;
    /* 页表/MMU 还没实现，内核实际在物理地址 0x80000000 执行，
     * 读全局变量也会访问到未映射的 0xC0000000，
     * 等 boot.S 建立 Sv32 映射并真正跳到高地址后，再改回 0xC0000000
     */
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

    /* 早期页表
     * 链接器中的符号地址其实都是虚拟地址，
     * la依赖于pc的相对寻址，
     * 因此运行在MMU未开启阶段会出现寻到暂不可用的虚拟地址的情况。
     * 所以最好全用li获取符号地址，再手动计算。
     */
    . = ALIGN(8);
    .early_pt : AT(ADDR(.early_pt) + _kernel_offset) ALIGN(4096) {
        _early_root_pt = .;
        . += 4096;          /* 根页表 */
        _early_l2_ident = .;
        . += 4096;          /* 恒等映射二级页表 */
        _early_l2_high = .;
        . += 4096;          /* 高映射二级页表 */
        _early_l2_mmio = .;
        . += 4096;          /* MMIO 二级页表 */
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

    /* 独立物理栈，同于M模式下的trap等函数执行 */
    _m_stack_bottom = _m_stack_top - _m_stack_size;

    ASSERT(_kernel_end + _kernel_offset <= _m_stack_bottom,
           "RAM overflow: kernel reaches machine stack's bottom")

}
