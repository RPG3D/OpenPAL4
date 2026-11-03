#include "pal4/player_manager_slot_data_copier.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PlayerManagerSlotCopyRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> allocations;
    std::vector<std::string> events;
    std::uint32_t erase_calls{};
    std::uint32_t allocate_calls{};
    std::uint32_t shared_calls{};
    std::uint32_t copy_calls{};

    std::uint32_t ReadDword(std::uint32_t address) override {
        return words[address];
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t EmptyTextAddress() override {
        events.emplace_back("empty-text");
        return pal4::PlayerManagerSlotDataCopier::kEmptyTextAddress;
    }
    void EraseText(std::uint32_t record, std::uint32_t start,
                   std::uint32_t count) override {
        events.push_back("erase:" + std::to_string(record) + ":" +
                         std::to_string(start) + ":" +
                         std::to_string(count));
        ++erase_calls;
    }
    void SetErrorFlag(std::uint32_t record,
                      std::uint8_t flag) override {
        Check(flag == 1u, "shared text sets flag one");
        events.push_back("share:" + std::to_string(record));
        ++shared_calls;
    }
    bool AllocateText(std::uint32_t record, std::uint32_t length,
                      std::uint8_t mode) override {
        Check(mode == 1u, "deep copy uses allocation mode one");
        events.push_back("allocate:" + std::to_string(record) + ":" +
                         std::to_string(length));
        ++allocate_calls;
        const auto it = allocations.find(record);
        if (it == allocations.end()) return false;
        words[record + 4u] = it->second;
        return true;
    }
    void CopyBytes(std::uint32_t destination, std::uint32_t source,
                   std::uint32_t count) override {
        events.emplace_back("copy");
        ++copy_calls;
        for (std::uint32_t i = 0u; i < count; ++i)
            bytes[destination + i] = bytes[source + i];
    }
};

void SetText(Runtime& runtime, std::uint32_t record,
             std::uint32_t data, std::uint32_t length,
             std::uint32_t capacity, std::uint8_t references) {
    runtime.words[record + 4u] = data;
    runtime.words[record + 8u] = length;
    runtime.words[record + 12u] = capacity;
    if (data != 0u) runtime.bytes[data - 1u] = references;
}

void MixedCopyPaths() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x9000u;
    constexpr std::uint32_t object = 0x5000u;
    constexpr std::uint32_t input = source + 0x90u;
    constexpr std::uint32_t output = object + 0x9f8u;

    SetText(runtime, input, 0x3000u, 3u, 5u, 2u);
    SetText(runtime, input + 0x10u, 0x3100u, 2u, 4u, 0xfeu);
    SetText(runtime, input + 0x20u, 0u, 1u, 6u, 0u);
    SetText(runtime, input + 0x30u, 0x3200u, 2u, 2u, 0xfeu);
    SetText(runtime, input + 0x60u, 0x3300u, 4u, 8u, 1u);
    runtime.bytes[pal4::PlayerManagerSlotDataCopier::kEmptyTextAddress - 1u] =
        7u;
    runtime.bytes[0x3100u] = 'A';
    runtime.bytes[0x3101u] = 'B';
    runtime.words[output + 0x34u] = 0xdeadu;
    runtime.allocations[output + 0x10u] = 0x4000u;

    for (std::uint32_t i = 0u; i < 8u; ++i)
        runtime.words[input + 0x40u + i * 4u] = 0xa000u + i;
    for (std::uint32_t i = 0u; i < 0x24u; ++i)
        runtime.words[source + i * 4u] = 0xb000u + i;

    Check(pal4::PlayerManagerSlotDataCopier::Copy(
              object, source, runtime) == object,
          "data copy returns the target object");
    Check(runtime.shared_calls == 3u && runtime.allocate_calls == 6u &&
              runtime.copy_calls == 1u && runtime.erase_calls == 0u,
          "nine records use three shared, six allocation, and one deep copy path");
    Check(runtime.words[output + 4u] == 0x3000u &&
              runtime.words[output + 8u] == 3u &&
              runtime.words[output + 12u] == 5u &&
              runtime.bytes[0x2fffu] == 3u,
          "shared text copies header fields and increments reference count");
    Check(runtime.words[output + 0x14u] == 0x4000u &&
              runtime.words[output + 0x18u] == 2u &&
              runtime.bytes[0x4000u] == 'A' &&
              runtime.bytes[0x4001u] == 'B' &&
              runtime.bytes[0x4002u] == 0u &&
              runtime.bytes[0x30ffu] == 0xfeu,
          "saturated source uses allocated deep copy and NUL termination");
    Check(runtime.words[output + 0x24u] ==
              pal4::PlayerManagerSlotDataCopier::kEmptyTextAddress &&
              runtime.bytes[
                  pal4::PlayerManagerSlotDataCopier::kEmptyTextAddress - 1u] ==
                  8u &&
              runtime.words[output + 0x34u] == 0xdeadu,
          "null source uses shared empty text; failed allocation preserves target");
    Check(runtime.words[output + 0x64u] == 0x3300u &&
              runtime.bytes[0x32ffu] == 2u,
          "second five-record group uses the same shared-text logic");
    for (std::uint32_t i = 0u; i < 8u; ++i)
        Check(runtime.words[output + 0x40u + i * 4u] == 0xa000u + i,
              "middle eight DWORDs are copied in order");
    for (std::uint32_t i = 0u; i < 0x24u; ++i)
        Check(runtime.words[object + 0x658u + i * 4u] == 0xb000u + i,
              "final 0x90-byte source prefix is copied to +0x658");
}

void SelfAssignmentErasesEveryRecord() {
    Runtime runtime;
    constexpr std::uint32_t object = 0x5000u;
    constexpr std::uint32_t source = object + 0x9f8u - 0x90u;
    constexpr std::uint32_t records = object + 0x9f8u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        runtime.words[records + i * 16u + 8u] = i + 1u;
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.words[records + 0x60u + i * 16u + 8u] = i + 5u;
    Check(pal4::PlayerManagerSlotDataCopier::Copy(
              object, source, runtime) == object &&
              runtime.erase_calls == 18u &&
              runtime.allocate_calls == 0u && runtime.shared_calls == 0u,
          "self assignment uses two erases for each of nine text records");
    Check(runtime.events[0] ==
              "erase:" + std::to_string(records) + ":1:4294967295" &&
              runtime.events[1] ==
              "erase:" + std::to_string(records) + ":0:0",
          "self assignment forwards size and sentinel erase arguments");
}

}  // namespace

int main() {
    MixedCopyPaths();
    SelfAssignmentErasesEveryRecord();
    std::cout << "2 player-manager data copy test groups passed\n";
}
