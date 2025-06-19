#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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

    char c;
    ssize_t n;
    while ((n = read(fd, &c, 1)) > 0) {
        if (write(STDOUT_FILENO, &c, 1) == -1) {
            fprintf(stderr, "Error writing to stdout: %s\n", strerror(errno));
            close(fd);
            return EXIT_FAILURE;
        }
    }

    if (n == -1) {
        fprintf(stderr, "Error reading file '%s': %s\n", argv[1], strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }

    if (close(fd) == -1) {
        fprintf(stderr, "Error closing file '%s': %s\n", argv[1], strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}