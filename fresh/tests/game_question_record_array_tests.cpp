#include "pal4/game_question_record_array.h"

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

struct Runtime final : pal4::IGameQuestionRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations;
    std::size_t next_allocation = 0;

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
    std::uint32_t CopyRecord(std::uint32_t destination,
                             std::uint32_t source) override {
        calls.push_back({"copy", destination, source});
        words[destination] = words[source];
        return destination + 1;
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

void CheckInPlace() {
    Runtime middle;
    middle.words[array + 4] = 0x1000;
    middle.words[array + 8] = 0x10e4;
    middle.words[array + 12] = 0x1130;
    middle.words[0x1000] = 10;
    middle.words[0x104c] = 20;
    middle.words[0x1098] = 30;
    middle.words[source] = 99;
    Check(pal4::GameQuestionRecordArray::Insert(
              array, 0x104c, source, middle) == 0x104c &&
              middle.calls == std::vector<Call>{
                  {"copy", 0x10e4, 0x1098},
                  {"assign", 0x1098, 0x104c},
                  {"assign", 0x104c, source},
                  {"write", array + 8, 0x1130}} &&
              middle.words[0x104c] == 99 &&
              middle.words[0x1098] == 20 &&
              middle.words[0x10e4] == 30,
          "spare 76-byte slot shifts suffix backward before assignment");

    Runtime append;
    append.words[array + 4] = 0x1000;
    append.words[array + 8] = 0x104c;
    append.words[array + 12] = 0x1098;
    append.words[source] = 42;
    Check(pal4::GameQuestionRecordArray::Insert(
              array, 0x104c, source, append) == 0x104c &&
              append.calls == std::vector<Call>{
                  {"copy", 0x104c, source},
                  {"write", array + 8, 0x1098}},
          "append copy-constructs at active end");
}

void CheckGrowth() {
    Runtime grow;
    grow.allocations = {0x3000};
    grow.words[array + 4] = 0x1000;
    grow.words[array + 8] = 0x1098;
    grow.words[array + 12] = 0x1098;
    grow.words[0x1000] = 10;
    grow.words[0x104c] = 20;
    grow.words[source] = 99;
    Check(pal4::GameQuestionRecordArray::Insert(
              array, 0x104c, source, grow) == 0x304c &&
              grow.calls == std::vector<Call>{
                  {"allocate", 304},
                  {"copy", 0x3000, 0x1000},
                  {"copy", 0x304c, source},
                  {"copy", 0x3098, 0x104c},
                  {"destroy", 0x1000},
                  {"destroy", 0x104c},
                  {"free", 0x1000},
                  {"write", array + 12, 0x3130},
                  {"write", array + 4, 0x3000},
                  {"write", array + 8, 0x30e4}} &&
              grow.words[0x3000] == 10 &&
              grow.words[0x304c] == 99 &&
              grow.words[0x3098] == 20,
          "full question array doubles capacity and preserves row order");

    Runtime empty;
    empty.allocations = {0x3000};
    empty.words[source] = 7;
    Check(pal4::GameQuestionRecordArray::Insert(
              array, 0, source, empty) == 0x3000 &&
              empty.calls == std::vector<Call>{
                  {"allocate", 76},
                  {"copy", 0x3000, source},
                  {"free", 0},
                  {"write", array + 12, 0x304c},
                  {"write", array + 4, 0x3000},
                  {"write", array + 8, 0x304c}},
          "empty question array starts with one record of capacity");
}

void CheckSizeAndRange() {
    Runtime runtime;
    Check(pal4::GameQuestionRecordArray::Size(array, runtime) == 0,
          "null question array is empty");
    runtime.words[array + 4] = 0x1000;
    runtime.words[array + 8] = 0x10e4;
    Check(pal4::GameQuestionRecordArray::Size(array, runtime) == 3,
          "question array counts 76-byte active records");
    runtime.words[0x1000] = 10;
    runtime.words[0x104c] = 20;
    Check(pal4::GameQuestionRecordArray::CopyRange(
              0x1000, 0x1098, 0x3000, runtime) == 0x3098 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x3000, 0x1000},
                  {"copy", 0x304c, 0x104c}} &&
              runtime.words[0x3000] == 10 &&
              runtime.words[0x304c] == 20,
          "range copy uses 76-byte stride and returns destination end");
    Check(pal4::GameQuestionRecordArray::CopyRange(
              0x1000, 0x1000, 0x4000, runtime) == 0x4000 &&
              runtime.calls.size() == 2,
          "empty range leaves destination unchanged");
}

void CheckInsertCopies() {
    Runtime shift;
    shift.words[array + 4] = 0x1000;
    shift.words[array + 8] = 0x1130;
    shift.words[array + 12] = 0x11c8;
    shift.words[0x1000] = 10;
    shift.words[0x104c] = 20;
    shift.words[0x1098] = 30;
    shift.words[0x10e4] = 40;
    shift.words[source] = 99;
    pal4::GameQuestionRecordArray::InsertCopies(
        array, 0x104c, 2, source, shift);
    Check(shift.calls == std::vector<Call>{
              {"copy", 0x1130, 0x1098},
              {"copy", 0x117c, 0x10e4},
              {"assign", 0x10e4, 0x104c},
              {"assign", 0x104c, source},
              {"assign", 0x1098, source},
              {"write", array + 8, 0x11c8}} &&
              shift.words[0x10e4] == 20 &&
              shift.words[0x1130] == 30 &&
              shift.words[0x117c] == 40,
          "question rows shift backward at 76-byte stride");

    Runtime short_tail;
    short_tail.words[array + 4] = 0x1000;
    short_tail.words[array + 8] = 0x1098;
    short_tail.words[array + 12] = 0x117c;
    short_tail.words[0x104c] = 20;
    short_tail.words[source] = 99;
    pal4::GameQuestionRecordArray::InsertCopies(
        array, 0x104c, 3, source, short_tail);
    Check(short_tail.calls == std::vector<Call>{
              {"copy", 0x1130, 0x104c},
              {"copy", 0x1098, source},
              {"copy", 0x10e4, source},
              {"assign", 0x104c, source},
              {"write", array + 8, 0x117c}} &&
              short_tail.words[0x1130] == 20,
          "short question tail is copied past the inserted copies");

    Runtime grow;
    grow.allocations = {0x3000};
    grow.words[array + 4] = 0x1000;
    grow.words[array + 8] = 0x1098;
    grow.words[array + 12] = 0x1098;
    grow.words[0x1000] = 10;
    grow.words[0x104c] = 20;
    grow.words[source] = 99;
    pal4::GameQuestionRecordArray::InsertCopies(
        array, 0x104c, 3, source, grow);
    Check(grow.calls == std::vector<Call>{
              {"allocate", 380},
              {"copy", 0x3000, 0x1000},
              {"copy", 0x304c, source},
              {"copy", 0x3098, source},
              {"copy", 0x30e4, source},
              {"copy", 0x3130, 0x104c},
              {"destroy", 0x1000},
              {"destroy", 0x104c},
              {"free", 0x1000},
              {"write", array + 12, 0x317c},
              {"write", array + 4, 0x3000},
              {"write", array + 8, 0x317c}} &&
              grow.words[0x3000] == 10 &&
              grow.words[0x3130] == 20,
          "full question array grows for count and preserves suffix");

    Runtime zero;
    zero.words[array + 4] = 0x1000;
    zero.words[array + 8] = 0x104c;
    zero.words[array + 12] = 0x1098;
    pal4::GameQuestionRecordArray::InsertCopies(
        array, 0x1000, 0, source, zero);
    Check(zero.calls.empty(), "zero question copies make no changes");
}

void CheckFill() {
    Runtime runtime;
    runtime.words[source] = 77;
    Check(pal4::GameQuestionRecordArray::Fill(
              0x3000, 3, source, runtime) == 0x3099 &&
              runtime.calls == std::vector<Call>{
                  {"copy", 0x3000, source},
                  {"copy", 0x304c, source},
                  {"copy", 0x3098, source}} &&
              runtime.words[0x3000] == 77 &&
              runtime.words[0x3098] == 77,
          "question fill forwards last copy result at 76-byte stride");
    Runtime empty;
    Check(pal4::GameQuestionRecordArray::Fill(
              0x3000, 0, source, empty) == 0 &&
              empty.calls.empty(),
          "empty question fill performs no copy");
}

}  // namespace

int main() {
    try {
        CheckInPlace();
        CheckGrowth();
        CheckSizeAndRange();
        CheckInsertCopies();
        CheckFill();
        std::cout << "game_question_record_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
