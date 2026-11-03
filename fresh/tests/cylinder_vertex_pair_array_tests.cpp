#include "pal4/cylinder_vertex_pair_array.h"

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

struct Runtime final : pal4::ICylinderVertexPairArrayRuntime {
    using Record = std::array<std::uint8_t, 40u>;
    static constexpr std::uint32_t kArray = 0x1000u;
    static constexpr std::uint32_t kBegin = 0x2000u;
    static constexpr std::uint32_t kSource = 0x3000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, Record> records;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x4000u;

    Runtime(std::uint32_t size, std::uint32_t capacity) {
        words[kArray + 4u] = kBegin;
        words[kArray + 8u] = kBegin + size * 40u;
        words[kArray + 12u] = kBegin + capacity * 40u;
        for (std::uint32_t i = 0u; i < size; ++i)
            Set(kBegin + i * 40u, static_cast<std::uint8_t>(i + 1u));
        Set(kSource, 9u);
    }
    void Set(std::uint32_t address, std::uint8_t value) {
        Record record{};
        record.fill(value);
        record.back() = static_cast<std::uint8_t>(value + 40u);
        records[address] = record;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    void Copy40(std::uint32_t destination,
                std::uint32_t source) override {
        calls.push_back("copy:" + std::to_string(destination) + ":" +
                        std::to_string(source));
        records[destination] = records.at(source);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    Record At(std::uint32_t index) const {
        return records.at(words.at(kArray + 4u) + index * 40u);
    }
};

void CheckSequence(const Runtime& runtime,
                   const std::vector<std::uint8_t>& expected,
                   const char* message) {
    for (std::uint32_t index = 0u; index < expected.size(); ++index) {
        const auto record = runtime.At(index);
        Check(record.front() == expected[index] &&
                  record.back() == expected[index] + 40u,
              message);
    }
}

void TestInPlace() {
    Runtime long_tail(4u, 7u);
    pal4::CylinderVertexPairArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 40u, 2u,
        Runtime::kSource, long_tail);
    Check(long_tail.words.at(Runtime::kArray + 8u) ==
              Runtime::kBegin + 6u * 40u &&
              long_tail.calls.front().starts_with("copy:"),
          "long tail shifts within available capacity");
    CheckSequence(long_tail, {1u, 9u, 9u, 2u, 3u, 4u},
                  "long-tail insertion preserves all forty bytes");

    Runtime short_tail(3u, 6u);
    pal4::CylinderVertexPairArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 2u * 40u, 2u,
        Runtime::kSource, short_tail);
    Check(short_tail.words.at(Runtime::kArray + 8u) ==
              Runtime::kBegin + 5u * 40u,
          "short tail extends end by two pairs");
    CheckSequence(short_tail, {1u, 2u, 9u, 9u, 3u},
                  "short-tail insertion preserves order");

    Runtime zero(3u, 5u);
    pal4::CylinderVertexPairArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 40u, 0u,
        Runtime::kSource, zero);
    Check(zero.calls.empty() &&
              zero.words.at(Runtime::kArray + 8u) ==
                  Runtime::kBegin + 3u * 40u,
          "zero count leaves storage untouched");
}

void TestGrowth() {
    Runtime runtime(3u, 3u);
    pal4::CylinderVertexPairArray::InsertCopies(
        Runtime::kArray, Runtime::kBegin + 40u, 2u,
        Runtime::kSource, runtime);
    Check(runtime.calls.front() == "allocate:240" &&
              runtime.calls.back() == "free:8192" &&
              runtime.words.at(Runtime::kArray + 4u) == 0x4000u &&
              runtime.words.at(Runtime::kArray + 8u) == 0x4000u + 5u * 40u &&
              runtime.words.at(Runtime::kArray + 12u) == 0x4000u + 6u * 40u,
          "growth uses max(old size, inserted count) and frees old storage");
    CheckSequence(runtime, {1u, 9u, 9u, 2u, 3u},
                  "reallocation copies full records in order");
}

}  // namespace

int main() {
    TestInPlace();
    TestGrowth();
}
