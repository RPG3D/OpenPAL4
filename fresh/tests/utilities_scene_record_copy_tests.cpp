#include "pal4/utilities_scene_record_copy.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::array<std::uint32_t, 3> kFields{4u, 20u, 36u};

struct Runtime final : pal4::IUtilitiesRecordTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::uint32_t next_data = 0x8001u;
    std::uint32_t allocations = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(std::uint32_t destination, std::uint32_t source,
              std::uint32_t length) override {
        std::vector<std::uint8_t> snapshot;
        for (std::uint32_t i = 0; i < length; ++i)
            snapshot.push_back(ReadByte(source + i));
        for (std::uint32_t i = 0; i < length; ++i)
            WriteByte(destination + i, snapshot[i]);
    }
    void Free(std::uint32_t) override {}
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void ThrowLength() override { throw std::length_error("text length"); }
    void DetachAndReset(pal4::StringBufferState&) override {}
    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length, std::uint8_t) override {
        if (length == 0u) {
            state = {};
            return false;
        }
        state = {next_data, 0u, length};
        next_data += 0x100u;
        ++allocations;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        state.length = length;
        WriteByte(state.data + length, 0u);
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t) override {
        state = {};
    }
    void Reallocate(pal4::StringBufferState& state,
                    std::uint32_t length) override {
        state = {next_data, 0u, length};
        next_data += 0x100u;
        ++allocations;
    }

    void PutField(std::uint32_t record, std::uint32_t offset,
                  std::uint8_t flag, std::uint32_t data,
                  const std::string& value, std::uint8_t reference) {
        WriteByte(record + offset, flag);
        WriteWord(record + offset + 4u, data);
        WriteWord(record + offset + 8u,
                  static_cast<std::uint32_t>(value.size()));
        WriteWord(record + offset + 12u,
                  static_cast<std::uint32_t>(value.size() + 4u));
        if (data == 0u) return;
        WriteByte(data - 1u, reference);
        for (std::uint32_t i = 0; i < value.size(); ++i)
            WriteByte(data + i, static_cast<std::uint8_t>(value[i]));
        WriteByte(data + value.size(), 0u);
    }
    std::string Text(std::uint32_t record, std::uint32_t offset) {
        const auto data = ReadWord(record + offset + 4u);
        const auto length = ReadWord(record + offset + 8u);
        std::string result;
        for (std::uint32_t i = 0; i < length; ++i)
            result.push_back(static_cast<char>(ReadByte(data + i)));
        return result;
    }
};

void PutScene(Runtime& runtime, std::uint32_t record,
              std::uint32_t key, std::uint32_t data_base,
              const std::string& prefix,
              std::uint8_t reference = 1u) {
    runtime.WriteWord(record, key);
    for (std::uint32_t i = 0; i < 3u; ++i)
        runtime.PutField(record, kFields[i],
                         static_cast<std::uint8_t>(0x20u + i),
                         data_base + 1u + i * 0x100u,
                         prefix + std::to_string(i), reference);
    runtime.WriteWord(record + 52u, key * 10u);
    runtime.WriteWord(record + 56u, key * 100u);
}

void RepeatedAssignmentPreservesFlagsAndCopiesTail() {
    Runtime runtime;
    PutScene(runtime, 0x2000u, 7u, 0x3000u, "source");
    PutScene(runtime, 0x1000u, 1u, 0x4000u, "old");
    PutScene(runtime, 0x103cu, 2u, 0x5000u, "old");
    runtime.WriteByte(0x1004u, 0xa1u);
    runtime.WriteByte(0x1040u, 0xa2u);
    Check(pal4::UtilitiesSceneRecordCopy::AssignRepeated(
              0x1000u, 0x1078u, 0x2000u, runtime) == 0x1078u,
          "Scene repeated assignment returns 60-byte range end");
    for (const auto record : {0x1000u, 0x103cu}) {
        Check(runtime.ReadWord(record) == 7u &&
                  runtime.ReadWord(record + 52u) == 70u &&
                  runtime.ReadWord(record + 56u) == 700u,
              "Scene assignment copies key and both tail words");
        for (std::uint32_t field = 0; field < 3u; ++field)
            Check(runtime.Text(record, kFields[field]) ==
                      "source" + std::to_string(field),
                  "Scene assignment copies every text field");
    }
    Check(runtime.ReadByte(0x3000u) == 3u &&
              runtime.ReadByte(0x3100u) == 3u &&
              runtime.ReadByte(0x3200u) == 3u &&
              runtime.ReadByte(0x1004u) == 0xa1u &&
              runtime.ReadByte(0x1040u) == 0xa2u,
          "Scene assignment shares text and leaves flag bytes untouched");
    Check(pal4::UtilitiesSceneRecordCopy::AssignRepeated(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x1000u,
          "empty Scene assignment is inert");
}

void ReverseAssignmentPreservesOverlappingTail() {
    Runtime runtime;
    PutScene(runtime, 0x1000u, 1u, 0x3000u, "first", 0u);
    PutScene(runtime, 0x103cu, 2u, 0x4000u, "second", 0u);
    PutScene(runtime, 0x1078u, 3u, 0x5000u, "third", 0u);
    Check(pal4::UtilitiesSceneRecordCopy::AssignBackward(
              0x1000u, 0x1078u, 0x10b4u, runtime) == 0x103cu,
          "Scene reverse assignment returns destination start");
    Check(runtime.ReadWord(0x103cu) == 1u &&
              runtime.ReadWord(0x1078u) == 2u &&
              runtime.ReadWord(0x1040u + 48u) == 10u &&
              runtime.ReadWord(0x1078u + 56u) == 200u &&
              runtime.Text(0x103cu, 4u) == "first0" &&
              runtime.Text(0x1078u, 36u) == "second2",
          "reverse movement preserves unread text and trailing values");
    Check(pal4::UtilitiesSceneRecordCopy::AssignBackward(
              0x1000u, 0x1000u, 0x10b4u, runtime) == 0x10b4u,
          "empty Scene reverse range is inert");
}

void ConstructionHandlesSharedAndSaturatedFields() {
    Runtime runtime;
    PutScene(runtime, 0x2000u, 9u, 0x3000u, "field");
    runtime.WriteByte(0x3100u, 0xfeu);
    pal4::UtilitiesSceneRecordCopy::CopyConstruct(
        0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 9u &&
              runtime.ReadWord(0x1034u) == 90u &&
              runtime.ReadWord(0x1038u) == 900u &&
              runtime.ReadByte(0x1004u) == 0x20u &&
              runtime.ReadByte(0x1014u) == 0x21u &&
              runtime.ReadByte(0x1024u) == 0x22u &&
              runtime.Text(0x1000u, 4u) == "field0" &&
              runtime.Text(0x1000u, 20u) == "field1" &&
              runtime.Text(0x1000u, 36u) == "field2" &&
              runtime.ReadWord(0x1018u) != 0x3101u &&
              runtime.ReadByte(0x3000u) == 2u &&
              runtime.ReadByte(0x3100u) == 0xfeu &&
              runtime.ReadByte(0x3200u) == 2u &&
              runtime.allocations == 1u,
          "Scene constructor copies flags and tails with mixed text ownership");
    pal4::UtilitiesSceneRecordCopy::CopyConstruct(0u, 0x2000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 2u,
          "null Scene destination performs no work");
}

void SingleAssignmentCopiesSceneTail() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 41u);
    runtime.WriteWord(0x2034u, 17u);
    runtime.WriteWord(0x2038u, 29u);
    pal4::UtilitiesSceneRecordCopy::AssignOne(0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 41u &&
              runtime.ReadWord(0x1034u) == 17u &&
              runtime.ReadWord(0x1038u) == 29u,
          "single Scene assignment copies key and both trailing words");
}

void SelectUniqueKeepsSceneTailWithItsText() {
    Runtime runtime;
    PutScene(runtime, 0x2000u, 1u, 0x3000u, "first");
    PutScene(runtime, 0x203cu, 1u, 0x3400u, "skip");
    PutScene(runtime, 0x2078u, 2u, 0x3800u, "second");
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesSceneRecordCopy::SelectUnique(
              0x2000u, 0x20b4u, 0x1000u, equals, runtime) == 0x1078u,
          "Scene selection returns two-record output end");
    Check(runtime.Text(0x1000u, 4u) == "first0" &&
              runtime.Text(0x103cu, 36u) == "second2" &&
              runtime.ReadWord(0x103cu) == 2u &&
              runtime.ReadWord(0x1070u) == 20u &&
              runtime.ReadWord(0x1074u) == 200u,
          "Scene selection keeps text and both tail fields in order");
    Check(pal4::UtilitiesSceneRecordCopy::SelectUnique(
              0x2000u, 0x2000u, 0x1000u, equals, runtime) == 0x1000u,
          "empty Scene selection returns its initial destination");
}

void SelectUniqueCompactsSceneRecordsInPlace() {
    Runtime runtime;
    PutScene(runtime, 0x2000u, 1u, 0x3000u, "first", 0u);
    PutScene(runtime, 0x203cu, 1u, 0x3400u, "skip", 0u);
    PutScene(runtime, 0x2078u, 2u, 0x3800u, "last", 0u);
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesSceneRecordCopy::SelectUnique(
              0x2000u, 0x20b4u, 0x2000u, equals, runtime) == 0x2078u &&
              runtime.ReadWord(0x203cu) == 2u &&
              runtime.Text(0x2000u, 4u) == "first0" &&
              runtime.Text(0x203cu, 36u) == "last2" &&
              runtime.ReadWord(0x2074u) == 200u,
          "in-place Scene compaction preserves text and trailing words");
}

}  // namespace

int main() {
    try {
        RepeatedAssignmentPreservesFlagsAndCopiesTail();
        ReverseAssignmentPreservesOverlappingTail();
        ConstructionHandlesSharedAndSaturatedFields();
        SingleAssignmentCopiesSceneTail();
        SelectUniqueKeepsSceneTailWithItsText();
        SelectUniqueCompactsSceneRecordsInPlace();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
