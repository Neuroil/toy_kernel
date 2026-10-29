//
// syscalls.c —— newlib 要求内核补上的那些“系统调用”桩
// 本文件把这 10 个补齐：_write _read _close _lseek _fstat _isatty _sbrk _exit _kill _getpid
//

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>

#include "uart.h"
#include "lac_symbols.h"
#include "power_off.h"

// 堆顶只属于本文件：写成头文件里的 static 会让每个 .c 各有一份副本
static char *heap_top = _heap_start;


ssize_t _write(int fd, const void *buf, size_t cnt) {
    // 现在就一个 fd
    (void) fd;
    auto p = (const unsigned char *) buf;

    for (size_t i = 0; i < cnt; ++i) {
        if (p[i] == '\n') uart_put_char('\r');
        uart_put_char(p[i]);
    }
    return (ssize_t) cnt;
}

// 堆增长：返回旧的堆顶，越过 _heap_end 时按 POSIX 约定返回 (void*)-1 并置 errno
void *_sbrk(ptrdiff_t incr) {
    if (heap_top + incr > _heap_end) {
        errno = ENOMEM;
        return (void *) -1;
    }
    char *prev = heap_top;
    heap_top += incr;
    return prev;
}

// 暂不支持
ssize_t _read(int fd, void *buf, size_t cnt) {
    (void) fd, (void) buf, (void) cnt;
    errno = EINVAL;
    return -1;
}

int _close(int fd) {
    (void) fd;
    errno = EINVAL;
    return -1;
}

off_t _lseek(int fd, off_t off, int whence) {
    (void) fd, (void) off, (void) whence;
    errno = EINVAL;
    return -1;
}

// 声明 stdout 是字符设备
// newlib 据此把它当成可交互的控制台
int _fstat(int fd, struct stat *st) {
    (void) fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    (void) fd;
    return 1;
}

// 裸机里没有进程退出
// 停在这里等中断
void _exit(int code) {
    (void) code;
    for (;;) __asm__ volatile("wfi");
}

int _kill(int pid, int sig) {
    (void) pid, (void) sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

// ASSERT
void __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void)func;
    uart_put_string("ASSERT: ");
    uart_put_string(file);
    uart_put_char(':');
    print_uint64((uint64_t)line);
    uart_put_string(": ");
    uart_put_string(expr);
    uart_put_char('\n');
    machine_exit_with_code(1);
}
