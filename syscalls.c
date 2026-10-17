//
// syscalls.c —— newlib 要求内核补上的那些“系统调用”桩
//
// libc.a（newlib）里的 printf/malloc/memcpy 全是机器无关的可移植代码，
// 真正碰硬件的部分被留成外部符号，由移植者实现：
//
//     printf -> _vfprintf_r -> _write_r -> _write
//     malloc -> _malloc_r   -> _sbrk_r  -> _sbrk
//
// 所以 `-lc` 之后缺哪个桩，ld 就报 undefined reference to `_xxx'；
// 本文件把这 10 个补齐：_write _read _close _lseek _fstat _isatty _sbrk _exit _kill _getpid
//

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>

#include "uart.h"

/* 堆：[_heap_start, _heap_end)，由 linker.ld 规划（默认 256K）；用完不是长进栈里，而是返 ENOMEM */
extern char _heap_start[];
extern char _heap_end[];

static char *heap_top = _heap_start;

/* stdout/stderr 上的字节全部走 UART0；换行补 CR，否则终端光标不回行首 */
ssize_t _write(int fd, const void *buf, size_t cnt) {
    (void) fd; /* 裸机只有一个串口，不区分 fd */
    auto p = (const unsigned char *) buf;

    for (size_t i = 0; i < cnt; ++i) {
        if (p[i] == '\n') uart_put_char('\r');
        uart_put_char(p[i]);
    }
    return (ssize_t) cnt;
}

/* 堆增长：返回旧的堆顶，越过 _heap_end 时按 POSIX 约定返回 (void*)-1 并置 errno */
void *_sbrk(ptrdiff_t incr) {
    if (heap_top + incr > _heap_end) {
        errno = ENOMEM;
        return (void *) -1;
    }
    char *prev = heap_top;
    heap_top += incr;
    return prev;
}

/* 没有任何输入设备 */
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

/* 声明 stdout 是字符设备：newlib 据此把它当成可交互的控制台 */
int _fstat(int fd, struct stat *st) {
    (void) fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    (void) fd;
    return 1;
}

/* 裸机里没有“进程退出”，停在这里等中断 */
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
