// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "inventory.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ay-engine-inventory MODULES_ROOT ABI\n";
        return 2;
    }
    for (const auto& module : ay::engine::discover_candidates(argv[1], argv[2])) {
        std::cout << "CANDIDATE|" << module.module_id << "|" << module.abi
                  << "|" << module.library_bytes << "\n";
    }
    return 0;
}
