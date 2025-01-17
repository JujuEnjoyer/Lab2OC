#ifndef API_H
#define API_H

#include <cstddef>
#include <cstdint>
#include <unistd.h>

using offset_t = int64_t;

int lab2_open(const char *path, size_t max_cache_size = 4);
int lab2_close(int fd);
ssize_t lab2_read(int fd, void *buf, size_t count);
ssize_t lab2_write(int fd, const void *buf, size_t count);
offset_t lab2_lseek(int fd, offset_t offset, int whence);
int lab2_fsync(int fd);


#endif //API_H
