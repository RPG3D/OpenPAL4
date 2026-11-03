#include "pal4/smith_window_word_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowWordArrayRuntime {
    static constexpr std::uint32_t adapter = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t next_allocation = 0x9000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateBytes(std::uint32_t size) override {
        allocations.push_back(size);
        return next_allocation;
    }
    void FreeBytes(std::uint32_t address) override {
        frees.push_back(address);
    }
};

void CheckInPlaceAndClear() {
    Runtime runtime;
    runtime.words = {
        {Runtime::adapter + 4u, 0x8000u},
        {Runtime::adapter + 8u, 0x8008u},
        {Runtime::adapter + 12u, 0x800cu},
        {0x8000u, 11u}, {0x8004u, 22u}};
    pal4::SmithWindowWordArray::Clear(Runtime::adapter, runtime);
    Check(runtime.words.at(Runtime::adapter + 8u) == 0x8000u &&
              runtime.words.at(Runtime::adapter + 12u) == 0x800cu,
          "clear resets end but retains capacity");
    pal4::SmithWindowWordArray::Append(Runtime::adapter, 33u, runtime);
    pal4::SmithWindowWordArray::Append(Runtime::adapter, 44u, runtime);
    Check(runtime.words.at(0x8000u) == 33u &&
              runtime.words.at(0x8004u) == 44u &&
              runtime.words.at(Runtime::adapter + 8u) == 0x8008u &&
              runtime.allocations.empty(),
          "append uses existing capacity after clear");
}

void CheckGrowth() {
    Runtime runtime;
    runtime.words = {
        {Runtime::adapter + 4u, 0x8000u},
        {Runtime::adapter + 8u, 0x8008u},
        {Runtime::adapter + 12u, 0x8008u},
        {0x8000u, 11u}, {0x8004u, 22u}};
    pal4::SmithWindowWordArray::Append(Runtime::adapter, 33u, runtime);
    Check(runtime.allocations == (std::vector<std::uint32_t>{16u}) &&
              runtime.frees == (std::vector<std::uint32_t>{0x8000u}) &&
              runtime.words.at(0x9000u) == 11u &&
              runtime.words.at(0x9004u) == 22u &&
              runtime.words.at(0x9008u) == 33u &&
              runtime.words.at(Runtime::adapter + 4u) == 0x9000u &&
              runtime.words.at(Runtime::adapter + 8u) == 0x900cu &&
              runtime.words.at(Runtime::adapter + 12u) == 0x9010u,
          "full two-element array doubles capacity and preserves values");

    Runtime empty;
    empty.words = {
        {Runtime::adapter + 4u, 0u},
        {Runtime::adapter + 8u, 0u},
        {Runtime::adapter + 12u, 0u}};
    pal4::SmithWindowWordArray::Append(Runtime::adapter, 7u, empty);
    Check(empty.allocations == (std::vector<std::uint32_t>{4u}) &&
              empty.words.at(0x9000u) == 7u &&
              empty.words.at(Runtime::adapter + 8u) == 0x9004u,
          "empty array starts with one slot");
}

void CheckAllocationFailure() {
    Runtime runtime;
    runtime.words = {
        {Runtime::adapter + 4u, 0u},
        {Runtime::adapter + 8u, 0u},
        {Runtime::adapter + 12u, 0u}};
    runtime.next_allocation = 0u;
    bool threw = false;
    try {
        pal4::SmithWindowWordArray::Append(Runtime::adapter,
                                            9u, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.allocations ==
                       (std::vector<std::uint32_t>{4u}) &&
              runtime.words.at(Runtime::adapter + 4u) == 0u &&
              runtime.words.at(Runtime::adapter + 8u) == 0u &&
              runtime.frees.empty(),
          "failed allocation preserves array header");
}

}  // namespace

int main() {
    try {
        CheckInPlaceAndClear();
        CheckGrowth();
        CheckAllocationFailure();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
