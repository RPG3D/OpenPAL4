#include "pal4/effect_visual_record_array.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IEffectVisualRecordArrayRuntime {
    using Record = std::array<std::uint8_t, 36u>;
    static constexpr std::uint32_t kArray = 0x1000u;
    static constexpr std::uint32_t kBegin = 0x2000u;
    static constexpr std::uint32_t kSource = 0x3000u;

    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, Record> records;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;

    Runtime(std::uint32_t size, std::uint32_t capacity) {
        words[kArray + 4u] = kBegin;
        words[kArray + 8u] = kBegin + size * 36u;
        words[kArray + 12u] = kBegin + capacity * 36u;
        for (std::uint32_t i = 0u; i < size; ++i)
            Set(kBegin + i * 36u, static_cast<std::uint8_t>(i + 1u));
        Set(kSource, 9u);
    }

    void Set(std::uint32_t address, std::uint8_t value) {
        Record record{};
        record.fill(value);
        records[address] = record;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    void Copy36(std::uint32_t destination,
                std::uint32_t source) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source));
        records[destination] = records.at(source);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    std::uint8_t At(std::uint32_t index) const {
        const auto begin = words.at(kArray + 4u);
        return records.at(begin + index * 36u).front();
    }
};

void TestInPlaceBranches() {
    Runtime tail_fits(4u, 7u);
    pal4::EffectVisualRecordArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 36u, 2u,
        Runtime::kSource, tail_fits);
    Check(tail_fits.words.at(Runtime::kArray + 8u) ==
              Runtime::kBegin + 6u * 36u &&
              std::vector<std::uint8_t>{
                  tail_fits.At(0u), tail_fits.At(1u), tail_fits.At(2u),
                  tail_fits.At(3u), tail_fits.At(4u), tail_fits.At(5u)} ==
                  std::vector<std::uint8_t>{1u, 9u, 9u, 2u, 3u, 4u},
          "in-place insert shifts a long tail backwards");

    Runtime short_tail(3u, 6u);
    pal4::EffectVisualRecordArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 2u * 36u, 2u,
        Runtime::kSource, short_tail);
    Check(short_tail.words.at(Runtime::kArray + 8u) ==
              Runtime::kBegin + 5u * 36u &&
              std::vector<std::uint8_t>{
                  short_tail.At(0u), short_tail.At(1u), short_tail.At(2u),
                  short_tail.At(3u), short_tail.At(4u)} ==
                  std::vector<std::uint8_t>{1u, 2u, 9u, 9u, 3u},
          "in-place insert extends a short tail and fills the gap");

    Runtime no_op(3u, 5u);
    pal4::EffectVisualRecordArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 36u, 0u,
        Runtime::kSource, no_op);
    Check(no_op.calls.empty() && no_op.words.at(Runtime::kArray + 8u) ==
              Runtime::kBegin + 3u * 36u,
          "zero repeated records leave storage intact");
}

void TestReallocate() {
    Runtime runtime(3u, 3u);
    pal4::EffectVisualRecordArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 36u, 2u,
        Runtime::kSource, runtime);
    Check(runtime.calls.front() == "allocate:216" &&
              runtime.calls.back() == "free:8192" &&
              runtime.words.at(Runtime::kArray + 4u) == 0x4000u &&
              runtime.words.at(Runtime::kArray + 8u) == 0x4000u + 5u * 36u &&
              runtime.words.at(Runtime::kArray + 12u) ==
                  0x4000u + 6u * 36u &&
              std::vector<std::uint8_t>{
                  runtime.At(0u), runtime.At(1u), runtime.At(2u),
                  runtime.At(3u), runtime.At(4u)} ==
                  std::vector<std::uint8_t>{1u, 9u, 9u, 2u, 3u},
          "reallocation grows capacity, copies sequence and frees old block");
}

}  // namespace

int main() {
    TestInPlaceBranches();
    TestReallocate();
}
