#include "pal4/utilities_preview_record_copy.h"
#include "pal4/story_record_copy.h"

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
    std::vector<std::uint32_t> freed;
    std::uint32_t next_data = 0x7001u;
    std::uint32_t allocations = 0u;
    std::uint32_t set_size_calls = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    void Copy(const std::uint32_t destination,
              const std::uint32_t source,
              const std::uint32_t length) override {
        std::vector<std::uint8_t> snapshot;
        for (std::uint32_t i = 0; i < length; ++i)
            snapshot.push_back(ReadByte(source + i));
        for (std::uint32_t i = 0; i < length; ++i)
            WriteByte(destination + i, snapshot[i]);
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void ThrowLength() override { throw std::length_error("text length"); }
    void DetachAndReset(pal4::StringBufferState&) override {}
    bool AllocateText(pal4::StringBufferState& state,
                      const std::uint32_t length,
                      const std::uint8_t mode) override {
        if (length == 0u) {
            if (mode != 0u) SetErrorFlag(state, 1u);
            return false;
        }
        const auto old_data = state.data;
        if (old_data != 0u) SetErrorFlag(state, 1u);
        state = {next_data, 0u, length};
        next_data += 0x100u;
        ++allocations;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     const std::uint32_t length) override {
        ++set_size_calls;
        state.length = length;
        WriteByte(state.data + length, 0u);
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      const std::uint8_t flag) override {
        if (flag != 0u && state.data != 0u) {
            const auto reference = ReadByte(state.data - 1u);
            if (reference == 0u || reference == 0xffu)
                Free(state.data - 1u);
            else
                WriteByte(state.data - 1u,
                          static_cast<std::uint8_t>(reference - 1u));
        }
        state = {};
    }
    void Reallocate(pal4::StringBufferState& state,
                    const std::uint32_t length) override {
        const auto old_data = state.data;
        const auto old_length = state.length;
        const auto new_data = next_data;
        next_data += 0x100u;
        ++allocations;
        Copy(new_data, old_data, old_length < length ? old_length : length);
        if (old_data != 0u) SetErrorFlag(state, 1u);
        state = {new_data, old_length, length};
    }

    void PutField(const std::uint32_t record,
                  const std::uint32_t offset,
                  const std::uint8_t flag,
                  const std::uint32_t data,
                  const std::string& value,
                  const std::uint8_t reference) {
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
    std::string Text(const std::uint32_t record,
                     const std::uint32_t offset) {
        const auto data = ReadWord(record + offset + 4u);
        const auto length = ReadWord(record + offset + 8u);
        std::string result;
        for (std::uint32_t i = 0; i < length; ++i)
            result.push_back(static_cast<char>(ReadByte(data + i)));
        return result;
    }
};

void RepeatedAssignmentCopiesThreeTextFields() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 19u);
    runtime.PutField(0x2000u, 4u, 0x11u, 0x3001u, "alpha", 1u);
    runtime.PutField(0x2000u, 20u, 0x22u, 0x3101u, "beta", 1u);
    runtime.PutField(0x2000u, 36u, 0x33u, 0x3201u, "gamma", 1u);
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto record = 0x1000u + index * 52u;
        runtime.WriteWord(record, index + 1u);
        for (std::uint32_t field = 0; field < 3; ++field)
            runtime.PutField(record, kFields[field],
                             static_cast<std::uint8_t>(0xa0u + field),
                             0x4001u + index * 0x300u + field * 0x100u,
                             "old", 0u);
    }

    Check(pal4::UtilitiesPreviewRecordCopy::AssignRepeated(
              0x1000u, 0x1068u, 0x2000u, runtime) == 0x1068u,
          "repeated assignment returns end at 52-byte stride");
    for (const auto record : {0x1000u, 0x1034u}) {
        Check(runtime.ReadWord(record) == 19u &&
                  runtime.Text(record, 4u) == "alpha" &&
                  runtime.Text(record, 20u) == "beta" &&
                  runtime.Text(record, 36u) == "gamma" &&
                  runtime.ReadByte(record + 4u) == 0xa0u &&
                  runtime.ReadByte(record + 20u) == 0xa1u &&
                  runtime.ReadByte(record + 36u) == 0xa2u,
              "assignment copies key and texts while preserving flag bytes");
    }
    Check(runtime.ReadByte(0x3000u) == 3u &&
              runtime.ReadByte(0x3100u) == 3u &&
              runtime.ReadByte(0x3200u) == 3u &&
              runtime.freed.size() == 6u,
          "each text is shared and each old buffer is released");
    Check(pal4::UtilitiesPreviewRecordCopy::AssignRepeated(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x1000u,
          "empty assignment range is inert");
}

void SaturatedFieldsTakeIndependentCopies() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 8u);
    runtime.PutField(0x2000u, 4u, 0u, 0x3001u, "one", 0xfeu);
    runtime.PutField(0x2000u, 20u, 0u, 0x3101u, "two", 0xfeu);
    runtime.PutField(0x2000u, 36u, 0u, 0x3201u, "three", 0xffu);
    static_cast<void>(pal4::UtilitiesPreviewRecordCopy::AssignRepeated(
        0x1000u, 0x1034u, 0x2000u, runtime));
    Check(runtime.allocations == 3u &&
              runtime.Text(0x1000u, 4u) == "one" &&
              runtime.Text(0x1000u, 20u) == "two" &&
              runtime.Text(0x1000u, 36u) == "three" &&
              runtime.ReadWord(0x1008u) != 0x3001u &&
              runtime.ReadWord(0x1018u) != 0x3101u &&
              runtime.ReadWord(0x1028u) != 0x3201u &&
              runtime.ReadByte(0x3000u) == 0xfeu &&
              runtime.ReadByte(0x3100u) == 0xfeu &&
              runtime.ReadByte(0x3200u) == 0xffu,
          "saturated headers force three deep copies");
}

void ReverseAssignmentPreservesUnreadSource() {
    Runtime runtime;
    for (std::uint32_t index = 0; index < 3; ++index) {
        const auto record = 0x1000u + index * 52u;
        runtime.WriteWord(record, index + 1u);
        for (std::uint32_t field = 0; field < 3; ++field)
            runtime.PutField(record, kFields[field], 0u,
                             0x3001u + index * 0x300u + field * 0x100u,
                             std::to_string(index + 1u) +
                                 std::to_string(field + 1u), 0u);
    }
    Check(pal4::UtilitiesPreviewRecordCopy::AssignBackward(
              0x1000u, 0x1068u, 0x109cu, runtime) == 0x1034u,
          "reverse assignment returns destination start");
    for (std::uint32_t index = 1; index < 3; ++index) {
        const auto record = 0x1000u + index * 52u;
        Check(runtime.ReadWord(record) == index &&
                  runtime.Text(record, 4u) == std::to_string(index) + "1" &&
                  runtime.Text(record, 20u) == std::to_string(index) + "2" &&
                  runtime.Text(record, 36u) == std::to_string(index) + "3",
              "reverse movement keeps all three source fields");
    }
    Check(pal4::UtilitiesPreviewRecordCopy::AssignBackward(
              0x1000u, 0x1000u, 0x109cu, runtime) == 0x109cu,
          "empty reverse range leaves end unchanged");
}

void ConstructionCopiesFlagsAndText() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 88u);
    for (std::uint32_t field = 0; field < 3; ++field)
        runtime.PutField(0x2000u, kFields[field],
                         static_cast<std::uint8_t>(0x10u + field),
                         0x3001u + field * 0x100u,
                         std::to_string(field + 1u), 1u);
    pal4::UtilitiesPreviewRecordCopy::CopyConstruct(
        0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 88u &&
              runtime.allocations == 0u,
          "constructor copies key and shares ordinary strings");
    for (std::uint32_t field = 0; field < 3; ++field) {
        const auto offset = kFields[field];
        Check(runtime.ReadByte(0x1000u + offset) == 0x10u + field &&
                  runtime.Text(0x1000u, offset) ==
                      std::to_string(field + 1u) &&
                  runtime.ReadByte(0x3000u + field * 0x100u) == 2u,
              "constructor copies each flag and increments each reference");
    }
    pal4::UtilitiesPreviewRecordCopy::CopyConstruct(0u, 0x2000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 2u,
          "null destination performs no work");
    pal4::UtilitiesPreviewRecordCopy::CopyConstruct(
        0x2000u, 0x2000u, runtime);
    for (const auto offset : kFields)
        Check(runtime.ReadWord(0x2000u + offset + 4u) == 0u,
              "same-address construction sees initialized empty fields");
}

void CallbackConstructionUsesSecondFieldSetSize() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 31u);
    runtime.PutField(0x2000u, 4u, 0x71u, 0x3001u, "name", 1u);
    runtime.PutField(0x2000u, 20u, 0x72u, 0x3101u, "detail", 0xfeu);
    pal4::StoryRecordCopy::ConstructCallback(0x1000u, 0x2000u,
                                              runtime);
    Check(runtime.ReadWord(0x1000u) == 31u &&
              runtime.ReadByte(0x1004u) == 0x71u &&
              runtime.ReadByte(0x1014u) == 0x72u &&
              runtime.ReadWord(0x1008u) == 0x3001u &&
              runtime.Text(0x1000u, 4u) == "name" &&
              runtime.Text(0x1000u, 20u) == "detail" &&
              runtime.ReadWord(0x1018u) != 0x3101u &&
              runtime.ReadByte(0x3000u) == 2u &&
              runtime.ReadByte(0x3100u) == 0xfeu &&
              runtime.set_size_calls == 1u,
          "callback constructor shares first text and finalizes second copy");

    pal4::StoryRecordCopy::ConstructCallback(0u, 0x2000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 2u &&
              runtime.set_size_calls == 1u,
          "null callback destination performs no work");

    runtime.PutField(0x2100u, 4u, 0u, 0u, "", 0u);
    runtime.PutField(0x2100u, 20u, 0u, 0u, "", 0u);
    pal4::StoryRecordCopy::ConstructCallback(0x1100u, 0x2100u,
                                              runtime);
    Check(runtime.ReadWord(0x1108u) == 0u &&
              runtime.ReadWord(0x1118u) == 0u,
          "empty callback text leaves both states empty");
}

void SingleAssignmentCoversThreeFields() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 41u);
    for (std::uint32_t i = 0; i < 3u; ++i) {
        runtime.PutField(0x2000u, kFields[i], 0x11u,
                         0x3001u + i * 0x100u, "text", 1u);
        runtime.PutField(0x1000u, kFields[i], 0xa5u,
                         0x4001u + i * 0x100u, "old", 0u);
    }
    pal4::UtilitiesPreviewRecordCopy::AssignOne(0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 41u && runtime.freed.size() == 3u,
          "single PreviewInfo assignment copies key and releases old text");
    for (const auto offset : kFields)
        Check(runtime.Text(0x1000u, offset) == "text" &&
                  runtime.ReadByte(0x1000u + offset) == 0xa5u,
              "single PreviewInfo assignment covers all text fields");
}

void SelectUniqueKeepsPreviewFieldsTogether() {
    Runtime runtime;
    const std::array<std::uint32_t, 3> records{0x2000u, 0x2034u,
                                                0x2068u};
    const std::array<std::uint32_t, 3> keys{1u, 1u, 2u};
    const std::array<std::string, 3> values{"first", "skip", "second"};
    for (std::uint32_t i = 0; i < 3u; ++i) {
        runtime.WriteWord(records[i], keys[i]);
        for (std::uint32_t field = 0; field < 3u; ++field)
            runtime.PutField(records[i], kFields[field], 0x11u,
                             0x3001u + i * 0x400u + field * 0x100u,
                             values[i], 1u);
    }
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesPreviewRecordCopy::SelectUnique(
              0x2000u, 0x209cu, 0x1000u, equals, runtime) == 0x1068u,
          "PreviewInfo selection returns two-record output end");
    for (const auto field : kFields)
        Check(runtime.Text(0x1000u, field) == "first" &&
                  runtime.Text(0x1034u, field) == "second",
              "PreviewInfo selection carries each accepted text field");
    Check(runtime.ReadWord(0x1000u) == 1u &&
              runtime.ReadWord(0x1034u) == 2u &&
              pal4::UtilitiesPreviewRecordCopy::SelectUnique(
                  0x2000u, 0x2000u, 0x1000u, equals, runtime) == 0x1000u,
          "PreviewInfo selection handles keys and an empty range");
}

void SelectUniqueCompactsPreviewRecordsInPlace() {
    Runtime runtime;
    for (std::uint32_t i = 0; i < 3u; ++i) {
        const auto record = 0x2000u + i * 52u;
        runtime.WriteWord(record, i == 2u ? 2u : 1u);
        for (std::uint32_t field = 0; field < 3u; ++field)
            runtime.PutField(record, kFields[field], 0x11u,
                             0x3001u + i * 0x400u + field * 0x100u,
                             i == 2u ? "last" : "first", 0u);
    }
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesPreviewRecordCopy::SelectUnique(
              0x2000u, 0x209cu, 0x2000u, equals, runtime) == 0x2068u &&
              runtime.ReadWord(0x2034u) == 2u &&
              runtime.Text(0x2000u, 4u) == "first" &&
              runtime.Text(0x2034u, 36u) == "last",
          "in-place PreviewInfo compaction preserves all record fields");
}

}  // namespace

int main() {
    try {
        RepeatedAssignmentCopiesThreeTextFields();
        SaturatedFieldsTakeIndependentCopies();
        ReverseAssignmentPreservesUnreadSource();
        ConstructionCopiesFlagsAndText();
        CallbackConstructionUsesSecondFieldSetSize();
        SingleAssignmentCoversThreeFields();
        SelectUniqueKeepsPreviewFieldsTogether();
        SelectUniqueCompactsPreviewRecordsInPlace();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
