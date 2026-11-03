#include "pal4/ai_action_pointer_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kArray = 0x4000u;
constexpr std::uint32_t kSource = 0x5000u;
constexpr std::uint32_t kOld = 0x6000u;
constexpr std::uint32_t kNew = 0x7000u;

struct Runtime final : pal4::IAiActionPointerArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kArray + 4u, 0u}, {kArray + 8u, 0u},
        {kArray + 12u, 0u}, {kSource, 0xabcu}};
    std::uint32_t allocation = kNew;
    std::uint32_t allocated_bytes = 0u;
    std::vector<std::uint32_t> freed;

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(words.count(address) == 1u,
              "pointer-array read uses initialized word");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        allocated_bytes = bytes;
        return allocation;
    }
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
};

void EmptyAndCapacity() {
    Runtime runtime;
    Check(pal4::AiActionPointerArray::Capacity(
              kArray, runtime) == 0u,
          "null begin reports zero capacity");
    Check(pal4::AiActionPointerArray::InsertOne(
              kArray, 0u, kSource, runtime) == kNew &&
          runtime.allocated_bytes == 4u &&
          runtime.words[kNew] == 0xabcu &&
          runtime.words[kArray + 4u] == kNew &&
          runtime.words[kArray + 8u] == kNew + 4u &&
          runtime.words[kArray + 12u] == kNew + 4u &&
          runtime.freed == std::vector<std::uint32_t>{0u} &&
          pal4::AiActionPointerArray::Capacity(
              kArray, runtime) == 1u,
          "empty insert allocates one 4-byte pointer and updates header");
}

void SpareCapacity() {
    Runtime runtime;
    runtime.words[kArray + 4u] = kOld;
    runtime.words[kArray + 8u] = kOld + 8u;
    runtime.words[kArray + 12u] = kOld + 16u;
    runtime.words[kOld] = 0x11u;
    runtime.words[kOld + 4u] = 0x22u;
    Check(pal4::AiActionPointerArray::InsertOne(
              kArray, kOld + 8u, kSource, runtime) ==
              kOld + 8u &&
          runtime.words[kOld + 8u] == 0xabcu &&
          runtime.words[kArray + 8u] == kOld + 12u &&
          runtime.allocated_bytes == 0u,
          "append into spare capacity does not allocate");

    runtime.words[kSource] = 0x33u;
    Check(pal4::AiActionPointerArray::InsertOne(
              kArray, kOld + 4u, kSource, runtime) ==
              kOld + 4u &&
          runtime.words[kOld] == 0x11u &&
          runtime.words[kOld + 4u] == 0x33u &&
          runtime.words[kOld + 8u] == 0x22u &&
          runtime.words[kOld + 12u] == 0xabcu &&
          runtime.words[kArray + 8u] == kOld + 16u,
          "middle insert shifts the tail backward in place");
}

void GrowthAndFailure() {
    Runtime runtime;
    runtime.words[kArray + 4u] = kOld;
    runtime.words[kArray + 8u] = kOld + 8u;
    runtime.words[kArray + 12u] = kOld + 8u;
    runtime.words[kOld] = 0x11u;
    runtime.words[kOld + 4u] = 0x22u;
    Check(pal4::AiActionPointerArray::InsertOne(
              kArray, kOld + 4u, kSource, runtime) ==
              kNew + 4u &&
          runtime.allocated_bytes == 16u &&
          runtime.words[kNew] == 0x11u &&
          runtime.words[kNew + 4u] == 0xabcu &&
          runtime.words[kNew + 8u] == 0x22u &&
          runtime.words[kArray + 4u] == kNew &&
          runtime.words[kArray + 8u] == kNew + 12u &&
          runtime.words[kArray + 12u] == kNew + 16u &&
          runtime.freed == std::vector<std::uint32_t>{kOld},
          "full two-element array grows to four slots");

    runtime = Runtime{};
    runtime.allocation = 0u;
    bool threw = false;
    try {
        (void)pal4::AiActionPointerArray::InsertOne(
            kArray, 0u, kSource, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.words[kArray + 4u] == 0u &&
          runtime.words[kArray + 8u] == 0u &&
          runtime.freed.empty(),
          "allocation failure leaves array header unchanged");
}

}  // namespace

int main() {
    try {
        EmptyAndCapacity();
        SpareCapacity();
        GrowthAndFailure();
        std::cout << "ai_action_pointer_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
