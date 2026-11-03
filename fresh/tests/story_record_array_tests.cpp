#include "pal4/story_record_array.h"
#include "pal4/story_desc_record_array.h"

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

struct Call {
    std::string name;
    std::uint32_t first;
    std::uint32_t second = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IStoryRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;
    bool fail_copy = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write", address, value});
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        Check(next_allocation < allocations.size(), "unexpected allocation");
        return allocations[next_allocation++];
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"copy", destination, source});
        if (fail_copy) throw std::runtime_error("copy failed");
        words[destination] = words[source];
    }
    void AssignRecord(std::uint32_t destination,
                      std::uint32_t source) override {
        calls.push_back({"assign", destination, source});
        words[destination] = words[source];
    }
    void DestroyRecord(std::uint32_t record) override {
        calls.push_back({"destroy", record});
    }
    void Free(std::uint32_t address) override {
        calls.push_back({"free", address});
    }
};

constexpr auto array = 0x7000u;
constexpr auto source = 0x5000u;

void CheckInPlaceInsertion() {
    Runtime middle;
    middle.words[array + 4] = 0x1000;
    middle.words[array + 8] = 0x106c;
    middle.words[array + 12] = 0x1090;
    middle.words[0x1000] = 10;
    middle.words[0x1024] = 20;
    middle.words[0x1048] = 30;
    middle.words[source] = 99;
    Check(pal4::StoryRecordArray::Insert(array, 0x1024, source, middle) ==
              0x1024 &&
              middle.calls == std::vector<Call>{
                  {"copy", 0x106c, 0x1048},
                  {"assign", 0x1048, 0x1024},
                  {"assign", 0x1024, source},
                  {"write", array + 8, 0x1090}} &&
              middle.words[0x1024] == 99 &&
              middle.words[0x1048] == 20 &&
              middle.words[0x106c] == 30,
          "spare capacity shifts records backward before assignment");

    Runtime append;
    append.words[array + 4] = 0x1000;
    append.words[array + 8] = 0x1024;
    append.words[array + 12] = 0x1048;
    append.words[source] = 42;
    Check(pal4::StoryRecordArray::Insert(array, 0x1024, source, append) ==
              0x1024 &&
              append.calls == std::vector<Call>{
                  {"copy", 0x1024, source},
                  {"write", array + 8, 0x1048}} &&
              append.words[0x1024] == 42,
          "append with spare capacity copy-constructs one record");
}

void CheckGrowth() {
    Runtime grow;
    grow.allocations = {0x3000};
    grow.words[array + 4] = 0x1000;
    grow.words[array + 8] = 0x1048;
    grow.words[array + 12] = 0x1048;
    grow.words[0x1000] = 10;
    grow.words[0x1024] = 20;
    grow.words[source] = 99;
    Check(pal4::StoryRecordArray::Insert(array, 0x1024, source, grow) ==
              0x3024 &&
              grow.calls == std::vector<Call>{
                  {"allocate", 144},
                  {"copy", 0x3000, 0x1000},
                  {"copy", 0x3024, source},
                  {"copy", 0x3048, 0x1024},
                  {"destroy", 0x1000},
                  {"destroy", 0x1024},
                  {"free", 0x1000},
                  {"write", array + 12, 0x3090},
                  {"write", array + 4, 0x3000},
                  {"write", array + 8, 0x306c}} &&
              grow.words[0x3000] == 10 &&
              grow.words[0x3024] == 99 &&
              grow.words[0x3048] == 20,
          "full array doubles capacity and preserves insertion position");

    Runtime empty;
    empty.allocations = {0x3000};
    empty.words[source] = 7;
    Check(pal4::StoryRecordArray::Insert(array, 0, source, empty) ==
              0x3000 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 36},
                  {"copy", 0x3000, source},
                  {"free", 0},
                  {"write", array + 12, 0x3024},
                  {"write", array + 4, 0x3000},
                  {"write", array + 8, 0x3024}},
          "empty array allocates one 36-byte slot");
}

void CheckCopyFailure() {
    Runtime failed;
    failed.allocations = {0x3000};
    failed.words[array + 4] = 0x1000;
    failed.words[array + 8] = 0x1024;
    failed.words[array + 12] = 0x1024;
    failed.fail_copy = true;
    try {
        static_cast<void>(pal4::StoryRecordArray::Insert(
            array, 0x1000, source, failed));
        Check(false, "copy failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "copy failed" &&
                  failed.calls == std::vector<Call>{
                      {"allocate", 72}, {"copy", 0x3000, source}} &&
                  failed.words[array + 4] == 0x1000 &&
                  failed.words[array + 8] == 0x1024,
              "copy failure does not commit new array pointers");
    }
}

void CheckCopyRange() {
    Runtime copied;
    copied.words[0x1000] = 10;
    copied.words[0x1024] = 20;
    Check(pal4::StoryRecordArray::CopyRange(
              0x1000, 0x1048, 0x3000, copied) == 0x3048 &&
              copied.calls == std::vector<Call>{
                  {"copy", 0x3000, 0x1000},
                  {"copy", 0x3024, 0x1024}} &&
              copied.words[0x3000] == 10 &&
              copied.words[0x3024] == 20,
          "range copy uses 36-byte stride and returns destination end");
    Check(pal4::StoryRecordArray::CopyRange(
              0x1000, 0x1000, 0x4000, copied) == 0x4000 &&
              copied.calls.size() == 2,
          "empty range returns untouched destination");
}

void CheckStoryDescInsertion() {
    Runtime append;
    append.words[array + 4] = 0x1000;
    append.words[array + 8] = 0x1024;
    append.words[array + 12] = 0x1048;
    append.words[source] = 42;
    Check(pal4::StoryDescRecordArray::Insert(
              array, 0x1024, source, append) == 0x1024 &&
              append.calls == std::vector<Call>{
                  {"copy", 0x1024, source},
                  {"write", array + 8, 0x1048}},
          "StoryDesc spare-capacity append uses its injected record copy");

    Runtime grow;
    grow.allocations = {0x3000};
    grow.words[array + 4] = 0x1000;
    grow.words[array + 8] = 0x1024;
    grow.words[array + 12] = 0x1024;
    grow.words[0x1000] = 10;
    grow.words[source] = 99;
    Check(pal4::StoryDescRecordArray::Insert(
              array, 0x1000, source, grow) == 0x3000 &&
              grow.calls == std::vector<Call>{
                  {"allocate", 72},
                  {"copy", 0x3000, source},
                  {"copy", 0x3024, 0x1000},
                  {"destroy", 0x1000},
                  {"free", 0x1000},
                  {"write", array + 12, 0x3048},
                  {"write", array + 4, 0x3000},
                  {"write", array + 8, 0x3048}},
          "StoryDesc growth preserves its prefix and suffix order");
}

void CheckSizeAndStoryDescRange() {
    Runtime runtime;
    Check(pal4::StoryRecordArray::Size(array, runtime) == 0,
          "null 36-byte array has zero active elements");
    runtime.words[array + 4] = 0x1000;
    runtime.words[array + 8] = 0x106c;
    Check(pal4::StoryRecordArray::Size(array, runtime) == 3,
          "36-byte array size uses active end");
    runtime.words[0x1000] = 10;
    runtime.words[0x1024] = 20;
    Check(pal4::StoryDescRecordArray::CopyRange(
              0x1000, 0x1048, 0x3000, runtime) == 0x3048 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x3000, 0x1000},
                  {"copy", 0x3024, 0x1024}},
          "StoryDesc range copy uses its injected record copy at 36-byte stride");
    Check(pal4::StoryDescRecordArray::CopyRange(
              0x1000, 0x1000, 0x4000, runtime) == 0x4000,
          "empty StoryDesc range returns destination unchanged");
}

void CheckStoryInsertCopies() {
    Runtime shift;
    shift.words[array + 4] = 0x1000;
    shift.words[array + 8] = 0x1090;
    shift.words[array + 12] = 0x10d8;
    shift.words[0x1000] = 10;
    shift.words[0x1024] = 20;
    shift.words[0x1048] = 30;
    shift.words[0x106c] = 40;
    shift.words[source] = 99;
    pal4::StoryRecordArray::InsertCopies(array, 0x1024, 2, source, shift);
    Check(shift.calls == std::vector<Call>{
              {"copy", 0x1090, 0x1048},
              {"copy", 0x10b4, 0x106c},
              {"assign", 0x106c, 0x1024},
              {"assign", 0x1024, source},
              {"assign", 0x1048, source},
              {"write", array + 8, 0x10d8}} &&
              shift.words[0x106c] == 20 &&
              shift.words[0x1090] == 30 &&
              shift.words[0x10b4] == 40,
          "sufficient tail copies last rows then shifts earlier rows backward");

    Runtime short_tail;
    short_tail.words[array + 4] = 0x1000;
    short_tail.words[array + 8] = 0x1048;
    short_tail.words[array + 12] = 0x10b4;
    short_tail.words[0x1024] = 20;
    short_tail.words[source] = 99;
    pal4::StoryRecordArray::InsertCopies(
        array, 0x1024, 3, source, short_tail);
    Check(short_tail.calls == std::vector<Call>{
              {"copy", 0x1090, 0x1024},
              {"copy", 0x1048, source},
              {"copy", 0x106c, source},
              {"assign", 0x1024, source},
              {"write", array + 8, 0x10b4}} &&
              short_tail.words[0x1090] == 20,
          "short tail is copied forward before new rows fill the gap");

    Runtime zero;
    zero.words[array + 4] = 0x1000;
    zero.words[array + 8] = 0x1024;
    zero.words[array + 12] = 0x1048;
    pal4::StoryRecordArray::InsertCopies(array, 0x1000, 0, source, zero);
    Check(zero.calls.empty() && zero.words[array + 8] == 0x1024,
          "zero copies leave a spare-capacity array unchanged");
}

void CheckStoryAndDescCopyGrowth() {
    Runtime story;
    story.allocations = {0x3000};
    story.words[array + 4] = 0x1000;
    story.words[array + 8] = 0x1048;
    story.words[array + 12] = 0x1048;
    story.words[0x1000] = 10;
    story.words[0x1024] = 20;
    story.words[source] = 99;
    pal4::StoryRecordArray::InsertCopies(
        array, 0x1024, 3, source, story);
    Check(story.calls == std::vector<Call>{
              {"allocate", 180},
              {"copy", 0x3000, 0x1000},
              {"copy", 0x3024, source},
              {"copy", 0x3048, source},
              {"copy", 0x306c, source},
              {"copy", 0x3090, 0x1024},
              {"destroy", 0x1000},
              {"destroy", 0x1024},
              {"free", 0x1000},
              {"write", array + 12, 0x30b4},
              {"write", array + 4, 0x3000},
              {"write", array + 8, 0x30b4}} &&
              story.words[0x3000] == 10 &&
              story.words[0x3090] == 20,
          "full Story array grows by max(old size, inserted count)");

    Runtime desc;
    desc.allocations = {0x4000};
    desc.words[array + 4] = 0x1000;
    desc.words[array + 8] = 0x1024;
    desc.words[array + 12] = 0x1024;
    desc.words[0x1000] = 10;
    desc.words[source] = 99;
    pal4::StoryDescRecordArray::InsertCopies(
        array, 0x1000, 2, source, desc);
    Check(desc.calls.front() == Call{"allocate", 108} &&
              desc.words[array + 4] == 0x4000 &&
              desc.words[array + 8] == 0x406c &&
              desc.words[0x4000] == 99 &&
              desc.words[0x4024] == 99 &&
              desc.words[0x4048] == 10,
          "StoryDesc copy growth uses its own record-copy runtime");
}

void CheckFill() {
    Runtime story;
    story.words[source] = 77;
    pal4::StoryRecordArray::Fill(0x3000, 3, source, story);
    Check(story.calls == std::vector<Call>{
              {"copy", 0x3000, source},
              {"copy", 0x3024, source},
              {"copy", 0x3048, source}} &&
              story.words[0x3000] == 77 &&
              story.words[0x3048] == 77,
          "Story fill copy-constructs three rows at 36-byte stride");

    Runtime desc;
    desc.words[source] = 88;
    pal4::StoryDescRecordArray::Fill(0x4000, 2, source, desc);
    Check(desc.calls == std::vector<Call>{
              {"copy", 0x4000, source},
              {"copy", 0x4024, source}} &&
              desc.words[0x4024] == 88,
          "StoryDesc fill uses its injected copy implementation");
    Runtime empty;
    pal4::StoryRecordArray::Fill(0x3000, 0, source, empty);
    pal4::StoryDescRecordArray::Fill(0x3000, 0, source, empty);
    Check(empty.calls.empty(), "zero count skips both 36-byte fill loops");
}

void CheckCallbackInstantiation() {
    for (const bool spare : {true, false}) {
        Runtime first;
        first.allocations = {0x3000};
        first.words[array + 4] = 0x1000;
        first.words[array + 8] = 0x1048;
        first.words[array + 12] = spare ? 0x106c : 0x1048;
        first.words[0x1000] = 10;
        first.words[0x1024] = 20;
        first.words[source] = 99;
        Runtime second = first;
        const auto result = pal4::StoryRecordArray::Insert(
            array, 0x1024, source, first);
        Check(pal4::StoryRecordArray::InsertCallback(
                  array, 0x1024, source, second) == result &&
                  second.words == first.words &&
                  second.calls == first.calls &&
                  second.next_allocation == first.next_allocation,
              "callback instantiation matches 36-byte Story insertion");
    }

    Runtime first;
    first.words[0x1000] = 10;
    first.words[0x1024] = 20;
    Runtime second = first;
    const auto result = pal4::StoryRecordArray::CopyRange(
        0x1000, 0x1048, 0x3000, first);
    Check(pal4::StoryRecordArray::CopyCallbackRange(
              0x1000, 0x1048, 0x3000, second) == result &&
              second.words == first.words &&
              second.calls == first.calls,
          "callback range copy matches Story 36-byte copy");

    for (const auto [used, capacity, insertion, copies] : {
             std::array<std::uint32_t, 4>{4, 6, 1, 2},
             std::array<std::uint32_t, 4>{3, 5, 2, 2},
             std::array<std::uint32_t, 4>{3, 3, 1, 2},
             std::array<std::uint32_t, 4>{0, 0, 0, 2},
             std::array<std::uint32_t, 4>{3, 3, 1, 0}}) {
        Runtime original;
        original.allocations = {0x3000};
        original.words[array + 4] = used ? 0x1000 : 0;
        original.words[array + 8] = used ? 0x1000 + used * 36 : 0;
        original.words[array + 12] = capacity ?
            0x1000 + capacity * 36 : 0;
        for (std::uint32_t i = 0; i < used; ++i)
            original.words[0x1000 + i * 36] = i + 1;
        original.words[source] = 99;
        Runtime callback = original;
        const auto position = used ? 0x1000 + insertion * 36 : 0;
        pal4::StoryRecordArray::InsertCopies(
            array, position, copies, source, original);
        pal4::StoryRecordArray::InsertCallbackCopies(
            array, position, copies, source, callback);
        Check(callback.words == original.words &&
                  callback.calls == original.calls &&
                  callback.next_allocation == original.next_allocation,
              "callback batch insertion matches Story branches");
    }

    Runtime fill_original;
    fill_original.words[source] = 77;
    Runtime fill_callback = fill_original;
    pal4::StoryRecordArray::Fill(0x3000, 3, source,
                                 fill_original);
    pal4::StoryRecordArray::FillCallback(0x3000, 3, source,
                                         fill_callback);
    Check(fill_callback.words == fill_original.words &&
              fill_callback.calls == fill_original.calls,
          "callback fill matches Story repeated copy");
    Runtime zero_fill;
    pal4::StoryRecordArray::FillCallback(0x3000, 0, source,
                                         zero_fill);
    Check(zero_fill.calls.empty(), "callback zero-fill skips copy calls");
}

}  // namespace

int main() {
    try {
        CheckInPlaceInsertion();
        CheckGrowth();
        CheckCopyFailure();
        CheckCopyRange();
        CheckStoryDescInsertion();
        CheckSizeAndStoryDescRange();
        CheckStoryInsertCopies();
        CheckStoryAndDescCopyGrowth();
        CheckFill();
        CheckCallbackInstantiation();
        std::cout << "story_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
