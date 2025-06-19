#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

// 获取系统内存页大小
size_t io_blocksize() {
    long size = sysconf(_SC_PAGESIZE);
    if (size == -1) return 4096;  // fallback
    return (size_t)size;
}

// 对齐分配: 返回页大小对齐的指针，原始指针保存在头部
char* align_alloc(size_t size) {
    size_t pagesize = io_blocksize();
    void* raw = malloc(size + pagesize + sizeof(void*));
    if (!raw) return NULL;

    uintptr_t addr = (uintptr_t)raw + sizeof(void*);
    uintptr_t aligned = (addr + pagesize - 1) & ~(pagesize - 1);

    void** p = (void**)(aligned - sizeof(void*));
    *p = raw;  // 保存原始指针
    return (char*)aligned;
}

// 释放 align_alloc 分配的内存
void align_free(void* ptr) {
    if (!ptr) return;
    void* raw = *((void**)((uintptr_t)ptr - sizeof(void*)));
    free(raw);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening file '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    size_t bufsize = io_blocksize();
    char *buffer = align_alloc(bufsize);
    if (!buffer) {
        fprintf(stderr, "Error allocating aligned buffer\n");
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t nread;
    while ((nread = read(fd, buffer, bufsize)) > 0) {
        ssize_t nwritten = 0;
        while (nwritten < nread) {
            ssize_t n = write(STDOUT_FILENO, buffer + nwritten, nread - nwritten);
            if (n == -1) {
                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                align_free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            nwritten += n;
        }
    }

    if (nread == -1) {
        fprintf(stderr, "Error reading file '%s': %s\n", argv[1], strerror(errno));
        align_free(buffer);
        close(fd);
        return EXIT_FAILURE;
    }

    align_free(buffer);
    close(fd);
    return EXIT_SUCCESS;
}