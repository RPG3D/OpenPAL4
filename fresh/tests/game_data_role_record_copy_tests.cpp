#include "pal4/game_data_role_record_copy.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t destination = 0;
    std::uint32_t source = 0;
    std::uint32_t count = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataRoleRecordCopyRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t text_calls = 0;
    std::uint32_t fail_on_text_call = 0;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        for (std::uint32_t i = 0; i < 4; ++i)
            bytes[address + i] =
                static_cast<std::uint8_t>(value >> (i * 8));
        calls.push_back({"word", address, value});
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t count) override {
        for (std::uint32_t i = 0; i < count; ++i)
            bytes[destination + i] = bytes[source + i];
        calls.push_back({"copy", destination, source, count});
    }
    std::uint32_t AssignText(std::uint32_t destination,
                             std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_on_text_call)
            throw std::runtime_error("text assignment failed");
        return destination + 0x55;
    }

    void FillScalars(std::uint32_t source) {
        for (std::uint32_t i = 0; i < 144; ++i)
            bytes[source + i] = static_cast<std::uint8_t>(i + 7);
        for (std::uint32_t i = 208; i < 240; ++i)
            bytes[source + i] = static_cast<std::uint8_t>(i + 11);
        for (std::uint32_t i = 0; i < 4; ++i)
            bytes[source + 144 + i * 16] =
                static_cast<std::uint8_t>(0x40 + i);
        for (std::uint32_t i = 0; i < 5; ++i)
            bytes[source + 240 + i * 16] =
                static_cast<std::uint8_t>(0x50 + i);
    }

    void CheckScalars(std::uint32_t destination) {
        for (std::uint32_t i = 0; i < 144; ++i)
            Check(bytes[destination + i] ==
                      static_cast<std::uint8_t>(i + 7),
                  "first 144 scalar bytes copied");
        for (std::uint32_t i = 208; i < 240; ++i)
            Check(bytes[destination + i] ==
                      static_cast<std::uint8_t>(i + 11),
                  "middle 32 scalar bytes copied");
    }
};

void CheckConstruct() {
    Runtime runtime;
    runtime.FillScalars(0x4000);
    Check(pal4::GameDataRoleRecordCopy::CopyConstruct(
              0x8000, 0x4000, runtime) == 0x8000 + 304 + 0x55 &&
              runtime.calls.front() ==
                  Call{"copy", 0x8000, 0x4000, 144} &&
              runtime.calls.back() ==
                  Call{"text", 0x8000 + 304, 0x4000 + 304} &&
              runtime.text_calls == 9,
          "copy constructor returns final text assignment and preserves order");
    runtime.CheckScalars(0x8000);
    std::vector<std::uint32_t> text_destinations;
    std::vector<std::uint32_t> flag_destinations;
    std::uint32_t middle_copy = 0;
    for (const auto& call : runtime.calls) {
        if (call.name == "text") text_destinations.push_back(call.destination);
        if (call.name == "byte") flag_destinations.push_back(call.destination);
        if (call.name == "copy" && call.count == 32) ++middle_copy;
    }
    Check(text_destinations == std::vector<std::uint32_t>{
              0x8090, 0x80a0, 0x80b0, 0x80c0,
              0x80f0, 0x8100, 0x8110, 0x8120, 0x8130} &&
              flag_destinations == text_destinations &&
              middle_copy == 1,
          "four middle and five tail text slots surround 32-byte scalars");
    for (std::uint32_t offset : {144u, 160u, 176u, 192u,
                                 240u, 256u, 272u, 288u, 304u}) {
        Check(runtime.bytes[0x8000 + offset] ==
                  runtime.bytes[0x4000 + offset],
              "text source flag copied");
        for (std::uint32_t i = 4; i < 16; ++i)
            Check(runtime.bytes[0x8000 + offset + i] == 0,
                  "new text slot pointers and lengths begin zeroed");
    }

    Runtime null_destination;
    Check(pal4::GameDataRoleRecordCopy::CopyConstruct(
              0, 0x4000, null_destination) == 0 &&
              null_destination.calls.empty(),
          "null destination skips all constructor work");
}

void CheckRangeAndBackward() {
    Runtime runtime;
    runtime.FillScalars(0x4000);
    Check(pal4::GameDataRoleRecordCopy::AssignRange(
              0x8000, 0x8280, 0x4000, runtime) == 0x8280 &&
              runtime.calls.size() == 22 &&
              runtime.calls[0] ==
                  Call{"copy", 0x8000, 0x4000, 144} &&
              runtime.calls[5] ==
                  Call{"copy", 0x80d0, 0x40d0, 32} &&
              runtime.calls[11] ==
                  Call{"copy", 0x8140, 0x4000, 144},
          "range assignment handles each 320-byte destination in order");
    runtime.CheckScalars(0x8000);
    runtime.CheckScalars(0x8140);
    for (const auto& call : runtime.calls)
        Check(call.name != "byte" && call.name != "word",
              "assignment does not reinitialize existing text slots");
    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordCopy::AssignRange(
              0x8000, 0x8000, 0x4000, runtime) == 0x8000 &&
              runtime.calls.empty(),
          "empty assignment returns the range end");

    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordCopy::AssignBackward(
              0x4000, 0x4280, 0x43c0, runtime) == 0x4140 &&
              runtime.calls.size() == 22 &&
              runtime.calls[0] ==
                  Call{"copy", 0x4280, 0x4140, 144} &&
              runtime.calls[11] ==
                  Call{"copy", 0x4140, 0x4000, 144},
          "overlapping assignment walks source records backward");
    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordCopy::AssignBackward(
              0x4000, 0x4000, 0x43c0, runtime) == 0x43c0 &&
              runtime.calls.empty(),
          "empty backward assignment returns unchanged destination end");
}

void CheckFailure() {
    Runtime runtime;
    runtime.fail_on_text_call = 3;
    bool threw = false;
    try {
        static_cast<void>(pal4::GameDataRoleRecordCopy::CopyConstruct(
            0x8000, 0x4000, runtime));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.text_calls == 3 &&
              runtime.calls.front().name == "copy" &&
              runtime.calls.back() ==
                  Call{"text", 0x80b0, 0x40b0},
          "text constructor failure stops before subsequent fields");
}

void CheckAssignOne() {
    Runtime runtime;
    runtime.FillScalars(0x4000);
    Check(pal4::GameDataRoleRecordCopy::AssignOne(
              0x8000, 0x4000, runtime) == 0x8000 &&
              runtime.calls.size() == 11 &&
              runtime.calls[0] ==
                  Call{"copy", 0x8000, 0x4000, 144} &&
              runtime.calls[5] ==
                  Call{"copy", 0x80d0, 0x40d0, 32} &&
              runtime.calls.back() ==
                  Call{"text", 0x8130, 0x4130},
          "single-record assignment returns destination after nine texts");
    runtime.CheckScalars(0x8000);
    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordCopy::AssignOne(
              0x8000, 0x8000, runtime) == 0x8000 &&
              runtime.calls.size() == 11 &&
              runtime.calls[1] ==
                  Call{"text", 0x8090, 0x8090},
          "self-assignment reaches the native text self path");
}

void CheckAssignTail() {
    Runtime runtime;
    for (std::uint32_t i = 64; i < 96; ++i)
        runtime.bytes[0x4000 + i] =
            static_cast<std::uint8_t>(i + 9);
    Check(pal4::GameDataRoleRecordCopy::AssignTail(
              0x8000, 0x4000, runtime) == 0x8000 &&
              runtime.calls.size() == 10 &&
              runtime.calls[0] == Call{"text", 0x8000, 0x4000} &&
              runtime.calls[3] == Call{"text", 0x8030, 0x4030} &&
              runtime.calls[4] == Call{"copy", 0x8040, 0x4040, 32} &&
              runtime.calls.back() == Call{"text", 0x80a0, 0x40a0},
          "tail assignment processes four texts, 32 scalar bytes and five texts");
    for (std::uint32_t i = 64; i < 96; ++i)
        Check(runtime.bytes[0x8000 + i] ==
                  static_cast<std::uint8_t>(i + 9),
              "tail scalar bytes preserved");
    runtime.calls.clear();
    Check(pal4::GameDataRoleRecordCopy::AssignTail(
              0x8000, 0x8000, runtime) == 0x8000 &&
              runtime.calls[0] == Call{"text", 0x8000, 0x8000},
          "tail self-assignment reaches native text self path");
}

void CheckUniqueCopy() {
    Runtime runtime;
    constexpr std::uint32_t begin = 0x4000;
    constexpr std::uint32_t output = 0x8000;
    constexpr std::uint32_t stride = pal4::GameDataRoleRecordCopy::kRecordBytes;
    const std::uint8_t keys[] = {1, 1, 2, 2, 1};
    for (std::uint32_t i = 0; i < 5; ++i) {
        runtime.bytes[begin + i * stride] = static_cast<std::uint8_t>(10 + i);
        runtime.bytes[begin + i * stride + 228] = keys[i];
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    const auto equivalent = [&](std::uint32_t left, std::uint32_t right) {
        comparisons.emplace_back(left, right);
        return runtime.bytes[left + 228] == runtime.bytes[right + 228];
    };
    Check(pal4::GameDataRoleRecordCopy::UniqueCopy(
              begin, begin + 5 * stride, output, equivalent, runtime) ==
              output + 3 * stride,
          "unique copy returns the end of the retained output range");
    Check(comparisons == std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                             {begin, begin + stride},
                             {begin, begin + 2 * stride},
                             {begin + 2 * stride, begin + 3 * stride},
                             {begin + 2 * stride, begin + 4 * stride}},
          "comparison uses the last retained source record");
    Check(runtime.calls.size() == 33 &&
              runtime.calls[0] == Call{"copy", output, begin, 144} &&
              runtime.calls[11] ==
                  Call{"copy", output + stride, begin + 2 * stride, 144} &&
              runtime.calls[22] ==
                  Call{"copy", output + 2 * stride, begin + 4 * stride, 144} &&
              runtime.calls[32] ==
                  Call{"text", output + 2 * stride + 304,
                       begin + 4 * stride + 304},
          "retained records receive full text-aware assignments");
    Check(runtime.bytes[output] == 10 &&
              runtime.bytes[output + stride] == 12 &&
              runtime.bytes[output + 2 * stride] == 14,
          "copies retain the first record of each adjacent run");

    runtime.calls.clear();
    comparisons.clear();
    Check(pal4::GameDataRoleRecordCopy::UniqueCopy(
              begin, begin, output, equivalent, runtime) == output &&
              runtime.calls.empty() && comparisons.empty(),
          "empty input does not copy or compare");
    Check(pal4::GameDataRoleRecordCopy::UniqueCopy(
              begin, begin + stride, output, equivalent, runtime) ==
              output + stride && runtime.calls.size() == 11 &&
              comparisons.empty(),
          "one record is copied without comparison");

    runtime.calls.clear();
    try {
        const auto fail = [](std::uint32_t, std::uint32_t) -> bool {
            throw std::runtime_error("compare failed");
        };
        static_cast<void>(pal4::GameDataRoleRecordCopy::UniqueCopy(
            begin, begin + 2 * stride, output, fail, runtime));
        Check(false, "comparison exception must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "compare failed" &&
                  runtime.calls.size() == 11,
              "failed comparison leaves only the first record assigned");
    }
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckRangeAndBackward();
        CheckFailure();
        CheckAssignOne();
        CheckAssignTail();
        CheckUniqueCopy();
        std::cout << "game_data_role_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
