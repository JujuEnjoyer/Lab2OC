#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <random>
#include <cstdlib>
#include <cstring>
#include "../../diy-shell-benchmark-24y-JujuEnjoyer/app/commands/child process commands/io_lat_read.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <file path> <block size (bytes)> <iterations>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filePath = argv[1];
    std::size_t blockSize = std::stoul(argv[2]);
    int iterations = std::stoi(argv[3]);

    randomReadTest(filePath, blockSize, iterations);

    return 0;
}
void randomReadTest(const std::string& filename, std::size_t blockSize, int iterations) {
    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Error opening test file!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Получение размера файла
    file.seekg(0, std::ios::end);
    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (blockSize > fileSize) {
        std::cerr << "Block size is larger than the file size!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Буфер для хранения всего файла в памяти
    std::vector<char> fileCache(fileSize);
    file.read(fileCache.data(), fileSize);
    if (file.gcount() < static_cast<std::streamsize>(fileSize)) {
        std::cerr << "Error reading the entire file into memory!" << std::endl;
        exit(EXIT_FAILURE);
    }
    file.close();

    // Генерация случайных позиций
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> dist(0, fileSize - blockSize);

    double totalDuration = 0.0;
    std::vector<char> buffer(blockSize);

    for (int i = 0; i < iterations; ++i) {
        // Генерация случайной позиции
        std::size_t randomPos = dist(rng);

        // Измерение времени чтения из кэша
        auto start = std::chrono::high_resolution_clock::now();
        std::memcpy(buffer.data(), fileCache.data() + randomPos, blockSize);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::micro> duration = end - start;
        totalDuration += duration.count();
    }

    std::cout << "Average latency: " << (totalDuration / iterations) << " µs" << std::endl;
}
