// Copyright (c) 2026 Eng. Ahmed Yasir — MIT
#include "inventory.hpp"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ay::engine {
namespace {

struct OwnedFd {
    explicit OwnedFd(int value = -1) : value(value) {}
    ~OwnedFd() { if (value >= 0) close(value); }
    OwnedFd(const OwnedFd&) = delete;
    OwnedFd& operator=(const OwnedFd&) = delete;
    int release() { const int result = value; value = -1; return result; }
    int value;
};

struct AbiInfo {
    unsigned char elf_class;
    unsigned machine;
};

bool abi_info(const std::string& abi, AbiInfo* out) {
    if (out == nullptr) return false;
    if (abi == "arm64-v8a") *out = {2, 183};
    else if (abi == "armeabi-v7a") *out = {1, 40};
    else if (abi == "x86") *out = {1, 3};
    else if (abi == "x86_64") *out = {2, 62};
    else return false;
    return true;
}

bool safe_module_id(const char* id) {
    if (id == nullptr || id[0] == 0) return false;
    const auto length = std::strlen(id);
    if (length > 96) return false;
    for (std::size_t i = 0; i < length; ++i) {
        const unsigned char c = static_cast<unsigned char>(id[i]);
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '.' || c == '_' ||
              c == '-')) return false;
    }
    return std::strcmp(id, ".") != 0 && std::strcmp(id, "..") != 0;
}

bool exists(int dir, const char* name) {
    struct stat metadata {};
    // Even a symlink marker is treated conservatively as a disable marker.
    return fstatat(dir, name, &metadata, AT_SYMLINK_NOFOLLOW) == 0;
}

bool regular_file(int dir, const char* name) {
    OwnedFd file(openat(dir, name, O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (file.value < 0) return false;
    struct stat metadata {};
    return fstat(file.value, &metadata) == 0 && S_ISREG(metadata.st_mode);
}

bool candidate_elf(int file, const AbiInfo& abi, std::uint64_t* bytes) {
    struct stat metadata {};
    if (fstat(file, &metadata) != 0 || !S_ISREG(metadata.st_mode) ||
        metadata.st_size < 64 || metadata.st_size > (128LL << 20)) {
        return false;
    }
    unsigned char header[64]{};
    if (pread(file, header, sizeof(header), 0) !=
        static_cast<ssize_t>(sizeof(header))) return false;
    if (header[0] != 0x7f || header[1] != 'E' || header[2] != 'L' ||
        header[3] != 'F' || header[4] != abi.elf_class ||
        header[5] != 1 || header[6] != 1) return false;
    const unsigned type = static_cast<unsigned>(header[16]) |
                          (static_cast<unsigned>(header[17]) << 8u);
    const unsigned machine = static_cast<unsigned>(header[18]) |
                             (static_cast<unsigned>(header[19]) << 8u);
    if (type != 3 || machine != abi.machine) return false;  // ET_DYN
    *bytes = static_cast<std::uint64_t>(metadata.st_size);
    return true;
}

}  // namespace

std::vector<Candidate> discover_candidates(const std::string& modules_root,
                                           const std::string& abi) {
    std::vector<Candidate> result;
    AbiInfo expected{};
    if (!abi_info(abi, &expected)) return result;
    OwnedFd root(open(modules_root.c_str(),
                      O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW));
    if (root.value < 0) return result;
    DIR* directory = fdopendir(root.value);
    if (directory == nullptr) return result;
    root.release();  // closedir now owns the underlying descriptor.

    bool enumeration_error = false;
    while (result.size() < 4096) {
        errno = 0;
        dirent* entry = readdir(directory);
        if (entry == nullptr) {
            enumeration_error = (errno != 0);
            break;
        }
        if (!safe_module_id(entry->d_name)) continue;
        OwnedFd module(openat(dirfd(directory), entry->d_name,
                              O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW));
        if (module.value < 0 || exists(module.value, "disable") ||
            exists(module.value, "remove") ||
            !regular_file(module.value, "module.prop")) continue;
        OwnedFd zygisk(openat(module.value, "zygisk",
                             O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW));
        if (zygisk.value < 0) continue;
        const std::string library_name = abi + ".so";
        OwnedFd library(openat(zygisk.value, library_name.c_str(),
                               O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
        if (library.value < 0) continue;
        std::uint64_t library_bytes = 0;
        if (!candidate_elf(library.value, expected, &library_bytes)) continue;
        result.push_back({entry->d_name, abi, library_bytes});
    }
    closedir(directory);
    if (enumeration_error) result.clear();
    std::sort(result.begin(), result.end(),
              [](const Candidate& lhs, const Candidate& rhs) {
                  return lhs.module_id < rhs.module_id;
              });
    return result;
}

}  // namespace ay::engine
