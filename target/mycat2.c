#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

// 获取当前系统的内存页大小
size_t io_blocksize() {
    long size = sysconf(_SC_PAGESIZE);
    if (size == -1) {
        // 如果获取失败，返回默认 4096
        return 4096;
    }
    return (size_t)size;
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
    char *buffer = malloc(bufsize);
    if (!buffer) {
        fprintf(stderr, "Error allocating buffer\n");
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
                free(buffer);
                close(fd);
                return EXIT_FAILURE;
            }
            nwritten += n;
        }
    }

    if (nread == -1) {
        fprintf(stderr, "Error reading file '%s': %s\n", argv[1], strerror(errno));
        free(buffer);
        close(fd);
        return EXIT_FAILURE;
    }

    free(buffer);
    close(fd);
    return EXIT_SUCCESS;
}