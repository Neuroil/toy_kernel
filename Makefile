# ============================================================================
#  toy_kernel —— RISC-V 32 位裸机内核
#
#   make             编译内核 -> build/kernel.elf / build/kernel.bin
#   make run         在 QEMU 里启动：M 模式，无固件
#   make clean       删除 build/ 整个目录
#
#  主线是 M 模式：QEMU 的复位 ROM 固定往 0x80000000 跳，所以内核就链在那里，
#  别改 linker.ld 里的 ORIGIN(RAM)。两处必须配套的常量：
#    linker.ld 的 LENGTH = 128M  <->  `make run` 里的 -m 128M（栈顶 = ORIGIN + LENGTH）
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

# --------------------------------------------------------------- 编译 / 链接
# 必须与 linker.ld 里的 riscv 设置配套，且要命中工具链的 multilib（rv32imac/ilp32）。
# 新版 binutils（2.36+）把 Zicsr 从基础 I 拆成独立扩展：csrr/csrw/csrs 需要显式写
# _zicsr，否则报 "unrecognized opcode ... extension `zicsr' required"。
# 多出来的 _zicsr 不影响 multilib 匹配（库仍是 rv32imac/ilp32 那一份）。
ARCH := -march=rv32imac_zicsr -mabi=ilp32

# -MMD -MP：编译 .c 时顺带生成 .d，记录它 include 了哪些头文件（见文件末尾）
CFLAGS := $(ARCH) \
          -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
          -Wall -Wextra -O2 -msmall-data-limit=0 \
          -MMD -MP

# 链接用 gcc 驱动而不是直接调 ld：让它替我们挑对的 linker emulation、并找到 libgcc
# --require-defined=_start：boot.S 里还没有 _start 时让链接直接报错，而不是“成功”产出一个空内核
LDFLAGS := $(ARCH) -nostdlib -Wl,-T,linker.ld -Wl,--require-defined=_start

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
.PHONY: all run clean

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
# 内核必须正好链接在那儿——这就是 linker.ld 里 ORIGIN(RAM) = 0x80000000 的由来。
run: $(ELF)
	$(QEMU) -M virt -m 128M -nographic -bios none -kernel $(ELF)

# ------------------------------------------------------------------- 清理
# 产物全在 build/ 一个目录里，所以清理就是删目录，不用逐个点名
clean:
	rm -rf $(BUILD)

# .d 文件里是 “x.o: x.c x.h ...” 之类的规则，直接 include 进来复用 make 的依赖判断。
# 前面的 `-` 表示文件不存在时不报错（第一次构建时 .d 还没生成）。
-include $(DEPS)
