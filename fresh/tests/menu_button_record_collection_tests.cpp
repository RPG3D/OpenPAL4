#include "pal4/menu_button_record_collection.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

constexpr std::uint32_t kCollection = 0x1000u;
constexpr std::uint32_t kSource = 0xb000u;
constexpr std::uint32_t kStride = 164u;

struct Runtime final : pal4::IMenuButtonRecordRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, int> records{{kSource, 99}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> cleaned;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocation = 0xa000u;
    std::uint32_t allocation_bytes = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate");
        allocation_bytes = bytes;
        return allocation;
    }
    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.emplace_back("copy");
        records[destination] = records.at(source);
    }
    void Assign(std::uint32_t destination,
                std::uint32_t source) override {
        calls.emplace_back("assign");
        records[destination] = records.at(source);
    }
    void Cleanup(std::uint32_t record) override {
        calls.emplace_back("cleanup");
        cleaned.push_back(record);
    }
    void Free(std::uint32_t pointer) override {
        calls.emplace_back("free");
        freed.push_back(pointer);
    }
};

void Seed(Runtime& runtime, std::uint32_t begin,
          std::uint32_t count, std::uint32_t capacity) {
    runtime.words[kCollection + 4u] = begin;
    runtime.words[kCollection + 8u] =
        begin + count * kStride;
    runtime.words[kCollection + 12u] =
        begin + capacity * kStride;
    for (std::uint32_t index = 0u; index < count; ++index)
        runtime.records[begin + index * kStride] =
            static_cast<int>(index + 1u);
}

void CheckRecords(const Runtime& runtime, std::uint32_t begin,
                  const std::vector<int>& expected) {
    for (std::size_t index = 0; index < expected.size(); ++index) {
        Check(runtime.records.at(begin +
                                 static_cast<std::uint32_t>(index) *
                                     kStride) == expected[index],
              "record order and ownership");
    }
}

void CheckClear(bool populated) {
    Runtime runtime;
    const auto begin = populated ? 0x5000u : 0u;
    Seed(runtime, begin, populated ? 2u : 0u,
         populated ? 3u : 0u);
    Check(pal4::MenuButtonRecordCollection::Clear(
              kCollection, runtime) == 0u,
          "clear return");
    Check(runtime.cleaned ==
              (populated
                   ? std::vector<std::uint32_t>{
                         0x5000u, 0x5000u + kStride}
                   : std::vector<std::uint32_t>{}) &&
          runtime.freed == std::vector<std::uint32_t>{begin} &&
          runtime.words.at(kCollection + 4u) == 0u &&
          runtime.words.at(kCollection + 8u) == 0u &&
          runtime.words.at(kCollection + 12u) == 0u,
          "record cleanup and pointer reset");
}

void CheckInPlaceLongTail() {
    Runtime runtime;
    Seed(runtime, 0x5000u, 3u, 4u);
    pal4::MenuButtonRecordCollection::Insert(
        kCollection, 0x5000u + kStride, 1u, kSource, runtime);
    CheckRecords(runtime, 0x5000u, {1, 99, 2, 3});
    Check(runtime.words.at(kCollection + 8u) ==
              0x5000u + 4u * kStride &&
          runtime.freed.empty() && runtime.cleaned.empty() &&
          runtime.calls[runtime.calls.size() - 2u] == "assign",
          "in-place backward shift");
}

void CheckInPlaceShortTail() {
    Runtime runtime;
    Seed(runtime, 0x5000u, 3u, 5u);
    pal4::MenuButtonRecordCollection::Insert(
        kCollection, 0x5000u + 2u * kStride,
        2u, kSource, runtime);
    CheckRecords(runtime, 0x5000u, {1, 2, 99, 99, 3});
    Check(runtime.words.at(kCollection + 8u) ==
              0x5000u + 5u * kStride &&
          runtime.freed.empty() && runtime.cleaned.empty(),
          "in-place tail copy and fill");
}

void CheckReallocation(bool empty) {
    Runtime runtime;
    const auto begin = empty ? 0u : 0x5000u;
    Seed(runtime, begin, empty ? 0u : 3u,
         empty ? 0u : 3u);
    const auto insertion = empty ? 0u : 0x5000u + kStride;
    pal4::MenuButtonRecordCollection::Insert(
        kCollection, insertion, empty ? 1u : 2u,
        kSource, runtime);
    CheckRecords(runtime, 0xa000u,
                 empty ? std::vector<int>{99}
                       : std::vector<int>{1, 99, 99, 2, 3});
    Check(runtime.allocation_bytes ==
              (empty ? kStride : 6u * kStride) &&
          runtime.words.at(kCollection + 4u) == 0xa000u &&
          runtime.words.at(kCollection + 8u) ==
              0xa000u + (empty ? 1u : 5u) * kStride &&
          runtime.words.at(kCollection + 12u) ==
              0xa000u + (empty ? 1u : 6u) * kStride &&
          runtime.freed == std::vector<std::uint32_t>{begin},
          "capacity growth and pointer replacement");
    Check(runtime.cleaned ==
              (empty
                   ? std::vector<std::uint32_t>{}
                   : std::vector<std::uint32_t>{
                         0x5000u, 0x5000u + kStride,
                         0x5000u + 2u * kStride}),
          "old record string cleanup");
}

void CheckZeroCount() {
    Runtime runtime;
    Seed(runtime, 0x5000u, 1u, 2u);
    pal4::MenuButtonRecordCollection::Insert(
        kCollection, 0x5000u + kStride, 0u,
        kSource, runtime);
    CheckRecords(runtime, 0x5000u, {1});
    Check(runtime.words.at(kCollection + 8u) ==
              0x5000u + kStride && runtime.freed.empty(),
          "zero-count insertion");
}

}  // namespace

int main() {
    try {
        CheckClear(false);
        CheckClear(true);
        CheckInPlaceLongTail();
        CheckInPlaceShortTail();
        CheckReallocation(true);
        CheckReallocation(false);
        CheckZeroCount();
        std::cout << "menu_button_record_collection_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
