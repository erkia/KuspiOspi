#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));

register char *stack_ptr asm("sp");

caddr_t _sbrk(int incr)
{
    extern char end asm("end");
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &end;
    }

    prev_heap_end = heap_end;
    if (heap_end + incr > stack_ptr) {
        errno = ENOMEM;
        return (caddr_t) -1;
    }

    heap_end += incr;

    return (caddr_t) prev_heap_end;
}

int _kill(int pid, int sig)
{
    errno = EINVAL;
    return -1;
}

void _exit (int status)
{
    _kill(status, -1);
    while (1) {}
}

int _open(char *path, int flags, ...)
{
    return -1;
}

int _close(int file)
{
    return -1;
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
    if (file != STDIN_FILENO) {
        return -1;
    }

    for (int p = 0; p < len; p++) {
        *ptr++ = __io_getchar();
    }

    return len;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
        return -1;
    }

    for (int p = 0; p < len; p++) {
        __io_putchar(*ptr++);
    }

    return len;
}

int _fstat(int file, struct stat *st)
{
    if (file == STDIN_FILENO || file == STDOUT_FILENO || file == STDERR_FILENO) {
        st->st_mode = S_IFCHR;
        return 0;
    } else {
        return -1;
    }
}

int _isatty(int file)
{
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    return 0;
}
