#include "pal4/game_data_mission_record_array.h"

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

struct Runtime final : pal4::IGameDataMissionRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    void CopyConstruct(std::uint32_t destination,
                       std::uint32_t source) override {
        calls.push_back("construct " + std::to_string(destination) + " " +
                        std::to_string(source));
        words[destination] = words[source];
    }
    void Assign(std::uint32_t destination,
                std::uint32_t source) override {
        calls.push_back("assign " + std::to_string(destination) + " " +
                        std::to_string(source));
        words[destination] = words[source];
    }
    void Destroy(std::uint32_t record) override {
        calls.push_back("destroy " + std::to_string(record));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing call: " + call);
}

Runtime ThreeRecords(std::uint32_t capacity_end) {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x20cc;
    runtime.words[0x100c] = capacity_end;
    runtime.words[0x2000] = 10;
    runtime.words[0x2044] = 20;
    runtime.words[0x2088] = 30;
    runtime.words[0x9000] = 99;
    return runtime;
}

void CheckInPlace() {
    auto runtime = ThreeRecords(0x2110);
    Check(pal4::GameDataMissionRecordArray::Insert(
              0x1000, 0x2044, 0x9000, runtime) == 0x2044 &&
              runtime.words[0x1008] == 0x2110,
          "in-place middle insertion returns original slot and advances end");
    Check(runtime.words[0x2000] == 10 &&
              runtime.words[0x2044] == 99 &&
              runtime.words[0x2088] == 20 &&
              runtime.words[0x20cc] == 30,
          "in-place insertion preserves record values in sequence");
    Check(runtime.calls == std::vector<std::string>{
                               "construct 8396 8328",
                               "assign 8328 8260",
                               "assign 8260 36864",
                               "write 4104 8464"},
          "in-place shift constructs tail then assigns backward");

    auto append = ThreeRecords(0x2110);
    Check(pal4::GameDataMissionRecordArray::Insert(
              0x1000, 0x20cc, 0x9000, append) == 0x20cc &&
              append.calls == std::vector<std::string>{
                                  "construct 8396 36864",
                                  "write 4104 8464"},
          "in-place append constructs one new record without assignments");
}

void CheckGrowth() {
    auto runtime = ThreeRecords(0x20cc);
    Check(pal4::GameDataMissionRecordArray::Insert(
              0x1000, 0x2044, 0x9000, runtime) == 0x5044,
          "grown insertion returns relocated slot");
    Check(runtime.calls.front() == "allocate 408" &&
              At(runtime, "construct 20480 8192") <
                  At(runtime, "construct 20548 36864") &&
              At(runtime, "construct 20548 36864") <
                  At(runtime, "construct 20616 8260") &&
              At(runtime, "construct 20684 8328") <
                  At(runtime, "destroy 8192") &&
              At(runtime, "destroy 8328") < At(runtime, "free 8192"),
          "growth copies prefix, inserted record and suffix before cleanup");
    Check(runtime.words[0x1004] == 0x5000 &&
              runtime.words[0x1008] == 0x5110 &&
              runtime.words[0x100c] == 0x5198 &&
              runtime.words[0x5000] == 10 &&
              runtime.words[0x5044] == 99 &&
              runtime.words[0x5088] == 20 &&
              runtime.words[0x50cc] == 30,
          "growth doubles three-slot capacity and preserves all four values");

    Runtime empty;
    empty.words[0x9000] = 99;
    Check(pal4::GameDataMissionRecordArray::Insert(
              0x1000, 0, 0x9000, empty) == 0x5000 &&
              empty.calls == std::vector<std::string>{
                                 "allocate 68",
                                 "construct 20480 36864",
                                 "free 0",
                                 "write 4108 20548",
                                 "write 4100 20480",
                                 "write 4104 20548"},
          "empty array allocates one record and sets all three pointers");
}

void CheckCopyRange() {
    Runtime runtime;
    runtime.words[0x2000] = 10;
    runtime.words[0x2044] = 20;
    Check(pal4::GameDataMissionRecordArray::CopyRange(
              0x2000, 0x2088, 0x5000, runtime) == 0x5088 &&
              runtime.words[0x5000] == 10 &&
              runtime.words[0x5044] == 20 &&
              runtime.calls == std::vector<std::string>{
                                   "construct 20480 8192",
                                   "construct 20548 8260"},
          "range copy constructs each 68-byte record and returns new end");
    Runtime empty;
    Check(pal4::GameDataMissionRecordArray::CopyRange(
              0x2000, 0x2000, 0x5000, empty) == 0x5000 &&
              empty.calls.empty(),
          "empty copy range returns unchanged destination");
}

void CheckInsertCopiesInPlace() {
    auto shifted = ThreeRecords(0x2198);
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0x2000, 1, 0x9000, shifted);
    Check(shifted.words[0x1008] == 0x2110 &&
              shifted.words[0x2000] == 99 &&
              shifted.words[0x2044] == 10 &&
              shifted.words[0x2088] == 20 &&
              shifted.words[0x20cc] == 30 &&
              shifted.calls.front() == "construct 8396 8328" &&
              At(shifted, "assign 8328 8260") <
                  At(shifted, "assign 8260 8192") &&
              At(shifted, "assign 8260 8192") <
                  At(shifted, "assign 8192 36864"),
          "one-copy insertion shifts a longer tail backward");

    auto equal_tail = ThreeRecords(0x2198);
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0x2044, 2, 0x9000, equal_tail);
    Check(equal_tail.words[0x1008] == 0x2154 &&
              equal_tail.words[0x2000] == 10 &&
              equal_tail.words[0x2044] == 99 &&
              equal_tail.words[0x2088] == 99 &&
              equal_tail.words[0x20cc] == 20 &&
              equal_tail.words[0x2110] == 30,
          "tail equal to count is copy-constructed before slot assignments");

    auto short_tail = ThreeRecords(0x2198);
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0x2088, 3, 0x9000, short_tail);
    Check(short_tail.words[0x1008] == 0x2198 &&
              short_tail.words[0x2000] == 10 &&
              short_tail.words[0x2044] == 20 &&
              short_tail.words[0x2088] == 99 &&
              short_tail.words[0x20cc] == 99 &&
              short_tail.words[0x2110] == 99 &&
              short_tail.words[0x2154] == 30 &&
              At(short_tail, "construct 8532 8328") <
                  At(short_tail, "construct 8396 36864") &&
              At(short_tail, "construct 8464 36864") <
                  At(short_tail, "assign 8328 36864"),
          "short tail moves beyond end and constructs remaining new copies");

    auto zero = ThreeRecords(0x20cc);
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0x2044, 0, 0x9000, zero);
    Check(zero.calls.empty() && zero.words[0x1008] == 0x20cc,
          "zero-count insertion leaves a full array unchanged");
}

void CheckInsertCopiesGrowth() {
    auto runtime = ThreeRecords(0x20cc);
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0x2044, 2, 0x9000, runtime);
    Check(runtime.calls.front() == "allocate 408" &&
              runtime.words[0x1004] == 0x5000 &&
              runtime.words[0x1008] == 0x5154 &&
              runtime.words[0x100c] == 0x5198 &&
              runtime.words[0x5000] == 10 &&
              runtime.words[0x5044] == 99 &&
              runtime.words[0x5088] == 99 &&
              runtime.words[0x50cc] == 20 &&
              runtime.words[0x5110] == 30 &&
              At(runtime, "construct 20752 8328") <
                  At(runtime, "destroy 8192") &&
              At(runtime, "destroy 8328") < At(runtime, "free 8192"),
          "growth doubles three-slot capacity and deep-copies all five values");

    Runtime empty;
    empty.words[0x9000] = 99;
    pal4::GameDataMissionRecordArray::InsertCopies(
        0x1000, 0, 2, 0x9000, empty);
    Check(empty.calls.front() == "allocate 136" &&
              empty.words[0x1004] == 0x5000 &&
              empty.words[0x1008] == 0x5088 &&
              empty.words[0x100c] == 0x5088 &&
              empty.words[0x5000] == 99 &&
              empty.words[0x5044] == 99 &&
              At(empty, "construct 20548 36864") < At(empty, "free 0"),
          "empty array grows to requested count and constructs each record");
}

void CheckFillConstruct() {
    Runtime runtime;
    runtime.words[0x9000] = 99;
    pal4::GameDataMissionRecordArray::FillConstruct(
        0x5000, 3, 0x9000, runtime);
    Check(runtime.calls == std::vector<std::string>{
                               "construct 20480 36864",
                               "construct 20548 36864",
                               "construct 20616 36864"} &&
              runtime.words[0x5000] == 99 &&
              runtime.words[0x5044] == 99 &&
              runtime.words[0x5088] == 99,
          "fill helper constructs each template copy at 68-byte stride");
    Runtime empty;
    pal4::GameDataMissionRecordArray::FillConstruct(
        0x5000, 0, 0x9000, empty);
    Check(empty.calls.empty(), "zero-count fill performs no construction");
}

void CheckAssignCopies() {
    Runtime runtime;
    runtime.words[0x9000] = 99;
    Check(pal4::GameDataMissionRecordArray::AssignCopies(
              0x2000, 0x2088, 0x9000, runtime) == 0x2088 &&
              runtime.words[0x2000] == 99 &&
              runtime.words[0x2044] == 99 &&
              runtime.calls == std::vector<std::string>{
                                   "assign 8192 36864",
                                   "assign 8260 36864"},
          "assignment helper updates every existing 68-byte record");
    Runtime empty;
    Check(pal4::GameDataMissionRecordArray::AssignCopies(
              0x2000, 0x2000, 0x9000, empty) == 0x2000 &&
              empty.calls.empty(),
          "empty assignment range returns its unchanged end");
}

void CheckShiftBackward() {
    Runtime runtime;
    runtime.words[0x2000] = 10;
    runtime.words[0x2044] = 20;
    Check(pal4::GameDataMissionRecordArray::ShiftBackward(
              0x2000, 0x2088, 0x20cc, runtime) == 0x2044 &&
              runtime.words[0x2044] == 10 &&
              runtime.words[0x2088] == 20 &&
              runtime.calls == std::vector<std::string>{
                                   "assign 8328 8260",
                                   "assign 8260 8192"},
          "overlapping shift assigns from the end toward the beginning");
    Runtime empty;
    Check(pal4::GameDataMissionRecordArray::ShiftBackward(
              0x2000, 0x2000, 0x5000, empty) == 0x5000 &&
              empty.calls.empty(),
          "empty backward shift returns unchanged destination end");
}

}  // namespace

int main() {
    try {
        CheckInPlace();
        CheckGrowth();
        CheckCopyRange();
        CheckInsertCopiesInPlace();
        CheckInsertCopiesGrowth();
        CheckFillConstruct();
        CheckAssignCopies();
        CheckShiftBackward();
        std::cout << "game_data_mission_record_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
