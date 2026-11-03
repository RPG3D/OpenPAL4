#include "pal4/game_data_parameter_record_copy.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

constexpr std::uint32_t kStride = 328;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    bool text;
    std::uint32_t destination;
    std::uint32_t source;
    std::uint32_t bytes;
};

struct Runtime final : pal4::IGameDataParameterRecordCopyRuntime {
    std::array<std::uint8_t, 0x10000> memory{};
    std::vector<Call> calls;
    std::vector<bool> initialized_texts;

    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t bytes) override {
        calls.push_back({false, destination, source, bytes});
        std::copy_n(memory.begin() + source, bytes,
                    memory.begin() + destination);
    }

    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({true, destination, source, 16});
        initialized_texts.push_back(
            memory[destination] == memory[source] &&
            std::all_of(memory.begin() + destination + 4,
                        memory.begin() + destination + 16,
                        [](std::uint8_t value) { return value == 0; }));
        // The test runtime models the resulting value; native ownership is
        // supplied by the game text runtime.
        std::copy_n(memory.begin() + source, 16,
                    memory.begin() + destination);
    }

    std::uint8_t ReadByte(std::uint32_t address) override {
        return memory[address];
    }

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        memory[address] = value;
    }

    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4; ++i)
            memory[address + i] =
                static_cast<std::uint8_t>(value >> (i * 8));
    }

    void Seed(std::uint32_t address, std::uint8_t seed) {
        for (std::uint32_t i = 0; i < kStride; ++i)
            memory[address + i] = static_cast<std::uint8_t>(seed + i * 13);
    }

    [[nodiscard]] std::array<std::uint8_t, kStride> Record(
        std::uint32_t address) const {
        std::array<std::uint8_t, kStride> value{};
        std::copy_n(memory.begin() + address, kStride, value.begin());
        return value;
    }
};

void CheckCallOrder(const Runtime& runtime, std::uint32_t destination,
                    std::uint32_t source, std::size_t first) {
    Check(runtime.calls.size() >= first + 8, "complete record call sequence");
    const auto& head = runtime.calls[first];
    Check(!head.text && head.destination == destination &&
              head.source == source && head.bytes == 4,
          "record key is copied first");
    for (std::uint32_t i = 0; i < 6; ++i) {
        const auto& call = runtime.calls[first + 1 + i];
        const auto offset = 4 + i * 16;
        Check(call.text && call.destination == destination + offset &&
                  call.source == source + offset,
              "six text members are assigned in offset order");
    }
    const auto& tail = runtime.calls[first + 7];
    Check(!tail.text && tail.destination == destination + 100 &&
              tail.source == source + 100 && tail.bytes == 228,
          "all scalar fields from +100 to +327 are copied last");
}

void CheckForward() {
    Runtime runtime;
    runtime.Seed(0x1000, 3);
    runtime.Seed(0x2000, 29);
    runtime.Seed(0x2000 + kStride, 47);
    const auto source = runtime.Record(0x1000);
    runtime.memory[0x1fff] = 0xa5;
    runtime.memory[0x2000 + 2 * kStride] = 0x5a;

    const auto end = 0x2000 + 2 * kStride;
    Check(pal4::GameDataParameterRecordCopy::AssignRange(
              0x2000, end, 0x1000, runtime) == end,
          "forward assignment returns destination end");
    Check(runtime.Record(0x2000) == source &&
              runtime.Record(0x2000 + kStride) == source,
          "two destination records receive the complete source value");
    Check(runtime.memory[0x1fff] == 0xa5 && runtime.memory[end] == 0x5a,
          "assignment preserves both adjacent bytes");
    Check(runtime.calls.size() == 16, "eight operations per record");
    CheckCallOrder(runtime, 0x2000, 0x1000, 0);
    CheckCallOrder(runtime, 0x2000 + kStride, 0x1000, 8);

    runtime.calls.clear();
    Check(pal4::GameDataParameterRecordCopy::AssignRange(
              0x2000, 0x2000, 0x1000, runtime) == 0x2000 &&
              runtime.calls.empty(),
          "empty forward range performs no writes");
}

void CheckBackwardOverlap() {
    Runtime runtime;
    const auto begin = 0x3000u;
    for (std::uint32_t i = 0; i < 3; ++i)
        runtime.Seed(begin + i * kStride,
                     static_cast<std::uint8_t>(11 + i * 21));
    runtime.Seed(begin + 3 * kStride, 97);
    const auto first = runtime.Record(begin);
    const auto second = runtime.Record(begin + kStride);
    const auto third = runtime.Record(begin + 2 * kStride);

    Check(pal4::GameDataParameterRecordCopy::AssignBackward(
              begin, begin + 3 * kStride, begin + 4 * kStride,
              runtime) == begin + kStride,
          "backward assignment returns new destination begin");
    Check(runtime.Record(begin) == first &&
              runtime.Record(begin + kStride) == first &&
              runtime.Record(begin + 2 * kStride) == second &&
              runtime.Record(begin + 3 * kStride) == third,
          "right shift preserves all overlapping source records");
    Check(runtime.calls.size() == 24, "three records use 24 operations");
    for (std::uint32_t i = 0; i < 3; ++i)
        CheckCallOrder(runtime, begin + (3 - i) * kStride,
                       begin + (2 - i) * kStride, i * 8);

    runtime.calls.clear();
    Check(pal4::GameDataParameterRecordCopy::AssignBackward(
              begin, begin, begin + 4 * kStride, runtime) ==
              begin + 4 * kStride && runtime.calls.empty(),
          "empty backward range preserves destination end");
}

void CheckSelfAssignment() {
    Runtime runtime;
    runtime.Seed(0x5000, 41);
    const auto original = runtime.Record(0x5000);
    pal4::GameDataParameterRecordCopy::Assign(0x5000, 0x5000, runtime);
    Check(runtime.Record(0x5000) == original && runtime.calls.size() == 8,
          "self assignment keeps record content");
    CheckCallOrder(runtime, 0x5000, 0x5000, 0);
}

void CheckConstruction() {
    Runtime runtime;
    runtime.Seed(0x1000, 17);
    runtime.Seed(0x6000, 83);
    const auto source = runtime.Record(0x1000);
    runtime.memory[0x5fff] = 0x55;
    runtime.memory[0x6000 + kStride] = 0xaa;
    pal4::GameDataParameterRecordCopy::Construct(0x6000, 0x1000, runtime);
    Check(runtime.Record(0x6000) == source,
          "construction deep-copies the complete 328-byte record");
    Check(runtime.memory[0x5fff] == 0x55 &&
              runtime.memory[0x6000 + kStride] == 0xaa,
          "construction preserves adjacent bytes");
    Check(runtime.calls.size() == 8, "construction copies eight regions");
    CheckCallOrder(runtime, 0x6000, 0x1000, 0);
    Check(runtime.initialized_texts.size() == 6 &&
              std::all_of(runtime.initialized_texts.begin(),
                          runtime.initialized_texts.end(),
                          [](bool value) { return value; }),
          "each text seed is copied and its storage words are zero before assignment");

    runtime.calls.clear();
    pal4::GameDataParameterRecordCopy::Construct(0, 0x1000, runtime);
    Check(runtime.calls.empty(), "null destination skips construction");
}

void CheckRecordAssignment() {
    Runtime runtime;
    runtime.Seed(0x1000, 59);
    runtime.Seed(0x7000, 101);
    const auto source = runtime.Record(0x1000);
    Check(pal4::GameDataParameterRecordCopy::AssignRecord(
              0x7000, 0x1000, runtime) == 0x7000,
          "record assignment returns its destination");
    Check(runtime.Record(0x7000) == source,
          "record assignment updates all 328 bytes");
    CheckCallOrder(runtime, 0x7000, 0x1000, 0);
}

void CheckSelectedRecords() {
    Runtime runtime;
    constexpr std::uint32_t begin = 0x1000;
    constexpr std::uint32_t destination = 0x4000;
    constexpr std::array<std::uint8_t, 5> keys{10, 10, 20, 20, 10};
    for (std::uint32_t i = 0; i < keys.size(); ++i) {
        runtime.Seed(begin + i * kStride, keys[i]);
        runtime.memory[begin + i * kStride] = keys[i];
    }
    const auto first = runtime.Record(begin);
    const auto third = runtime.Record(begin + 2 * kStride);
    const auto fifth = runtime.Record(begin + 4 * kStride);
    std::vector<std::array<std::uint32_t, 2>> compared;
    const auto equivalent = [&](std::uint32_t left, std::uint32_t right) {
        compared.push_back({left, right});
        return runtime.memory[left] == runtime.memory[right];
    };

    Check(pal4::GameDataParameterRecordCopy::CopySelected(
              begin, begin + 5 * kStride, destination,
              equivalent, runtime) == destination + 3 * kStride,
          "selection returns the end of three retained records");
    Check(runtime.Record(destination) == first &&
              runtime.Record(destination + kStride) == third &&
              runtime.Record(destination + 2 * kStride) == fifth,
          "selection retains first record and each changed key");
    Check(compared ==
              std::vector<std::array<std::uint32_t, 2>>{
                  {begin, begin + kStride},
                  {begin, begin + 2 * kStride},
                  {begin + 2 * kStride, begin + 3 * kStride},
                  {begin + 2 * kStride, begin + 4 * kStride}},
          "each candidate compares against the last retained source");
    Check(runtime.calls.size() == 24,
          "only retained records are assigned");
    CheckCallOrder(runtime, destination, begin, 0);
    CheckCallOrder(runtime, destination + kStride,
                   begin + 2 * kStride, 8);
    CheckCallOrder(runtime, destination + 2 * kStride,
                   begin + 4 * kStride, 16);

    runtime.calls.clear();
    compared.clear();
    Check(pal4::GameDataParameterRecordCopy::CopySelected(
              begin, begin, destination, equivalent, runtime) ==
              destination && runtime.calls.empty() && compared.empty(),
          "empty selection does not assign or compare");

    runtime.calls.clear();
    Check(pal4::GameDataParameterRecordCopy::CopySelected(
              begin, begin + kStride, destination, equivalent,
              runtime) == destination + kStride &&
              runtime.calls.size() == 8 && compared.empty(),
          "singleton selection copies the first record without comparison");
}

}  // namespace

int main() {
    try {
        CheckForward();
        CheckBackwardOverlap();
        CheckSelfAssignment();
        CheckConstruction();
        CheckRecordAssignment();
        CheckSelectedRecords();
        std::cout << "game_data_parameter_record_copy_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
