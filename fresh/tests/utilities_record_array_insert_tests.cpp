#include "pal4/utilities_preview_info_table.h"
#include "pal4/utilities_text_record_table.h"
#include "pal4/utilities_scene_table.h"
#include "pal4/utilities_window_table.h"
#include "pal4/utilities_world_table.h"

#include <array>
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

struct Event {
    std::string name;
    std::uint32_t first = 0;
    std::uint32_t second = 0;
    std::uint32_t third = 0;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::IUtilitiesRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Event> events;
    std::uint32_t next_allocation = 0x5000;

    Runtime(std::uint32_t stride, std::uint32_t count,
            std::uint32_t capacity) {
        const auto begin = capacity ? 0x2000u : 0u;
        words[0x1004] = begin;
        words[0x1008] = begin + count * stride;
        words[0x100c] = begin + capacity * stride;
        for (std::uint32_t i = 0; i < count; ++i)
            words[begin + i * stride] = i + 1;
        words[0x9000] = 99;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back({"allocate", bytes});
        return next_allocation;
    }
    void Free(std::uint32_t address) override {
        events.push_back({"free", address});
    }
    void CopyConstruct(std::uint32_t to, std::uint32_t from,
                       std::uint32_t stride) override {
        Check(stride == 20 || stride == 40 || stride == 44 ||
                  stride == 52 || stride == 60, "record width");
        words[to] = words.at(from);
        events.push_back({"copy", to, from, stride});
    }
    void Assign(std::uint32_t to, std::uint32_t from,
                std::uint32_t stride) override {
        words[to] = words.at(from);
        events.push_back({"assign", to, from, stride});
    }
    void MoveBackward(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t destination_end,
                      std::uint32_t stride) override {
        events.push_back({"move", begin, end, destination_end});
        const auto displacement = destination_end - end;
        for (auto from = end; from != begin;) {
            from -= stride;
            words[from + displacement] = words.at(from);
        }
    }
    void DestroyRange(std::uint32_t begin, std::uint32_t end,
                      std::uint32_t stride) override {
        events.push_back({"destroy", begin, end, stride});
    }
};

using InsertFn = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IUtilitiesRecordArrayRuntime&);
using CountFn = std::uint32_t (*)(
    std::uint32_t, pal4::IUtilitiesRecordArrayRuntime&);
using CopyFn = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    pal4::IUtilitiesRecordArrayRuntime&);
using InsertCopiesFn = void (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, pal4::IUtilitiesRecordArrayRuntime&);
using FillFn = void (*)(std::uint32_t, std::uint32_t,
                       std::uint32_t,
                       pal4::IUtilitiesRecordArrayRuntime&);

struct Operations {
    std::uint32_t stride;
    InsertFn insert;
    CountFn count;
    CopyFn copy;
    InsertCopiesFn insert_copies;
    FillFn fill;
};

constexpr std::array<Operations, 5> kOperations{{
    {20, &pal4::UtilitiesTextRecordArray::Insert,
     &pal4::UtilitiesTextRecordArray::Count,
     &pal4::UtilitiesTextRecordArray::CopyRange,
     &pal4::UtilitiesTextRecordArray::InsertCopies,
     &pal4::UtilitiesTextRecordArray::Fill},
    {52, &pal4::UtilitiesPreviewInfoArray::Insert,
     &pal4::UtilitiesPreviewInfoArray::Count,
     &pal4::UtilitiesPreviewInfoArray::CopyRange,
     &pal4::UtilitiesPreviewInfoArray::InsertCopies,
     &pal4::UtilitiesPreviewInfoArray::Fill},
    {60, &pal4::UtilitiesSceneArray::Insert, nullptr,
     &pal4::UtilitiesSceneArray::CopyRange,
     &pal4::UtilitiesSceneArray::InsertCopies,
     &pal4::UtilitiesSceneArray::Fill},
    {44, &pal4::UtilitiesWindowArray::Insert, nullptr,
     &pal4::UtilitiesWindowArray::CopyRange,
     &pal4::UtilitiesWindowArray::InsertCopies,
     &pal4::UtilitiesWindowArray::Fill},
    {40, &pal4::UtilitiesWorldArray::Insert, nullptr,
     &pal4::UtilitiesWorldArray::CopyRange,
     &pal4::UtilitiesWorldArray::InsertCopies,
     &pal4::UtilitiesWorldArray::Fill},
}};

void CheckCountAndCopy(const Operations& operation) {
    const auto stride = operation.stride;
    Runtime runtime(stride, 3, 4);
    if (operation.count)
        Check(operation.count(0x1000, runtime) == 3,
              "active record count uses the class stride");
    Check(operation.copy(0x2000, 0x2000 + 2 * stride,
                         0x6000, runtime) == 0x6000 + 2 * stride &&
              runtime.words.at(0x6000) == 1 &&
              runtime.words.at(0x6000 + stride) == 2 &&
              runtime.events == std::vector<Event>{
                  {"copy", 0x6000, 0x2000, stride},
                  {"copy", 0x6000 + stride, 0x2000 + stride,
                   stride}},
          "half-open range copies each record and returns next destination");
    runtime.events.clear();
    Check(operation.copy(0x2000, 0x2000,
                         0x7000, runtime) == 0x7000 &&
              runtime.events.empty(), "empty copy range is a no-op");

    Runtime empty(stride, 0, 0);
    if (operation.count)
        Check(operation.count(0x1000, empty) == 0,
              "null begin reports zero records");
}

void CheckInPlace(const Operations& operation) {
    const auto stride = operation.stride;
    Runtime middle(stride, 3, 5);
    Check(operation.insert(0x1000, 0x2000 + stride,
                           0x9000, middle) == 0x2000 + stride &&
              middle.words.at(0x1008) == 0x2000 + 4 * stride &&
              middle.words.at(0x2000) == 1 &&
              middle.words.at(0x2000 + stride) == 99 &&
              middle.words.at(0x2000 + 2 * stride) == 2 &&
              middle.words.at(0x2000 + 3 * stride) == 3,
          "middle insertion shifts live records without reallocating");
    Check(middle.events == std::vector<Event>{
              {"copy", 0x2000 + 3 * stride,
               0x2000 + 2 * stride, stride},
              {"move", 0x2000 + stride, 0x2000 + 2 * stride,
               0x2000 + 3 * stride},
              {"assign", 0x2000 + stride, 0x9000, stride},
              {"write", 0x1008, 0x2000 + 4 * stride}},
          "in-place copy, reverse move, assignment and end update order");

    Runtime append(stride, 2, 3);
    Check(operation.insert(0x1000, 0x2000 + 2 * stride,
                           0x9000, append) == 0x2000 + 2 * stride &&
              append.events == std::vector<Event>{
                  {"copy", 0x2000 + 2 * stride, 0x9000, stride},
                  {"write", 0x1008, 0x2000 + 3 * stride}},
          "tail insertion constructs directly into spare slot");
}

void CheckGrowth(const Operations& operation) {
    const auto stride = operation.stride;
    Runtime middle(stride, 3, 3);
    Check(operation.insert(0x1000, 0x2000 + stride,
                           0x9000, middle) == 0x5000 + stride &&
              middle.words.at(0x1004) == 0x5000 &&
              middle.words.at(0x1008) == 0x5000 + 4 * stride &&
              middle.words.at(0x100c) == 0x5000 + 6 * stride &&
              middle.words.at(0x5000) == 1 &&
              middle.words.at(0x5000 + stride) == 99 &&
              middle.words.at(0x5000 + 2 * stride) == 2 &&
              middle.words.at(0x5000 + 3 * stride) == 3,
          "growth doubles capacity and retains insertion position");
    Check(middle.events == std::vector<Event>{
              {"allocate", 6 * stride},
              {"copy", 0x5000, 0x2000, stride},
              {"copy", 0x5000 + stride, 0x9000, stride},
              {"copy", 0x5000 + 2 * stride,
               0x2000 + stride, stride},
              {"copy", 0x5000 + 3 * stride,
               0x2000 + 2 * stride, stride},
              {"destroy", 0x2000, 0x2000 + 3 * stride, stride},
              {"free", 0x2000},
              {"write", 0x100c, 0x5000 + 6 * stride},
              {"write", 0x1004, 0x5000},
              {"write", 0x1008, 0x5000 + 4 * stride}},
          "growth copies before destroying and freeing old records");

    Runtime empty(stride, 0, 0);
    Check(operation.insert(0x1000, 0, 0x9000, empty) ==
              0x5000 &&
              empty.words.at(0x1004) == 0x5000 &&
              empty.words.at(0x1008) == 0x5000 + stride &&
              empty.words.at(0x100c) == 0x5000 + stride &&
              empty.words.at(0x5000) == 99 &&
              empty.events.front() == Event{"allocate", stride},
          "first insertion allocates one record");
}

void CheckBatch(const Operations& operation) {
    const auto stride = operation.stride;
    Runtime long_tail(stride, 4, 6);
    operation.insert_copies(0x1000, 0x2000 + stride,
                            2, 0x9000, long_tail);
    Check(long_tail.words.at(0x1008) == 0x2000 + 6 * stride &&
              long_tail.words.at(0x2000) == 1 &&
              long_tail.words.at(0x2000 + stride) == 99 &&
              long_tail.words.at(0x2000 + 2 * stride) == 99 &&
              long_tail.words.at(0x2000 + 3 * stride) == 2 &&
              long_tail.words.at(0x2000 + 4 * stride) == 3 &&
              long_tail.words.at(0x2000 + 5 * stride) == 4,
          "batch insertion with a long tail shifts and fills correctly");
    Check(long_tail.events[0] == Event{"copy", 0x2000 + 4 * stride,
                                       0x2000 + 2 * stride, stride} &&
              long_tail.events[2] == Event{"move", 0x2000 + stride,
                                           0x2000 + 2 * stride,
                                           0x2000 + 4 * stride},
          "long-tail branch copy-constructs last items before reverse move");

    Runtime short_tail(stride, 3, 5);
    operation.insert_copies(0x1000, 0x2000 + 2 * stride,
                            2, 0x9000, short_tail);
    Check(short_tail.words.at(0x1008) == 0x2000 + 5 * stride &&
              short_tail.words.at(0x2000 + 2 * stride) == 99 &&
              short_tail.words.at(0x2000 + 3 * stride) == 99 &&
              short_tail.words.at(0x2000 + 4 * stride) == 3 &&
              short_tail.events == std::vector<Event>{
                  {"copy", 0x2000 + 4 * stride,
                   0x2000 + 2 * stride, stride},
                  {"copy", 0x2000 + 3 * stride, 0x9000, stride},
                  {"assign", 0x2000 + 2 * stride, 0x9000, stride},
                  {"write", 0x1008, 0x2000 + 5 * stride}},
          "short-tail branch constructs beyond end and assigns old tail");

    Runtime grown(stride, 3, 3);
    operation.insert_copies(0x1000, 0x2000 + stride,
                            2, 0x9000, grown);
    Check(grown.words.at(0x1004) == 0x5000 &&
              grown.words.at(0x1008) == 0x5000 + 5 * stride &&
              grown.words.at(0x100c) == 0x5000 + 6 * stride &&
              grown.words.at(0x5000) == 1 &&
              grown.words.at(0x5000 + stride) == 99 &&
              grown.words.at(0x5000 + 2 * stride) == 99 &&
              grown.words.at(0x5000 + 3 * stride) == 2 &&
              grown.words.at(0x5000 + 4 * stride) == 3 &&
              grown.events.front() == Event{"allocate", 6 * stride} &&
              grown.events[6] == Event{"destroy", 0x2000,
                                        0x2000 + 3 * stride, stride} &&
              grown.events[7] == Event{"free", 0x2000},
          "batch growth copies values then destroys old array");

    Runtime empty(stride, 0, 0);
    operation.insert_copies(0x1000, 0, 2, 0x9000, empty);
    Check(empty.words.at(0x1004) == 0x5000 &&
              empty.words.at(0x1008) == 0x5000 + 2 * stride &&
              empty.words.at(0x100c) == 0x5000 + 2 * stride &&
              empty.words.at(0x5000) == 99 &&
              empty.words.at(0x5000 + stride) == 99,
          "empty batch allocates requested count");

    Runtime zero(stride, 3, 3);
    operation.insert_copies(0x1000, 0x2000 + stride,
                            0, 0x9000, zero);
    Check(zero.events.empty() &&
              zero.words.at(0x1008) == 0x2000 + 3 * stride,
          "zero-count insertion does not change array");
}

void CheckFill(const Operations& operation) {
    const auto stride = operation.stride;
    Runtime runtime(stride, 0, 0);
    operation.fill(0x6000, 3, 0x9000, runtime);
    Check(runtime.events == std::vector<Event>{
              {"copy", 0x6000, 0x9000, stride},
              {"copy", 0x6000 + stride, 0x9000, stride},
              {"copy", 0x6000 + 2 * stride, 0x9000, stride}} &&
              runtime.words.at(0x6000) == 99 &&
              runtime.words.at(0x6000 + stride) == 99 &&
              runtime.words.at(0x6000 + 2 * stride) == 99,
          "fill constructs each record at the class-specific stride");
    runtime.events.clear();
    operation.fill(0x7000, 0, 0x9000, runtime);
    Check(runtime.events.empty(), "zero-count fill is a no-op");
}

}  // namespace

int main() {
    try {
        for (const auto& operation : kOperations) {
            CheckCountAndCopy(operation);
            CheckInPlace(operation);
            CheckGrowth(operation);
            CheckBatch(operation);
            CheckFill(operation);
        }
        std::cout << "utilities_record_array_insert_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
