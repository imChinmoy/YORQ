#include <iostream>
#include <string>

#include "../include/yorq/file_storage.hpp"

int main() {
    FileStorage storage("data.yorq");

    storage.append("Hello from YORQ!\n");

    std::string contents;

    if (storage.read_all(contents)) {
        std::cout << contents;
    } else {
        std::cerr << "Failed to read storage file\n";
        return 1;
    }

    return 0;
}
