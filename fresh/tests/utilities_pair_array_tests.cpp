#include "pal4/utilities_pair_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IUtilitiesPairArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::uint32_t replacement = 0x5000;

    Runtime(std::uint32_t count, std::uint32_t capacity) {
        const auto begin = capacity ? 0x2000u : 0u;
        words[0x1004] = begin;
        words[0x1008] = begin + count * 8;
        words[0x100c] = begin + capacity * 8;
        for (std::uint32_t i = 0; i < count; ++i) {
            words[begin + i * 8] = i + 1;
            words[begin + i * 8 + 4] = 0x100 + i;
        }
        words[0x9000] = 99;
        words[0x9004] = 0x999;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        return replacement;
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
    void CheckPair(std::uint32_t address, std::uint32_t key,
                   std::uint32_t value) const {
        Check(words.at(address) == key &&
                  words.at(address + 4) == value,
              "both DWORDs preserved at physical position");
    }
};

void CheckSpareCapacity() {
    Runtime middle(3, 5);
    Check(pal4::UtilitiesPairArray::Insert(
              0x1000, 0x2008, 0x9000, middle) == 0x2008 &&
              middle.words.at(0x1008) == 0x2020 &&
              middle.calls.back() == Call{"write", 0x1008, 0x2020},
          "middle insertion advances end without allocation");
    middle.CheckPair(0x2000, 1, 0x100);
    middle.CheckPair(0x2008, 99, 0x999);
    middle.CheckPair(0x2010, 2, 0x101);
    middle.CheckPair(0x2018, 3, 0x102);
    Check(middle.calls.size() == 7,
          "middle insertion copies last, shifts tail and writes new pair");

    Runtime append(2, 3);
    Check(pal4::UtilitiesPairArray::Insert(
              0x1000, 0x2010, 0x9000, append) == 0x2010 &&
              append.calls == std::vector<Call>{
                  {"write", 0x2010, 99},
                  {"write", 0x2014, 0x999},
                  {"write", 0x1008, 0x2018}},
          "tail insertion copies one pair directly");
}

void PlainRangeHelpers() {
    Runtime runtime(2, 2);
    Check(pal4::UtilitiesPairArray::CopyRangePlain(
              0x2000u, 0x2010u, 0x3000u, runtime) == 0x3010u,
          "plain range copy returns destination end");
    runtime.CheckPair(0x3000u, 1u, 0x100u);
    runtime.CheckPair(0x3008u, 2u, 0x101u);
    runtime.calls.clear();
    Check(pal4::UtilitiesPairArray::CopyRangePlain(
              0x2000u, 0x2010u, 0u, runtime) == 16u &&
              runtime.calls == std::vector<Call>{
                  {"write", 8u, 2u}, {"write", 12u, 0x101u}},
          "null destination skips first pair then advances");
    Check(pal4::UtilitiesPairArray::CopyRangePlain(
              0x2000u, 0x2000u, 0x3000u, runtime) == 0x3000u,
          "empty plain range preserves destination");

    runtime.calls.clear();
    Check(pal4::UtilitiesPairArray::FillPlain(
              0x4000u, 3u, 0x9000u, runtime) == 0x4018u,
          "plain fill returns end of three records");
    for (std::uint32_t index = 0; index < 3u; ++index)
        runtime.CheckPair(0x4000u + index * 8u, 99u, 0x999u);
    runtime.calls.clear();
    Check(pal4::UtilitiesPairArray::FillPlain(
              0x4000u, 0u, 0x9000u, runtime) == 0u &&
              runtime.calls.empty(), "zero-count fill returns zero");
    Check(pal4::UtilitiesPairArray::FillPlain(
              0u, 2u, 0x9000u, runtime) == 16u &&
              runtime.calls == std::vector<Call>{
                  {"write", 8u, 99u}, {"write", 12u, 0x999u}},
          "null destination skips first fill pair then advances");
}

void CheckGrowth() {
    Runtime middle(3, 3);
    Check(pal4::UtilitiesPairArray::Insert(
              0x1000, 0x2008, 0x9000, middle) == 0x5008 &&
              middle.words.at(0x1004) == 0x5000 &&
              middle.words.at(0x1008) == 0x5020 &&
              middle.words.at(0x100c) == 0x5030 &&
              middle.calls.front() == Call{"allocate", 48} &&
              middle.calls[9] == Call{"free", 0x2000},
          "full array doubles capacity and frees old buffer");
    middle.CheckPair(0x5000, 1, 0x100);
    middle.CheckPair(0x5008, 99, 0x999);
    middle.CheckPair(0x5010, 2, 0x101);
    middle.CheckPair(0x5018, 3, 0x102);

    Runtime empty(0, 0);
    Check(pal4::UtilitiesPairArray::Insert(
              0x1000, 0, 0x9000, empty) == 0x5000 &&
              empty.calls.front() == Call{"allocate", 8} &&
              empty.words.at(0x1004) == 0x5000 &&
              empty.words.at(0x1008) == 0x5008 &&
              empty.words.at(0x100c) == 0x5008,
          "first insertion allocates one 8-byte pair");
    empty.CheckPair(0x5000, 99, 0x999);
}

void SelectUniqueKeepsFirstPairPerKey() {
    Runtime runtime(4, 4);
    runtime.words[0x2000u] = 1u;
    runtime.words[0x2008u] = 1u;
    runtime.words[0x2010u] = 2u;
    runtime.words[0x2018u] = 3u;
    std::vector<std::uint32_t> compared;
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        compared.push_back(previous);
        compared.push_back(candidate);
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesPairArray::SelectUnique(
              0x2000u, 0x2020u, 0x5000u, equals, runtime) == 0x5018u,
          "pair selection returns three-pair output end");
    runtime.CheckPair(0x5000u, 1u, 0x100u);
    runtime.CheckPair(0x5008u, 2u, 0x102u);
    runtime.CheckPair(0x5010u, 3u, 0x103u);
    Check(compared == std::vector<std::uint32_t>{
              0x2000u, 0x2008u, 0x2000u, 0x2010u,
              0x2010u, 0x2018u},
          "pair selection compares each candidate with the last retained pair");
    Check(pal4::UtilitiesPairArray::SelectUnique(
              0x2000u, 0x2000u, 0x5000u, equals, runtime) == 0x5000u &&
              compared.size() == 6u,
          "empty pair selection leaves destination unchanged");
}

void SelectUniqueCompactsPairsInPlace() {
    Runtime runtime(3, 3);
    runtime.words[0x2000u] = 1u;
    runtime.words[0x2008u] = 1u;
    runtime.words[0x2010u] = 2u;
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesPairArray::SelectUnique(
              0x2000u, 0x2018u, 0x2000u, equals, runtime) == 0x2010u,
          "in-place pair selection returns compacted end");
    runtime.CheckPair(0x2000u, 1u, 0x100u);
    runtime.CheckPair(0x2008u, 2u, 0x102u);
}

}  // namespace

int main() {
    try {
        CheckSpareCapacity();
        PlainRangeHelpers();
        CheckGrowth();
        SelectUniqueKeepsFirstPairPerKey();
        SelectUniqueCompactsPairsInPlace();
        std::cout << "utilities_pair_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
