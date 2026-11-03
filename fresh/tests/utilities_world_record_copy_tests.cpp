#include "pal4/utilities_world_record_copy.h"

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

struct Runtime final : pal4::IUtilitiesRecordTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_data = 0x8001u;
    std::uint32_t allocations = 0u;
    std::uint32_t set_size_calls = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
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
    void Free(std::uint32_t address) override {
        freed.push_back(address);
    }
    void ThrowRange() override { throw std::out_of_range("text range"); }
    void ThrowLength() override { throw std::length_error("text length"); }
    void DetachAndReset(pal4::StringBufferState&) override {}
    bool AllocateText(pal4::StringBufferState& state,
                      std::uint32_t length, std::uint8_t mode) override {
        if (length == 0u) {
            if (mode != 0u) SetErrorFlag(state, 1u);
            return false;
        }
        if (state.data != 0u) SetErrorFlag(state, 1u);
        state = {next_data, 0u, length};
        next_data += 0x100u;
        ++allocations;
        return true;
    }
    void SetTextSize(pal4::StringBufferState& state,
                     std::uint32_t length) override {
        ++set_size_calls;
        state.length = length;
        WriteByte(state.data + length, 0u);
    }
    void SetErrorFlag(pal4::StringBufferState& state,
                      std::uint8_t flag) override {
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
                    std::uint32_t length) override {
        const auto old_data = state.data;
        const auto old_length = state.length;
        const auto new_data = next_data;
        next_data += 0x100u;
        ++allocations;
        Copy(new_data, old_data, old_length < length ? old_length : length);
        if (old_data != 0u) SetErrorFlag(state, 1u);
        state = {new_data, old_length, length};
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
    void PutRecord(std::uint32_t record, std::uint32_t key,
                   std::uint32_t data_base,
                   const std::string& prefix,
                   std::uint8_t reference = 1u) {
        WriteWord(record, key);
        PutField(record, 4u, 0x41u, data_base + 1u,
                 prefix + "A", reference);
        WriteWord(record + 20u, key * 10u);
        PutField(record, 24u, 0x42u, data_base + 0x101u,
                 prefix + "B", reference);
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

void RepeatedAssignmentSharesTextAndCopiesMiddleWord() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 7u, 0x3000u, "source");
    runtime.PutRecord(0x1000u, 1u, 0x4000u, "old", 0u);
    runtime.PutRecord(0x1028u, 2u, 0x5000u, "old", 0u);
    runtime.WriteByte(0x1004u, 0xa1u);
    runtime.WriteByte(0x102cu, 0xa2u);
    Check(pal4::UtilitiesWorldRecordCopy::AssignRepeated(
              0x1000u, 0x1050u, 0x2000u, runtime) == 0x1050u,
          "World repeated assignment returns 40-byte range end");
    for (const auto record : {0x1000u, 0x1028u})
        Check(runtime.ReadWord(record) == 7u &&
                  runtime.ReadWord(record + 20u) == 70u &&
                  runtime.Text(record, 4u) == "sourceA" &&
                  runtime.Text(record, 24u) == "sourceB",
              "World assignment copies key, middle word and both texts");
    Check(runtime.ReadByte(0x3000u) == 3u &&
              runtime.ReadByte(0x3100u) == 3u &&
              runtime.freed.size() == 4u &&
              runtime.ReadByte(0x1004u) == 0xa1u &&
              runtime.ReadByte(0x102cu) == 0xa2u,
          "World assignment shares text and preserves flag bytes");
    Check(pal4::UtilitiesWorldRecordCopy::AssignRepeated(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x1000u,
          "empty World range is inert");
}

void ReverseAssignmentPreservesOverlap() {
    Runtime runtime;
    runtime.PutRecord(0x1000u, 1u, 0x3000u, "first", 0u);
    runtime.PutRecord(0x1028u, 2u, 0x4000u, "second", 0u);
    runtime.PutRecord(0x1050u, 3u, 0x5000u, "third", 0u);
    Check(pal4::UtilitiesWorldRecordCopy::AssignBackward(
              0x1000u, 0x1050u, 0x1078u, runtime) == 0x1028u,
          "World reverse assignment returns destination start");
    Check(runtime.ReadWord(0x1028u) == 1u &&
              runtime.ReadWord(0x103cu) == 10u &&
              runtime.ReadWord(0x1050u) == 2u &&
              runtime.ReadWord(0x1064u) == 20u &&
              runtime.Text(0x1028u, 4u) == "firstA" &&
              runtime.Text(0x1050u, 24u) == "secondB",
          "World backward movement preserves unread fields");
    Check(pal4::UtilitiesWorldRecordCopy::AssignBackward(
              0x1000u, 0x1000u, 0x1078u, runtime) == 0x1078u,
          "empty World reverse range is inert");
}

void SaturatedAssignmentCallsSetTextSize() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 9u, 0x3000u, "deep", 0xfeu);
    static_cast<void>(pal4::UtilitiesWorldRecordCopy::AssignRepeated(
        0x1000u, 0x1028u, 0x2000u, runtime));
    Check(runtime.allocations == 2u &&
              runtime.set_size_calls == 2u &&
              runtime.Text(0x1000u, 4u) == "deepA" &&
              runtime.Text(0x1000u, 24u) == "deepB" &&
              runtime.ReadByte(0x3000u) == 0xfeu &&
              runtime.ReadByte(0x3100u) == 0xfeu,
          "World deep assignment finalizes both strings via SetTextSize");
}

void ConstructionUsesDistinctSecondFieldFinish() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 11u, 0x3000u, "text");
    runtime.WriteByte(0x3100u, 0xfeu);
    pal4::UtilitiesWorldRecordCopy::CopyConstruct(
        0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 11u &&
              runtime.ReadWord(0x1014u) == 110u &&
              runtime.ReadByte(0x1004u) == 0x41u &&
              runtime.ReadByte(0x1018u) == 0x42u &&
              runtime.Text(0x1000u, 4u) == "textA" &&
              runtime.Text(0x1000u, 24u) == "textB" &&
              runtime.ReadWord(0x1008u) == 0x3001u &&
              runtime.ReadWord(0x101cu) != 0x3101u &&
              runtime.ReadByte(0x3000u) == 2u &&
              runtime.ReadByte(0x3100u) == 0xfeu &&
              runtime.allocations == 1u &&
              runtime.set_size_calls == 0u,
          "World constructor copies flags and terminates second deep copy directly");
    pal4::UtilitiesWorldRecordCopy::CopyConstruct(0u, 0x2000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 2u,
          "null World destination performs no work");
}

void SingleAssignmentCopiesWorldMiddleWord() {
    Runtime runtime;
    runtime.WriteWord(0x2000u, 41u);
    runtime.WriteWord(0x2014u, 42u);
    pal4::UtilitiesWorldRecordCopy::AssignOne(0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 41u &&
              runtime.ReadWord(0x1014u) == 42u,
          "single World assignment copies key and middle word");
}

void SelectUniqueKeepsWorldTextAndMiddleWord() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 1u, 0x3000u, "first");
    runtime.PutRecord(0x2028u, 1u, 0x3400u, "skip");
    runtime.PutRecord(0x2050u, 2u, 0x3800u, "second", 0xfeu);
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesWorldRecordCopy::SelectUnique(
              0x2000u, 0x2078u, 0x1000u, equals, runtime) == 0x1050u,
          "World selection returns two-record output end");
    Check(runtime.ReadWord(0x1000u) == 1u &&
              runtime.ReadWord(0x1028u) == 2u &&
              runtime.ReadWord(0x103cu) == 20u &&
              runtime.Text(0x1000u, 4u) == "firstA" &&
              runtime.Text(0x1028u, 24u) == "secondB" &&
              runtime.set_size_calls == 0u,
          "World selection copies both text fields and directly terminates saturated copies");
    Check(pal4::UtilitiesWorldRecordCopy::SelectUnique(
              0x2000u, 0x2000u, 0x1000u, equals, runtime) == 0x1000u,
          "empty World selection returns initial destination");
}

void SelectUniqueCompactsWorldRecordsInPlace() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 1u, 0x3000u, "first", 0u);
    runtime.PutRecord(0x2028u, 1u, 0x3400u, "skip", 0u);
    runtime.PutRecord(0x2050u, 2u, 0x3800u, "last", 0u);
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesWorldRecordCopy::SelectUnique(
              0x2000u, 0x2078u, 0x2000u, equals, runtime) == 0x2050u &&
              runtime.ReadWord(0x2028u) == 2u &&
              runtime.Text(0x2000u, 4u) == "firstA" &&
              runtime.Text(0x2028u, 24u) == "lastB" &&
              runtime.ReadWord(0x203cu) == 20u,
          "in-place World compaction preserves text and middle word");
}

}  // namespace

int main() {
    try {
        RepeatedAssignmentSharesTextAndCopiesMiddleWord();
        ReverseAssignmentPreservesOverlap();
        SaturatedAssignmentCallsSetTextSize();
        ConstructionUsesDistinctSecondFieldFinish();
        SingleAssignmentCopiesWorldMiddleWord();
        SelectUniqueKeepsWorldTextAndMiddleWord();
        SelectUniqueCompactsWorldRecordsInPlace();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
