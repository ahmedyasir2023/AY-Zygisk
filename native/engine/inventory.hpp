// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ay::engine {

struct Candidate {
    std::string module_id;
    std::string abi;
    std::uint64_t library_bytes;
};

// Read-only, symlink-safe discovery of Zygisk library *candidates* from one
// trusted root manager's modules directory. Never dlopen these paths.
// ABI/ELF header matching does not prove compatibility or safety.
std::vector<Candidate> discover_candidates(const std::string& modules_root,
                                           const std::string& abi);

}  // namespace ay::engine
