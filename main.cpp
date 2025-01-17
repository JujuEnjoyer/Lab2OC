#include "api/api.h"
#include <cstring>
#include <iostream>

int main() {
    int fd = lab2_open("/home/nic/CLionProjects/Lab2OC/test_file.txt", 4);
    if (fd == -1) {
        std::cerr << "АЙ меня снайпнули в полете\n";
        return 1;
    }
    std::cout << "Открыл с божьей помошью" << fd << "\n";

    char read_buffer[100] = {0};
    ssize_t bytes_read = lab2_read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read == -1) {
        std::cerr << "Error reading file\n";
        lab2_close(fd);
        return 1;
    }
    std::cout << "читаем " << bytes_read << " байт: " << read_buffer << "\n";

    lab2_lseek(fd, 0, SEEK_END);

    const char *write_data = "трес";
    ssize_t bytes_written = lab2_write(fd, write_data, strlen(write_data));
    if (bytes_written == -1) {
        std::cerr << "Я не умею писать у меня лапки \n";
        lab2_close(fd);
        return 1;
    }
    std::cout << "пишу " << bytes_written << " байт в файлик\n";

    if (lab2_fsync(fd) == -1) {
        std::cerr << "ну разные данные че бубнить\n";
        lab2_close(fd);
        return 1;
    }
    std::cout << "Мы один человек ?\n";

    if (lab2_close(fd) == -1) {
        std::cerr << "Ты кому рот закрыть пытаешься ? \n";
        return 1;
    }
    std::cout << "Ладно закрыл\n";

    return 0;
}