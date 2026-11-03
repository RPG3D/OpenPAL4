#include "pal4/game_data_parameter_record_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kStride = 328;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataParameterRecordArrayRuntime {
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
    void DestroyTextRecord(std::uint32_t address) override {
        destroyed_texts.push_back(address);
        calls.push_back("text " + std::to_string(address));
    }
};

Runtime ThreeRecords(std::uint32_t capacity_end) {
    Runtime runtime;
    runtime.words[0x1004] = 0x2000;
    runtime.words[0x1008] = 0x2000 + 3 * kStride;
    runtime.words[0x100c] = capacity_end;
    runtime.words[0x2000] = 10;
    runtime.words[0x2000 + kStride] = 20;
    runtime.words[0x2000 + 2 * kStride] = 30;
    runtime.words[0x9000] = 99;
    return runtime;
}

std::size_t At(const Runtime& runtime, const std::string& call) {
    for (std::size_t i = 0; i < runtime.calls.size(); ++i)
        if (runtime.calls[i] == call) return i;
    throw std::runtime_error("missing operation: " + call);
}

void CheckInsert() {
    auto runtime = ThreeRecords(0x2000 + 4 * kStride);
    Check(pal4::GameDataParameterRecordArray::Insert(
              0x1000, 0x2000 + kStride, 0x9000, runtime) ==
              0x2000 + kStride &&
              runtime.words[0x1008] == 0x2000 + 4 * kStride &&
              runtime.words[0x2000] == 10 &&
              runtime.words[0x2000 + kStride] == 99 &&
              runtime.words[0x2000 + 2 * kStride] == 20 &&
              runtime.words[0x2000 + 3 * kStride] == 30 &&
              runtime.destroyed_texts.empty(),
          "middle insertion shifts records inside available capacity");
    Check(At(runtime, "construct " +
                          std::to_string(0x2000 + 3 * kStride) + " " +
                          std::to_string(0x2000 + 2 * kStride)) <
              At(runtime, "assign " +
                              std::to_string(0x2000 + 2 * kStride) + " " +
                              std::to_string(0x2000 + kStride)) &&
              At(runtime, "assign " +
                              std::to_string(0x2000 + kStride) + " 36864") <
                  At(runtime, "write 4104"),
          "tail construction precedes backward shift and end update");

    auto append = ThreeRecords(0x2000 + 4 * kStride);
    Check(pal4::GameDataParameterRecordArray::Insert(
              0x1000, 0x2000 + 3 * kStride, 0x9000, append) ==
              0x2000 + 3 * kStride &&
              append.calls.size() == 2 &&
              append.calls.front() ==
                  "construct " + std::to_string(0x2000 + 3 * kStride) +
                      " 36864",
          "append constructs one record without shifts");

    auto grown = ThreeRecords(0x2000 + 3 * kStride);
    Check(pal4::GameDataParameterRecordArray::Insert(
              0x1000, 0x2000 + kStride, 0x9000, grown) ==
              0x5000 + kStride &&
              grown.calls.front() == "allocate 1968" &&
              grown.words[0x1004] == 0x5000 &&
              grown.words[0x1008] == 0x5000 + 4 * kStride &&
              grown.words[0x100c] == 0x5000 + 6 * kStride &&
              grown.words[0x5000] == 10 &&
              grown.words[0x5000 + kStride] == 99 &&
              grown.words[0x5000 + 2 * kStride] == 20 &&
              grown.words[0x5000 + 3 * kStride] == 30 &&
              grown.destroyed_texts.size() == 18,
          "full array doubles capacity and deep-copies all records");
    Check(grown.destroyed_texts.front() == 0x2000 + 84 &&
              grown.destroyed_texts[6] == 0x2000 + kStride + 84 &&
              At(grown, "construct " +
                            std::to_string(0x5000 + 3 * kStride) + " " +
                            std::to_string(0x2000 + 2 * kStride)) <
                  At(grown, "text " + std::to_string(0x2000 + 84)) &&
              At(grown, "text " +
                            std::to_string(0x2000 + 2 * kStride + 4)) <
                  At(grown, "free 8192"),
          "all deep copies finish before old text destruction and free");

    Runtime empty;
    empty.words[0x9000] = 99;
    Check(pal4::GameDataParameterRecordArray::Insert(
              0x1000, 0, 0x9000, empty) == 0x5000 &&
              empty.calls.front() == "allocate 328" &&
              empty.words[0x1004] == 0x5000 &&
              empty.words[0x1008] == 0x5000 + kStride &&
              empty.words[0x100c] == 0x5000 + kStride,
          "empty array grows to one record");
}

void CheckHelpers() {
    auto runtime = ThreeRecords(0x2000 + 3 * kStride);
    Check(pal4::GameDataParameterRecordArray::Size(0x1000, runtime) == 3,
          "size counts 328-byte records");
    Runtime empty;
    Check(pal4::GameDataParameterRecordArray::Size(0x1000, empty) == 0,
          "null array has zero size");
    Check(pal4::GameDataParameterRecordArray::CopyRange(
              0x2000, 0x2000 + 2 * kStride, 0x6000, runtime) ==
              0x6000 + 2 * kStride &&
              runtime.words[0x6000] == 10 &&
              runtime.words[0x6000 + kStride] == 20,
          "copy helper deep-constructs a half-open two-record range");
    Check(pal4::GameDataParameterRecordArray::CopyRange(
              0x2000, 0x2000, 0x7000, runtime) == 0x7000,
          "empty copy range keeps destination unchanged");
    pal4::GameDataParameterRecordArray::FillConstruct(
        0x7000, 3, 0x9000, runtime);
    Check(runtime.words[0x7000] == 99 &&
              runtime.words[0x7000 + kStride] == 99 &&
              runtime.words[0x7000 + 2 * kStride] == 99,
          "fill helper constructs three copies at 328-byte stride");
    runtime.calls.clear();
    pal4::GameDataParameterRecordArray::FillConstruct(
        0x8000, 0, 0x9000, runtime);
    Check(runtime.calls.empty(), "zero-count fill is inert");
}

void CheckInsertCopies() {
    auto zero = ThreeRecords(0x2000 + 4 * kStride);
    pal4::GameDataParameterRecordArray::InsertCopies(
        0x1000, 0x2000 + kStride, 0, 0x9000, zero);
    Check(zero.calls.empty(), "zero copies leave storage untouched");

    auto shifted = ThreeRecords(0x2000 + 4 * kStride);
    pal4::GameDataParameterRecordArray::InsertCopies(
        0x1000, 0x2000 + kStride, 1, 0x9000, shifted);
    Check(shifted.words[0x1008] == 0x2000 + 4 * kStride &&
              shifted.words[0x2000 + kStride] == 99 &&
              shifted.words[0x2000 + 2 * kStride] == 20 &&
              shifted.words[0x2000 + 3 * kStride] == 30 &&
              shifted.destroyed_texts.empty(),
          "longer tail shifts backward in place");

    auto short_tail = ThreeRecords(0x2000 + 5 * kStride);
    pal4::GameDataParameterRecordArray::InsertCopies(
        0x1000, 0x2000 + 2 * kStride, 2, 0x9000, short_tail);
    Check(short_tail.words[0x1008] == 0x2000 + 5 * kStride &&
              short_tail.words[0x2000 + 2 * kStride] == 99 &&
              short_tail.words[0x2000 + 3 * kStride] == 99 &&
              short_tail.words[0x2000 + 4 * kStride] == 30,
          "shorter tail moves beyond inserted copies");

    auto grown = ThreeRecords(0x2000 + 3 * kStride);
    pal4::GameDataParameterRecordArray::InsertCopies(
        0x1000, 0x2000 + kStride, 2, 0x9000, grown);
    Check(grown.calls.front() == "allocate 1968" &&
              grown.words[0x1004] == 0x5000 &&
              grown.words[0x1008] == 0x5000 + 5 * kStride &&
              grown.words[0x100c] == 0x5000 + 6 * kStride &&
              grown.words[0x5000] == 10 &&
              grown.words[0x5000 + kStride] == 99 &&
              grown.words[0x5000 + 2 * kStride] == 99 &&
              grown.words[0x5000 + 3 * kStride] == 20 &&
              grown.words[0x5000 + 4 * kStride] == 30 &&
              grown.destroyed_texts.size() == 18,
          "growth copies prefix, two templates and suffix before cleanup");

    Runtime empty;
    empty.words[0x9000] = 99;
    pal4::GameDataParameterRecordArray::InsertCopies(
        0x1000, 0, 3, 0x9000, empty);
    Check(empty.calls.front() == "allocate 984" &&
              empty.words[0x1004] == 0x5000 &&
              empty.words[0x1008] == 0x5000 + 3 * kStride &&
              empty.words[0x100c] == 0x5000 + 3 * kStride &&
              empty.words[0x5000] == 99 &&
              empty.words[0x5000 + kStride] == 99 &&
              empty.words[0x5000 + 2 * kStride] == 99,
          "empty array grows to exactly three repeated records");
}

}  // namespace

int main() {
    try {
        CheckInsert();
        CheckHelpers();
        CheckInsertCopies();
        std::cout << "game_data_parameter_record_array_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
