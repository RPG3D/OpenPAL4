#include "pal4/rw_geometry_flags.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IRwGeometryFlagsRuntime {
public:
    std::uint32_t ReadGeometryWord(std::uint32_t address) override {
        reads.push_back(address);
        return words[address];
    }
    void WriteGeometryWord(std::uint32_t address,
                           std::uint32_t value) override {
        writes.push_back(address);
        words[address] = value;
    }
    std::uint32_t ForAllGeometryFlagAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   pal4::IRwGeometryFlagsRuntime&),
        std::uint32_t context) override {
        Check(clump == 0x700 && context == 0, "clump traversal parameters");
        for (auto atomic : {0xA00u, 0xC00u}) {
            Check(callback(atomic, context, *this) == atomic,
                  "atomic callback return");
        }
        return 0x777;
    }
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> reads, writes;
};

void AtomicAndClumpFlags() {
    Runtime runtime;
    runtime.words[0xA18] = 0xB00;
    runtime.words[0xB08] = 0x120;
    runtime.words[0xC18] = 0;
    Check(pal4::RwGeometryFlags::SetAtomicFlags45(0xA00, 0,
                                                  runtime) == 0xA00 &&
          runtime.words[0xB08] == 0x165 &&
          runtime.reads == std::vector<std::uint32_t>{0xA18, 0xB08} &&
          runtime.writes == std::vector<std::uint32_t>{0xB08},
          "atomic geometry flags and return");
    runtime.reads.clear();
    runtime.writes.clear();
    Check(pal4::RwGeometryFlags::SetAtomicFlags45(0xC00, 9,
                                                  runtime) == 0xC00 &&
          runtime.reads == std::vector<std::uint32_t>{0xC18} &&
          runtime.writes.empty(), "missing geometry leaves flags alone");
    runtime.reads.clear();
    Check(pal4::RwGeometryFlags::SetClumpFlags45(0x700, runtime) == 0x777 &&
          runtime.reads == std::vector<std::uint32_t>{0xA18, 0xB08, 0xC18} &&
          runtime.words[0xB08] == 0x165,
          "clump traversal and result forwarding");
}

}  // namespace

int main() { AtomicAndClumpFlags(); }
