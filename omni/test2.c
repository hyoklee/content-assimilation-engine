#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main() {
    // Open a shared memory object
    int fd = shm_open("/test_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }

    // Set the size of the shared memory object
    size_t size = 4096;
    if (ftruncate(fd, size) == -1) {
        perror("ftruncate");
        shm_unlink("/test_shm");
        return 1;
    }

    // Map the shared memory object
    void *addr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        shm_unlink("/test_shm");
        return 1;
    }

    // Write data to shared memory
    const char *message = "Hello, shared memory!";
    memcpy(addr, message, strlen(message) + 1); // Include null terminator

    // Read data from shared memory
    char *read_data = (char *)addr;
    printf("Data read from shared memory: %s\n", read_data);

    // Clean up
    if (munmap(addr, size) == -1) {
        perror("munmap");
    }
    if (shm_unlink("/test_shm") == -1) {
        perror("shm_unlink");
    }
    close(fd);

    return 0;
}
