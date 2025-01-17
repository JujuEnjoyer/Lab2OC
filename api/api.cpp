#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <vector>
#include <deque>
#include <iostream>
#include <sys/stat.h>

using namespace std;
using offset_t = int64_t;

struct CachePage {
    offset_t offset;
    vector<char> data;
    bool dirty;
    bool used;
};

struct CachedFile {
    int fd;
    map<offset_t, CachePage> cache;
    deque<offset_t> cache_queue;
    size_t max_cache_size;
    offset_t current_offset;
};

map<int, CachedFile> open_files;

const size_t PAGE_SIZE = 4096;

inline offset_t page_offset(offset_t offset) {
    return offset & ~(PAGE_SIZE - 1);
}

void second_chance_evict(CachedFile &file) {
    while (!file.cache_queue.empty()) {
        offset_t offset = file.cache_queue.front();
        file.cache_queue.pop_front();

        CachePage &page = file.cache[offset];
        if (page.used) {
            page.used = false;
            file.cache_queue.push_back(offset);
        } else {
            if (page.dirty) {
                cout << "удаление dirty бейби по смещению" << offset << endl;
                lseek(file.fd, offset, SEEK_SET);
                write(file.fd, page.data.data(), PAGE_SIZE);
            } else {
                cout << "удаление clear бейби по смещению" << offset;
            }
            file.cache.erase(offset);
            return;
        }
    }
}

int lab2_open(const char *path, size_t max_cache_size = 4) {
    int fd = open(path, O_RDWR | O_DIRECT);
    if (fd == -1) return -1;

    CachedFile cachedFile = {fd, {}, {}, max_cache_size, 0};
    open_files[fd] = cachedFile;
    return fd;
}

int lab2_close(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    for (auto &entry : it->second.cache) {
        if (entry.second.dirty) {
            lseek(fd, entry.first, SEEK_SET);
            write(fd, entry.second.data.data(), PAGE_SIZE);
        }
    }
    close(fd);
    open_files.erase(fd);
    return 0;
}

ssize_t lab2_read(int fd, void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    size_t bytesRead = 0;

    while (count > 0) {
        offset_t page_offset = file.current_offset & ~(PAGE_SIZE - 1);
        size_t offset_in_page = file.current_offset % PAGE_SIZE;
        size_t to_read = min(count, PAGE_SIZE - offset_in_page);

        if (file.cache.find(page_offset) == file.cache.end()) {
            if (file.cache.size() >= file.max_cache_size) second_chance_evict(file);

            CachePage page;
            page.offset = page_offset;
            page.data.resize(PAGE_SIZE, 0);
            page.dirty = false;
            page.used = true;

            lseek(fd, page_offset, SEEK_SET);
            ssize_t result = read(fd, page.data.data(), PAGE_SIZE);
            if (result < 0) {
                cerr << "Я читать не умею" << page_offset << endl;
                return -1;
            }
            file.cache[page_offset] = page;
            file.cache_queue.push_back(page_offset);
            cout << "А он вообще латыж (прочитал по смещению )" << page_offset << ", это что буквы? " << result << endl;
        }


        CachePage &page = file.cache[page_offset];
        memcpy((char *)buf + bytesRead, page.data.data() + offset_in_page, to_read);
        page.used = true;

        file.current_offset += to_read;
        bytesRead += to_read;
        count -= to_read;
    }

    return bytesRead;
}

ssize_t lab2_write(int fd, const void *buf, size_t count) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    size_t bytesWritten = 0;

    while (count > 0) {
        offset_t page_offset = file.current_offset & ~(PAGE_SIZE - 1);
        size_t offset_in_page = file.current_offset % PAGE_SIZE;
        size_t to_write = min(count, PAGE_SIZE - offset_in_page);

        if (file.cache.find(page_offset) == file.cache.end()) {
            if (file.cache.size() >= file.max_cache_size) second_chance_evict(file);

            CachePage page;
            page.offset = page_offset;
            page.data.resize(PAGE_SIZE, 0);
            page.dirty = true;
            page.used = true;

            file.cache[page_offset] = page;
            file.cache_queue.push_back(page_offset);
        }

        CachePage &page = file.cache[page_offset];
        memcpy(page.data.data() + offset_in_page, (const char *)buf + bytesWritten, to_write);
        page.dirty = true;
        page.used = true;

        file.current_offset += to_write;
        bytesWritten += to_write;
        count -= to_write;
    }

    return bytesWritten;
}

offset_t lab2_lseek(int fd, offset_t offset, int whence) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    CachedFile &file = it->second;
    switch (whence) {
        case SEEK_SET:
            file.current_offset = offset;
            break;
        case SEEK_CUR:
            file.current_offset += offset;
            break;
        case SEEK_END:
            struct stat st;
            if (fstat(fd, &st) == -1) return -1;
            file.current_offset = st.st_size + offset;
            break;
        default:
            return -1;
    }

    return file.current_offset;
}

int lab2_fsync(int fd) {
    auto it = open_files.find(fd);
    if (it == open_files.end()) return -1;

    for (auto &entry : it->second.cache) {
        if (entry.second.dirty) {
            cout << "Синхронизация грязи по смещению " << entry.first << endl;
            lseek(fd, entry.first, SEEK_SET);
            write(fd, entry.second.data.data(), PAGE_SIZE);
            entry.second.dirty = false;
        }
    }

    return fsync(fd);
}
