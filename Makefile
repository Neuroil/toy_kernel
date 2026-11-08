# ============================================================================
#  toy_kernel —— RISC-V 32 位裸机内核
#
#   make             编译内核 -> build/kernel.elf / build/kernel.bin
#   make run         在 QEMU 里启动：M 模式，无固件
#   make clean       删除 build/ 整个目录
#
#  主线是 M 模式：QEMU 的复位 ROM 固定往 0x80000000 跳，所以内核的装载地址（LMA）
#  必须是 0x80000000；链接地址（VMA）由 linker.ld 的 _vir_kernel_base 决定。
#  页表启用前 VMA 必须等于 LMA（0x80000000），否则 C 代码第一次访问全局就会 fault。
#  两处必须配套的常量：config.h 的 PHYS_SIZE = 128M <-> `make run` 里的 -m 128M。
#
#  Makefile 与 CMake 的对应关系（重点看这三条）：
#   1. `CC := ...`        变量赋值，      ≈ set(CC ...)
#   2. `目标: 依赖`        一条规则，      ≈ 构建图上的一条边（谁先于谁）
#   3. `(TAB)命令`         规则的执行体，  必须是 TAB 缩进，不能用空格
#   模式规则 `%.o: %.c` 则是 CMake 帮你自动生成、这里要手写的那部分：
#   “任意 .c 编译出同名 .o，配方如下”。
# ============================================================================

# ------------------------------------------------------------------- 工具链
CROSS   ?= riscv32-unknown-elf-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
QEMU    ?= qemu-system-riscv32
GDB		:= $(CROSS)gdb
READELF := $(CROSS)readelf

# --------------------------------------------------------------- 编译 / 链接
# 必须与 linker.ld 里的 riscv 设置配套，且要命中工具链的 multilib（rv32imac/ilp32）。
# 新版 binutils（2.36+）把 Zicsr 从基础 I 拆成独立扩展：csrr/csrw/csrs 需要显式写
# _zicsr，否则报 "unrecognized opcode ... extension `zicsr' required"。
# 多出来的 _zicsr 不影响 multilib 匹配（库仍是 rv32imac/ilp32 那一份）。
ARCH := -march=rv32imac_zicsr -mabi=ilp32

# -MMD -MP：编译 .c 时顺带生成 .d，记录它 include 了哪些头文件（见文件末尾）
# -g：生成调试信息，GDB 才能按源码行下断点；调试时用 `make clean && make OPT=-O0`，
# 否则 -O2 会把局部变量和 assert 直接优化掉（断点看起来像“没生效”）。
OPT ?= -O2

# 代码模型 / 重定位策略：内核是固定地址镜像，“链接在哪就必须运行在哪”，
# 不能让 PC 相对寻址把“装载地址装错了”这件事掩盖过去。
#   -mcmodel=medlow    数据地址用 lui+addi 绝对编码（RV32 的 ±2GiB 覆盖全部 32 位地址）。
#                      运行地址与链接地址不一致时，第一次访问全局/字符串就 fault。
#                      不用 medany：auipc 相对寻址会让整份镜像平移后仍然“正常”，
#                      链接地址和实际装载地址对不上也发现不了。
#   -mno-relax         禁止链接器 relaxation：它会把 auipc/lui+addi 改写成 gp 相对，
#                      并删改指令，导致反汇编和断点与编译结果对不上。
#                      编译和链接都要传，否则链接器仍会改写预编译库里的指令序列。
#   -fno-pic -fno-pie  不生成 GOT/PLT 间接寻址；内核没有动态链接器，
#                      所有地址必须是链接期常量。
# 对应的链接参数见下面的 LDFLAGS。
CFLAGS := $(ARCH) \
          -mcmodel=medlow -mno-relax -fno-pic -fno-pie \
          -ffreestanding -nostdlib -fno-stack-protector \
          -Wall -Wextra $(OPT) -g -msmall-data-limit=0 \
          -MMD -MP

# 链接用 gcc 驱动而不是直接调 ld：让它替我们挑对的 linker emulation、并找到 libgcc
# --require-defined=_start：boot.S 里还没有 _start 时让链接直接报错，而不是“成功”产出一个空内核
# -mno-relax：GCC 会转成 ld 的 --no-relax，阻止链接器改写/删除我们和 libc 里的指令。
# -no-pie：即使工具链默认 PIE，也强制产出固定地址的 EXEC。
LDFLAGS := $(ARCH) -mno-relax -no-pie -nostdlib -Wl,-T,linker.ld -Wl,--require-defined=_start

# 库 + 顺序：ld 从左往右扫，只从库里“抓”出解决当前未定义符号的目标文件，
# 所以库必须排在使用者后面；-lgcc 永远放最后（libc 里也可能用到 __udivdi3 之类）。
#   -lc    newlib：printf/memcpy/malloc...
#   -lgcc  编译器辅助例程：64 位除法、软浮点...
# -nostdlib 的含义是“不自动链接 libc/libgcc、也不链接 crt0”，手动写 -lc 依然有效；
# 代价是 newlib 里碰硬件的那 10 个桩要自己实现（见 syscalls.c）。
# 另有一条捷径 --specs=nosys.specs：它替你把 libnosys.a 里的桩接上，
# 但那些桩全部返回错误，_sbrk 拿不到堆、_write 也到不了串口，对内核没意义。
LDLIBS := -lc -lgcc

# ------------------------------------------------------------ 源文件 / 产物
# 所有中间产物和成品都放进 build/，根目录只留源码。$(BUILD)/%.o 的规则里会按需建目录。
#
# 小陷阱：`VAR := 值   # 注释` 里，注释前的空格会被算进变量的值里，一路带进命令行
# （`echo "[$(X)]"` 会看见尾部空格）。所以下面这几条赋值都不写行尾注释。
BUILD  := build
KERNEL := kernel

# 通配符：新增 .c / .S 会自动被编进来
SRCS := $(wildcard *.c) $(wildcard *.S)

# boot.S -> build/boot.o, kernel.c -> build/kernel.o
OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.S=%.o)
OBJS := $(addprefix $(BUILD)/,$(OBJS))

# -MMD 生成的依赖文件，跟着 .o 一起待在 build/ 里
DEPS := $(OBJS:.o=.d)

ELF := $(BUILD)/$(KERNEL).elf
BIN := $(BUILD)/$(KERNEL).bin

# ------------------------------------------------------------------- 目标
.PHONY: all run debug clean f gdb i

all: $(BIN)

# $@ = 目标名，$^ = 所有依赖，$< = 第一个依赖；linker.ld 也是依赖：改脚本会重新链接
$(ELF): $(OBJS) linker.ld
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# 模式规则的 % 会匹配带目录的部分，所以 build/%.o 也能对上根目录里的同名源文件。
# `@mkdir -p $(dir $@)` 保证 build/ 不存在时先建出来；开头的 @ 表示不回显这条命令。
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ------------------------------------------------------------------- 运行
# -bios none：QEMU 不加载任何固件，复位 ROM 直接跳到 0x80000000 的 M 模式开始执行。
# 这是装载地址（LMA）；链接地址（VMA）由 linker.ld 决定。没有页表时两者必须相等。
# sstc=on：内核在 S 模式用 stimecmp 处理定时器中断，QEMU 必须暴露 Sstc。
run: $(ELF)
	$(QEMU) -M virt -m 128M -cpu rv32,sstc=on -nographic -bios none -kernel $(ELF)

# make debug：QEMU 停在复位向量并开放 gdbstub（localhost:1234），
# 另开一个终端执行 `riscv32-unknown-elf-gdb build/kernel.elf`，再 `target remote :1234`。
# -d int,guest_errors 可选：把异常/中断原因打到 stderr，排查 trap 很有用。
debug: $(ELF)
	$(QEMU) -M virt -m 128M -cpu rv32,sstc=on -nographic -bios none -kernel $(ELF) -s -S

gdb: $(ELF)
	$(GDB) $(ELF)

# make f：跑 3 秒，把 QEMU 的 serial 输出和第一条 trap 打到终端。
# 注意 </dev/null：QEMU 的 -nographic 在 stdin 是真实终端时会吞掉 -d 日志
#（-D 写出的文件也是 0 字节）；stdin 接 /dev/null 后才正常。
# awk 丢弃异步中断（定时器，async:1）的日志，只在第一条同步异常（async:0）
# 处退出，不再打印后续的取指死循环；QEMU 由 timeout 收尾，不会生成几百 MB 日志。
f: $(ELF)
	@timeout 3 $(QEMU) -M virt -m 128M -cpu rv32,sstc=on \
	  -nographic -bios none -kernel $(ELF) \
	  -d int,guest_errors </dev/null 2>&1 | \
	  awk '/riscv_cpu_do_interrupt/ { if (/async:1/) next; print; exit } { print }'

# ------------------------------------------------------------------- 信息
# 显示 elf 文件信息与布局
i: $(ELF)
	$(READELF) $(ELF) -Shl

# ------------------------------------------------------------------- 清理
# 产物全在 build/ 一个目录里，所以清理就是删目录，不用逐个点名
# rm -f /tmp/qemu*.log
clean:
	rm -rf $(BUILD)

# .d 文件里是 “x.o: x.c x.h ...” 之类的规则，直接 include 进来复用 make 的依赖判断。
# 前面的 `-` 表示文件不存在时不报错（第一次构建时 .d 还没生成）。
-include $(DEPS)
