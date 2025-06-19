#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

// 系统页大小
size_t get_page_size() {
    long size = sysconf(_SC_PAGESIZE);
    return size > 0 ? (size_t)size : 4096;
}

// 文件系统块大小
size_t get_filesystem_blocksize(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) return get_page_size();
    return st.st_blksize;
}

#define BUF_MULTIPLIER 16

size_t io_blocksize(int fd) {
    size_t page_size = get_page_size();
    size_t block_size = get_filesystem_blocksize(fd);
    size_t base = (page_size > block_size) ? page_size : block_size;
    return base * BUF_MULTIPLIER;
}

char* align_alloc(size_t size) {
    size_t pagesize = get_page_size();
    void* raw = malloc(size + pagesize + sizeof(void*));
    if (!raw) return NULL;
    uintptr_t addr = (uintptr_t)raw + sizeof(void*);
    uintptr_t aligned = (addr + pagesize - 1) & ~(pagesize - 1);
    ((void**)(aligned - sizeof(void*)))[0] = raw;
    return (char*)aligned;
}

void align_free(void* ptr) {
    if (ptr) free(*((void**)((uintptr_t)ptr - sizeof(void*))));
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

    // 提示操作系统采用顺序读取优化
    if (posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        fprintf(stderr, "Warning: posix_fadvise failed, continuing anyway...\n");
    }

    size_t bufsize = io_blocksize(fd);
    char *buffer = align_alloc(bufsize);
    if (!buffer) {
        fprintf(stderr, "Error allocating aligned buffer\n");
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t nread;
    while ((nread = read(fd, buffer, bufsize)) > 0) {
        ssize_t written = 0;
        while (written < nread) {
            ssize_t n = write(STDOUT_FILENO, buffer + written, nread - written);
            if (n == -1) {
                fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
                align_free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            written += n;
        }
    }

    if (nread == -1)
        fprintf(stderr, "Error reading file '%s': %s\n", argv[1], strerror(errno));

    align_free(buffer);
    close(fd);
    return (nread == -1) ? EXIT_FAILURE : EXIT_SUCCESS;
}