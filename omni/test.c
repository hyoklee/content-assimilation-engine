#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    int fd = shm_open("/test_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1) { perror("shm_open"); return 1; }
    if (ftruncate(fd, 4096) == -1) { perror("ftruncate"); return 1; }
    void *addr = mmap(NULL, 4096, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) { perror("mmap"); return 1; }
    printf("mmap succeeded\n");
    shm_unlink("/test_shm");
    return 0;
}