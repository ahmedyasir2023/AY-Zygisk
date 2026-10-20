// AY Zygisk Native Lab - original ELF-only Zygisk module preflight scanner.
// Developer: Eng. Ahmed Yasir. This program never loads or executes scanned code.
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr uint64_t kMaxFile = 64ULL * 1024ULL * 1024ULL;
constexpr uint32_t kShtDynsym = 11;
struct AbiInfo {const char *filename, *name; uint16_t machine; uint8_t bits;};
constexpr std::array<AbiInfo, 5> kAbis = {{{"arm64-v8a.so", "arm64-v8a", 183, 2},
    {"armeabi-v7a.so", "armeabi-v7a", 40, 1}, {"x86.so", "x86", 3, 1},
    {"x86_64.so", "x86_64", 62, 2}, {"riscv64.so", "riscv64", 243, 2}}};

std::string safe(const std::string &text) {
    std::string out; out.reserve(std::min(text.size(), size_t(100)));
    for (unsigned char ch : text) {
        if (out.size() >= 100) break;
        out.push_back(ch < 32 || ch == '|' || ch == 127 ? '_' : char(ch));
    }
    return out;
}

bool span(uint64_t off, uint64_t len, uint64_t total) {
    return off <= total && len <= total - off;
}
uint64_t le(const std::vector<uint8_t>& b, uint64_t off, unsigned n) {
    if (!span(off, n, b.size())) return 0;
    uint64_t result = 0;
    for (unsigned i = 0; i < n; ++i) result |= uint64_t(b[off+i]) << (i * 8);
    return result;
}
std::string get_string(const std::vector<uint8_t> &b, uint64_t offset, uint64_t end) {
    if (offset >= end || end > b.size()) return {};
    std::string out;
    for (uint64_t i = offset; i < end && b[i] != 0 && out.size() <= 128; ++i)
        out.push_back(char(b[i]));
    return out;
}
struct Section {uint32_t type; uint64_t offset, size, entsize; uint32_t link;};
struct Result {std::string status, arch, entry; uint64_t bytes{0};};
const AbiInfo* matching(uint16_t machine, uint8_t bits) {
    for (const auto &abi: kAbis) if (abi.machine == machine && abi.bits == bits) return &abi;
    return nullptr;
}
Result inspect(const fs::path &path, const AbiInfo* expected = nullptr) {
    Result r{"invalid-elf", "unknown", "no", 0};
    std::error_code ec;
    if (!fs::is_regular_file(fs::symlink_status(path, ec)) || ec) {
        r.status = "not-regular-file"; return r;
    }
    r.bytes = fs::file_size(path, ec);
    if (ec) {r.status="unreadable";return r;}
    if (r.bytes > kMaxFile) {r.status="too-large";return r;}
    if (r.bytes < 52) return r;
    std::ifstream in(path, std::ios::binary);
    if (!in) {r.status="unreadable";return r;}
    std::vector<uint8_t> b(static_cast<size_t>(r.bytes));
    if (!in.read(reinterpret_cast<char *>(b.data()), static_cast<std::streamsize>(b.size()))) {
        r.status="unreadable"; return r;
    }
    if (b[0]!=0x7f || b[1]!='E' || b[2]!='L' || b[3]!='F') return r;
    uint8_t bits=b[4];
    if (bits!=1 && bits!=2) return r;
    if (b[5]!=1) {r.status="unsupported-endian";return r;}
    const uint64_t hdr_size = bits == 2 ? 64 : 52;
    if (b.size() < hdr_size) return r;
    const uint16_t type = static_cast<uint16_t>(le(b,16,2));
    const uint16_t machine = static_cast<uint16_t>(le(b,18,2));
    const AbiInfo* found=matching(machine,bits);
    if (!found) {r.status="unsupported-architecture";return r;}
    r.arch = found->name;
    if (expected && (machine!=expected->machine || bits!=expected->bits)) {
        r.status="architecture-mismatch";return r;
    }
    if (type!=3) {r.status="not-shared-library";return r;}
    const uint64_t shoff=le(b,bits==2?40:32,bits==2?8:4);
    const uint64_t shentsize=le(b,bits==2?58:46,2);
    const uint64_t shnum=le(b,bits==2?60:48,2);
    const uint64_t min_entry=bits==2?64:40;
    if (!shnum || shnum>4096 || shentsize<min_entry ||
        shentsize>512 || shnum > std::numeric_limits<uint64_t>::max()/shentsize ||
        !span(shoff,shnum*shentsize,b.size())) {
        r.status="invalid-section-table";return r;
    }
    std::vector<Section> sections;
    sections.reserve(static_cast<size_t>(shnum));
    for (uint64_t idx=0;idx<shnum;++idx) {
        const uint64_t o=shoff+idx*shentsize;
        Section s{};
        s.type=static_cast<uint32_t>(le(b,o+4,4));
        s.offset=le(b,o+(bits==2?24:16),bits==2?8:4);
        s.size=le(b,o+(bits==2?32:20),bits==2?8:4);
        s.link=static_cast<uint32_t>(le(b,o+(bits==2?40:24),4));
        s.entsize=le(b,o+(bits==2?56:36),bits==2?8:4);
        if (s.type!=8 && !span(s.offset,s.size,b.size())) {
            r.status="invalid-section-bounds";return r;
        }
        sections.push_back(s);
    }
    bool has_entry=false, has_companion=false, has_dynsym=false;
    for (const Section& s:sections) {
        if (s.type!=kShtDynsym) continue;
        has_dynsym=true;
        if (s.link>=sections.size() || !s.entsize || s.entsize < (bits==2?24:16)) {
            r.status="invalid-symbol-table";return r;
        }
        const Section& strings=sections[s.link];
        if (strings.type!=3 || s.size/s.entsize > 200000) {
            r.status="invalid-symbol-table";return r;
        }
        for (uint64_t o=s.offset; o < s.offset+s.size; o+=s.entsize) {
            if (!span(o,4,b.size()) || !span(o,s.entsize,s.offset+s.size)) break;
            const uint32_t no=static_cast<uint32_t>(le(b,o,4));
            if (no>=strings.size) continue;
            const std::string name=get_string(b,strings.offset+no,strings.offset+strings.size);
            if (name=="zygisk_module_entry") has_entry=true;
            if (name=="zygisk_companion_entry") has_companion=true;
        }
    }
    r.entry=has_entry?"module":has_companion?"companion-only":"no";
    if (has_entry) r.status="valid-zygisk";
    else if (has_dynsym) r.status="missing-module-entry";
    else r.status="missing-dynsym";
    return r;
}
void print_file(const std::string& id, const std::string& abi, const Result& r) {
    std::cout<<"LIB|"<<safe(id)<<'|'<<safe(abi)<<'|'<<r.status<<'|'<<r.arch<<'|'
             <<r.entry<<'|'<<r.bytes<<'\n';
}
int inventory(const fs::path& root) {
    std::error_code ec;
    if (!fs::is_directory(root,ec)) {
        std::cout<<"NATIVE|state|module-root-unavailable\n";return 1;
    }
    unsigned modules=0, libs=0, warnings=0;
    for (fs::directory_iterator it(root,fs::directory_options::skip_permission_denied,ec),end;
         !ec && it!=end;it.increment(ec)) {
        if (!it->is_directory(ec) || ec) {ec.clear();continue;}
        const fs::path dir=it->path();
        if (!fs::is_regular_file(dir/"module.prop",ec)) {ec.clear();continue;}
        const fs::path zdir=dir/"zygisk";
        if (!fs::is_directory(zdir,ec)) {ec.clear();continue;}
        ++modules;
        const std::string id=dir.filename().string();
        const bool disabled=fs::exists(dir/"disable",ec);
        std::cout<<"MODULE|"<<safe(id)<<'|'<<(disabled?"disabled":"enabled")<<'\n';
        for (const AbiInfo& abi:kAbis) {
            const fs::path path=zdir/abi.filename;
            if (!fs::exists(path,ec)) {ec.clear();continue;}
            const Result r=inspect(path,&abi);
            print_file(id,abi.name,r);
            ++libs;
            if (r.status!="valid-zygisk") ++warnings;
        }
    }
    if (ec) {std::cout<<"WARNING|Unable to enumerate module root\n"; ++warnings;}
    std::cout<<"SUMMARY|native_modules|"<<modules<<'\n';
    std::cout<<"SUMMARY|native_libraries|"<<libs<<'\n';
    std::cout<<"SUMMARY|native_warnings|"<<warnings<<'\n';
    return warnings?1:0;
}
} // namespace
int main(int argc, char** argv) {
    if (argc==2 && std::string(argv[1])=="version") {
        std::cout<<"NATIVE|version|0.3.0\nNATIVE|mode|read-only-preflight\n"; return 0;
    }
    if (argc==3 && std::string(argv[1])=="inspect") {
        const Result r=inspect(fs::path(argv[2]));
        print_file("single",fs::path(argv[2]).filename().string(),r);
        return r.status=="valid-zygisk"?0:1;
    }
    if (argc==3 && std::string(argv[1])=="inventory") return inventory(fs::path(argv[2]));
    std::cerr<<"AY Zygisk Native Lab: version | inspect FILE | inventory MODULE_ROOT\n";
    return 2;
}
