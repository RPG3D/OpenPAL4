#include "pal4/game_data_monster_record_array.h"

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

struct Runtime final : pal4::IGameDataMonsterRecordArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> destroyed_texts;
    std::uint32_t allocation = 0x5000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
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
    void DestroyText(std::uint32_t address) override {
        destroyed_texts.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing operation: " + call);
}

Runtime ThreeRecords(std::uint32_t capacity_end) {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x2000 + 3 * 596;
    runtime.words[0x100c] = capacity_end;
    runtime.words[0x2000] = 10;
    runtime.words[0x2000 + 596] = 20;
    runtime.words[0x2000 + 2 * 596] = 30;
    runtime.words[0x9000] = 99;
    return runtime;
}

void CheckInPlace() {
    auto runtime = ThreeRecords(0x2000 + 4 * 596);
    Check(pal4::GameDataMonsterRecordArray::Insert(
              0x1000, 0x2000 + 596, 0x9000, runtime) ==
              0x2000 + 596 &&
              runtime.words[0x1008] == 0x2000 + 4 * 596,
          "middle insertion returns original slot and advances end");
    Check(runtime.words[0x2000] == 10 &&
              runtime.words[0x2000 + 596] == 99 &&
              runtime.words[0x2000 + 2 * 596] == 20 &&
              runtime.words[0x2000 + 3 * 596] == 30 &&
              runtime.destroyed_texts.empty(),
          "in-place insert shifts records without destroying them");
    Check(At(runtime, "construct " +
                          std::to_string(0x2000 + 3 * 596) + " " +
                          std::to_string(0x2000 + 2 * 596)) <
              At(runtime, "assign " +
                              std::to_string(0x2000 + 2 * 596) + " " +
                              std::to_string(0x2000 + 596)) &&
              At(runtime, "assign " +
                              std::to_string(0x2000 + 596) + " 36864") <
                  At(runtime, "write 4104"),
          "tail construction precedes backward assignment and end update");

    auto append = ThreeRecords(0x2000 + 4 * 596);
    Check(pal4::GameDataMonsterRecordArray::Insert(
              0x1000, 0x2000 + 3 * 596, 0x9000, append) ==
              0x2000 + 3 * 596 &&
              append.calls.size() == 2 &&
              append.calls.front() ==
                  "construct " + std::to_string(0x2000 + 3 * 596) +
                      " 36864",
          "tail append constructs one record without shifts");
}

void CheckGrowth() {
    auto runtime = ThreeRecords(0x2000 + 3 * 596);
    Check(pal4::GameDataMonsterRecordArray::Insert(
              0x1000, 0x2000 + 596, 0x9000, runtime) ==
              0x5000 + 596 &&
              runtime.calls.front() == "allocate 3576",
          "full three-record array doubles capacity and relocates insert slot");
    Check(runtime.words[0x1004] == 0x5000 &&
              runtime.words[0x1008] == 0x5000 + 4 * 596 &&
              runtime.words[0x100c] == 0x5000 + 6 * 596 &&
              runtime.words[0x5000] == 10 &&
              runtime.words[0x5000 + 596] == 99 &&
              runtime.words[0x5000 + 2 * 596] == 20 &&
              runtime.words[0x5000 + 3 * 596] == 30,
          "growth preserves prefix, inserted record and suffix");
    Check(runtime.destroyed_texts.size() == 33 &&
              runtime.destroyed_texts[0] == 0x2000 + 336 + 232 &&
              runtime.destroyed_texts[22] ==
                  0x2000 + 2 * 596 + 336 + 232 &&
              At(runtime, "construct " +
                              std::to_string(0x5000 + 3 * 596) + " " +
                              std::to_string(0x2000 + 2 * 596)) <
                  At(runtime, "text " +
                                  std::to_string(0x2000 + 336 + 232)) &&
              At(runtime, "text " +
                              std::to_string(0x2000 + 2 * 596 +
                                             336 + 72)) <
                  At(runtime, "free 8192"),
          "deep copies finish before old record text destruction and free");

    Runtime empty;
    empty.words[0x9000] = 99;
    Check(pal4::GameDataMonsterRecordArray::Insert(
              0x1000, 0, 0x9000, empty) == 0x5000 &&
              empty.words[0x1004] == 0x5000 &&
              empty.words[0x1008] == 0x5000 + 596 &&
              empty.words[0x100c] == 0x5000 + 596 &&
              empty.calls.front() == "allocate 596",
          "empty array grows to one record and appends template");
}

void CheckHelpers() {
    auto runtime = ThreeRecords(0x2000 + 3 * 596);
    Check(pal4::GameDataMonsterRecordArray::Size(0x1000, runtime) == 3,
          "size counts 596-byte records");
    Runtime empty;
    Check(pal4::GameDataMonsterRecordArray::Size(0x1000, empty) == 0,
          "null array has zero size");
    Check(pal4::GameDataMonsterRecordArray::CopyRange(
              0x2000, 0x2000 + 2 * 596, 0x6000, runtime) ==
              0x6000 + 2 * 596 &&
              runtime.words[0x6000] == 10 &&
              runtime.words[0x6000 + 596] == 20,
          "copy helper deep-constructs a half-open two-record range");
    Check(pal4::GameDataMonsterRecordArray::CopyRange(
              0x2000, 0x2000, 0x7000, runtime) == 0x7000,
          "empty copy range returns unchanged destination");
}

void CheckInsertCopies() {
    auto zero = ThreeRecords(0x2000 + 4 * 596);
    pal4::GameDataMonsterRecordArray::InsertCopies(
        0x1000, 0x2000 + 596, 0, 0x9000, zero);
    Check(zero.calls.empty() && zero.words[0x1008] == 0x2000 + 3 * 596,
          "zero copies preserve record array without construction");

    auto shifted = ThreeRecords(0x2000 + 4 * 596);
    pal4::GameDataMonsterRecordArray::InsertCopies(
        0x1000, 0x2000 + 596, 1, 0x9000, shifted);
    Check(shifted.words[0x1008] == 0x2000 + 4 * 596 &&
              shifted.words[0x2000] == 10 &&
              shifted.words[0x2000 + 596] == 99 &&
              shifted.words[0x2000 + 2 * 596] == 20 &&
              shifted.words[0x2000 + 3 * 596] == 30 &&
              shifted.destroyed_texts.empty(),
          "tail at least count shifts backward in existing capacity");
    Check(At(shifted, "construct " +
                           std::to_string(0x2000 + 3 * 596) + " " +
                           std::to_string(0x2000 + 2 * 596)) <
              At(shifted, "assign " +
                              std::to_string(0x2000 + 2 * 596) + " " +
                              std::to_string(0x2000 + 596)) &&
              At(shifted, "assign " +
                              std::to_string(0x2000 + 596) + " 36864") <
                  At(shifted, "write 4104"),
          "tail construction, backward shift and template assignment order");

    auto shorter_tail = ThreeRecords(0x2000 + 5 * 596);
    pal4::GameDataMonsterRecordArray::InsertCopies(
        0x1000, 0x2000 + 2 * 596, 2, 0x9000, shorter_tail);
    Check(shorter_tail.words[0x1008] == 0x2000 + 5 * 596 &&
              shorter_tail.words[0x2000] == 10 &&
              shorter_tail.words[0x2000 + 596] == 20 &&
              shorter_tail.words[0x2000 + 2 * 596] == 99 &&
              shorter_tail.words[0x2000 + 3 * 596] == 99 &&
              shorter_tail.words[0x2000 + 4 * 596] == 30,
          "shorter tail moves beyond two copies and constructs remainder");

    auto grown = ThreeRecords(0x2000 + 3 * 596);
    pal4::GameDataMonsterRecordArray::InsertCopies(
        0x1000, 0x2000 + 596, 2, 0x9000, grown);
    Check(grown.calls.front() == "allocate 3576" &&
              grown.words[0x1004] == 0x5000 &&
              grown.words[0x1008] == 0x5000 + 5 * 596 &&
              grown.words[0x100c] == 0x5000 + 6 * 596 &&
              grown.words[0x5000] == 10 &&
              grown.words[0x5000 + 596] == 99 &&
              grown.words[0x5000 + 2 * 596] == 99 &&
              grown.words[0x5000 + 3 * 596] == 20 &&
              grown.words[0x5000 + 4 * 596] == 30 &&
              grown.destroyed_texts.size() == 33,
          "growth copies prefix, two templates and suffix before cleanup");

    Runtime empty;
    empty.words[0x9000] = 99;
    pal4::GameDataMonsterRecordArray::InsertCopies(
        0x1000, 0, 3, 0x9000, empty);
    Check(empty.calls.front() == "allocate 1788" &&
              empty.words[0x1004] == 0x5000 &&
              empty.words[0x1008] == 0x5000 + 3 * 596 &&
              empty.words[0x100c] == 0x5000 + 3 * 596 &&
              empty.words[0x5000] == 99 &&
              empty.words[0x5000 + 596] == 99 &&
              empty.words[0x5000 + 2 * 596] == 99,
          "empty array grows to exactly three template records");
}

void CheckFillConstruct() {
    Runtime runtime;
    runtime.words[0x9000] = 77;
    pal4::GameDataMonsterRecordArray::FillConstruct(
        0x6000, 3, 0x9000, runtime);
    Check(runtime.words[0x6000] == 77 &&
              runtime.words[0x6000 + 596] == 77 &&
              runtime.words[0x6000 + 2 * 596] == 77 &&
              runtime.calls.size() == 3,
          "fill helper deep-constructs three records at 596-byte stride");
    runtime.calls.clear();
    pal4::GameDataMonsterRecordArray::FillConstruct(
        0x7000, 0, 0x9000, runtime);
    Check(runtime.calls.empty(), "zero-count fill performs no construction");
}

}  // namespace

int main() {
    try {
        CheckInPlace();
        CheckGrowth();
        CheckHelpers();
        CheckInsertCopies();
        CheckFillConstruct();
        std::cout << "game_data_monster_record_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
