#include "pal4/dword_collection_span.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::DwordCollectionSpanRuntimeStub {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
};

void TestSize() {
    pal4::DwordCollectionSpanState state;
    state.end = 0x1010u;
    Check(pal4::DwordCollectionSpan::Size(state) == 0,
          "null begin forces zero size");
    state.begin = 0x1000u;
    Check(pal4::DwordCollectionSpan::Size(state) == 4,
          "size divides byte span by four");
    state.end = 0x0ffcu;
    Check(pal4::DwordCollectionSpan::Size(state) == -1,
          "size uses arithmetic right shift for negative span");
    state.end = 0x1003u;
    Check(pal4::DwordCollectionSpan::Size(state) == 0,
          "partial positive word is truncated");
    state.end = 0x0fffu;
    Check(pal4::DwordCollectionSpan::Size(state) == -1,
          "partial negative word rounds down by arithmetic shift");
}

void TestCopyAndEndPointer() {
    pal4::DwordCollectionSpanState state;
    state.begin = 0x6000u;
    state.end = 0x600cu;
    state.capacity = 0x6020u;
    Runtime runtime;
    runtime.words = {{0x6004u, 11u}, {0x6008u, 22u}};
    Check(pal4::DwordCollectionSpan::CopyTo(
              state, 0x7000u, 0x6004u, runtime) == 0x7000u,
          "copy returns original destination");
    Check(runtime.words.at(0x7000u) == 11u &&
              runtime.words.at(0x7004u) == 22u &&
              state.end == 0x7008u && state.begin == 0x6000u &&
              state.capacity == 0x6020u,
          "copy updates only end pointer and destination words");
    Check(runtime.calls == std::vector<std::string>{
                               "read:24580", "write:28672",
                               "read:24584", "write:28676"},
          "copy reads then writes each element in source order");
}

void TestEmptyCopy() {
    pal4::DwordCollectionSpanState state;
    state.end = 0x6000u;
    Runtime runtime;
    Check(pal4::DwordCollectionSpan::CopyTo(
              state, 0x7000u, 0x6000u, runtime) == 0x7000u &&
              state.end == 0x7000u && runtime.calls.empty(),
          "empty range moves end to destination without memory access");
}

}  // namespace

int main() {
    TestSize();
    TestCopyAndEndPointer();
    TestEmptyCopy();
}
