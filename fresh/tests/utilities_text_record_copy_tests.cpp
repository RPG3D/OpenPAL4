#include "pal4/utilities_text_record_copy.h"

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

struct Runtime final : pal4::IUtilitiesTextRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> freed;
    std::uint32_t next_data = 0x5001u;
    std::uint32_t allocations = 0u;
    std::uint32_t detach_calls = 0u;
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
    void DetachAndReset(pal4::StringBufferState&) override {
        ++detach_calls;
    }
    bool AllocateText(pal4::StringBufferState& state,
                      const std::uint32_t length,
                      const std::uint8_t mode) override {
        if (length == 0u) {
            if (mode != 0u) {
                if (state.data != 0u) Free(state.data - 1u);
                state = {};
            }
            return false;
        }
        state.data = next_data;
        state.capacity = length;
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
                      const std::uint8_t) override {
        state = {};
    }
    void Reallocate(pal4::StringBufferState& state,
                    const std::uint32_t length) override {
        const auto old_data = state.data;
        state.data = next_data;
        state.capacity = length;
        next_data += 0x100u;
        ++allocations;
        Copy(state.data, old_data, state.length < length
                                  ? state.length : length);
    }

    void PutRecord(const std::uint32_t record, const std::uint32_t key,
                   const std::uint32_t data, const std::string& value,
                   const std::uint8_t reference) {
        words[record] = key;
        bytes[record + 4u] = 0x5au;
        words[record + 8u] = data;
        words[record + 12u] = static_cast<std::uint32_t>(value.size());
        words[record + 16u] = static_cast<std::uint32_t>(value.size() + 4u);
        if (data != 0u) {
            bytes[data - 1u] = reference;
            for (std::uint32_t i = 0; i < value.size(); ++i)
                bytes[data + i] = static_cast<std::uint8_t>(value[i]);
            bytes[data + value.size()] = 0u;
        }
    }
    std::string Text(const std::uint32_t record) {
        std::string result;
        const auto data = ReadWord(record + 8u);
        const auto length = ReadWord(record + 12u);
        for (std::uint32_t i = 0; i < length; ++i)
            result.push_back(static_cast<char>(ReadByte(data + i)));
        return result;
    }
};

void RepeatedAssignmentSharesAndPreservesFlag() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 17u, 0x3001u, "abc", 1u);
    runtime.PutRecord(0x1000u, 2u, 0x3101u, "old", 0u);
    runtime.PutRecord(0x1014u, 3u, 0x3201u, "old", 0u);
    runtime.WriteByte(0x1004u, 0x11u);
    runtime.WriteByte(0x1018u, 0x22u);

    Check(pal4::UtilitiesTextRecordCopy::AssignRepeated(
              0x1000u, 0x1028u, 0x2000u, runtime) == 0x1028u,
          "repeated assignment returns the range end");
    for (const auto record : {0x1000u, 0x1014u}) {
        Check(runtime.ReadWord(record) == 17u &&
                  runtime.ReadWord(record + 8u) == 0x3001u &&
                  runtime.ReadWord(record + 12u) == 3u &&
                  runtime.ReadWord(record + 16u) == 7u &&
                  runtime.Text(record) == "abc",
              "repeated assignment copies key and shares full text state");
    }
    Check(runtime.ReadByte(0x3000u) == 3u &&
              runtime.freed == std::vector<std::uint32_t>{0x3100u, 0x3200u} &&
              runtime.ReadByte(0x1004u) == 0x11u &&
              runtime.ReadByte(0x1018u) == 0x22u,
          "reference count, release and untouched flags");
    Check(pal4::UtilitiesTextRecordCopy::AssignRepeated(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x1000u,
          "empty assignment range is inert");
}

void SaturatedReferenceCopiesBytes() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 9u, 0x3001u, "xyz", 0xfeu);
    runtime.PutRecord(0x1000u, 1u, 0u, "", 0u);
    static_cast<void>(pal4::UtilitiesTextRecordCopy::AssignRepeated(
        0x1000u, 0x1014u, 0x2000u, runtime));
    Check(runtime.ReadWord(0x1008u) == 0x5001u &&
              runtime.Text(0x1000u) == "xyz" &&
              runtime.ReadByte(0x5004u) == 0u &&
              runtime.ReadByte(0x3000u) == 0xfeu,
          "saturated header takes an independent terminated copy");
}

void EmptySourceReleasesExistingText() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 8u, 0u, "", 0u);
    runtime.PutRecord(0x1000u, 3u, 0x3001u, "old", 0u);
    static_cast<void>(pal4::UtilitiesTextRecordCopy::AssignRepeated(
        0x1000u, 0x1014u, 0x2000u, runtime));
    Check(runtime.ReadWord(0x1000u) == 8u &&
              runtime.ReadWord(0x1008u) == 0u &&
              runtime.ReadWord(0x100cu) == 0u &&
              runtime.freed == std::vector<std::uint32_t>{0x3000u},
          "empty source releases destination text and clears its state");
}

void ReverseAssignmentHandlesOverlap() {
    Runtime runtime;
    runtime.PutRecord(0x1000u, 1u, 0x3001u, "one", 0u);
    runtime.PutRecord(0x1014u, 2u, 0x3101u, "two", 0u);
    runtime.PutRecord(0x1028u, 3u, 0x3201u, "old", 0u);
    Check(pal4::UtilitiesTextRecordCopy::AssignBackward(
              0x1000u, 0x1028u, 0x103cu, runtime) == 0x1014u,
          "reverse assignment returns final destination start");
    Check(runtime.ReadWord(0x1014u) == 1u &&
              runtime.ReadWord(0x1028u) == 2u &&
              runtime.Text(0x1014u) == "one" &&
              runtime.Text(0x1028u) == "two",
          "overlap moves records backward without overwriting unread source");
    Check(pal4::UtilitiesTextRecordCopy::AssignBackward(
              0x1000u, 0x1000u, 0x103cu, runtime) == 0x103cu,
          "empty reverse range leaves destination end unchanged");
}

void ConstructionSharesOrCopies() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 42u, 0x3001u, "abc", 1u);
    runtime.WriteByte(0x2004u, 0x7bu);
    pal4::UtilitiesTextRecordCopy::CopyConstruct(0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 42u &&
              runtime.ReadByte(0x1004u) == 0x7bu &&
              runtime.ReadWord(0x1008u) == 0x3001u &&
              runtime.ReadWord(0x100cu) == 3u &&
              runtime.ReadWord(0x1010u) == 7u &&
              runtime.ReadByte(0x3000u) == 2u && runtime.allocations == 0u,
          "constructor copies scalar fields and shares ordinary text");

    runtime.PutRecord(0x2100u, 7u, 0x3101u, "deep", 0xfeu);
    pal4::UtilitiesTextRecordCopy::CopyConstruct(0x1100u, 0x2100u, runtime);
    Check(runtime.ReadWord(0x1108u) == 0x5001u &&
              runtime.Text(0x1100u) == "deep" &&
              runtime.ReadByte(0x5005u) == 0u &&
              runtime.ReadByte(0x3100u) == 0xfeu,
          "saturated source is copied and null terminated");

    runtime.PutRecord(0x2200u, 8u, 0u, "", 0u);
    pal4::UtilitiesTextRecordCopy::CopyConstruct(0x1200u, 0x2200u, runtime);
    Check(runtime.ReadWord(0x1200u) == 8u &&
              runtime.ReadWord(0x1208u) == 0u &&
              runtime.ReadWord(0x120cu) == 0u,
          "empty source leaves text state empty");
    pal4::UtilitiesTextRecordCopy::CopyConstruct(0u, 0x2000u, runtime);
    Check(runtime.ReadByte(0x3000u) == 2u,
          "null destination performs no work");

    runtime.PutRecord(0x1300u, 12u, 0x3201u, "same", 0u);
    pal4::UtilitiesTextRecordCopy::CopyConstruct(0x1300u, 0x1300u,
                                                  runtime);
    Check(runtime.ReadWord(0x1300u) == 12u &&
              runtime.ReadWord(0x1308u) == 0u &&
              runtime.detach_calls == 2u,
          "same-address construction observes initialized empty text");
}

void SelfAssignmentUsesStringSelfPath() {
    Runtime runtime;
    runtime.PutRecord(0x1000u, 4u, 0x3001u, "self", 0u);
    static_cast<void>(pal4::UtilitiesTextRecordCopy::AssignRepeated(
        0x1000u, 0x1014u, 0x1000u, runtime));
    Check(runtime.detach_calls == 2u &&
              runtime.ReadWord(0x1000u) == 4u,
          "self assignment takes the string self branch");
}

void SingleAssignmentUsesTheTextRecordLayout() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 31u, 0x3001u, "single", 1u);
    runtime.PutRecord(0x1000u, 2u, 0x3101u, "old", 0u);
    runtime.WriteByte(0x1004u, 0xa5u);
    pal4::UtilitiesTextRecordCopy::AssignOne(0x1000u, 0x2000u, runtime);
    Check(runtime.ReadWord(0x1000u) == 31u &&
              runtime.Text(0x1000u) == "single" &&
              runtime.ReadWord(0x1008u) == 0x3001u &&
              runtime.ReadByte(0x1004u) == 0xa5u &&
              runtime.ReadByte(0x3000u) == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x3100u},
          "single TextRecord assignment shares text and preserves its flag");
}

void SelectUniqueKeepsTheFirstOfEachRun() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 1u, 0x3001u, "first", 0xfeu);
    runtime.PutRecord(0x2014u, 1u, 0x3101u, "skip", 1u);
    runtime.PutRecord(0x2028u, 2u, 0x3201u, "second", 0xfeu);
    std::vector<std::uint32_t> compared;
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        compared.push_back(previous);
        compared.push_back(candidate);
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesTextRecordCopy::SelectUnique(
              0x2000u, 0x203cu, 0x1000u, equals, runtime) == 0x1028u,
          "TextRecord selection returns the output end");
    Check(runtime.ReadWord(0x1000u) == 1u &&
              runtime.ReadWord(0x1014u) == 2u &&
              runtime.Text(0x1000u) == "first" &&
              runtime.Text(0x1014u) == "second" &&
              runtime.set_size_calls == 1u &&
              compared == std::vector<std::uint32_t>{
                  0x2000u, 0x2014u, 0x2000u, 0x2028u},
          "selection compares against the last retained record and finalizes later copies");
    Check(pal4::UtilitiesTextRecordCopy::SelectUnique(
              0x2000u, 0x2000u, 0x1000u, equals, runtime) == 0x1000u &&
              compared.size() == 4u,
          "empty TextRecord selection performs no comparison");
}

void SelectUniqueCompactsTextRecordsInPlace() {
    Runtime runtime;
    runtime.PutRecord(0x2000u, 1u, 0x3001u, "first", 0u);
    runtime.PutRecord(0x2014u, 1u, 0x3101u, "skip", 0u);
    runtime.PutRecord(0x2028u, 2u, 0x3201u, "second", 0u);
    const auto equals = [&](const std::uint32_t previous,
                            const std::uint32_t candidate) {
        return runtime.ReadWord(previous) == runtime.ReadWord(candidate);
    };
    Check(pal4::UtilitiesTextRecordCopy::SelectUnique(
              0x2000u, 0x203cu, 0x2000u, equals, runtime) == 0x2028u &&
              runtime.ReadWord(0x2014u) == 2u &&
              runtime.Text(0x2000u) == "first" &&
              runtime.Text(0x2014u) == "second",
          "in-place TextRecord compaction preserves self and shifted text");
}

}  // namespace

int main() {
    try {
        RepeatedAssignmentSharesAndPreservesFlag();
        SaturatedReferenceCopiesBytes();
        EmptySourceReleasesExistingText();
        ReverseAssignmentHandlesOverlap();
        ConstructionSharesOrCopies();
        SelfAssignmentUsesStringSelfPath();
        SingleAssignmentUsesTheTextRecordLayout();
        SelectUniqueKeepsTheFirstOfEachRun();
        SelectUniqueCompactsTextRecordsInPlace();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
