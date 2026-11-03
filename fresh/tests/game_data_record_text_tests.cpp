#include "pal4/game_data_record_text.h"

#include <array>
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

struct Call {
    std::string operation;
    std::uint32_t address;
    std::uint32_t value = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataRecordTextRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    bool fail_free = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back({"read-byte", address});
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"write-word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"write-byte", address, value});
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back({"free", address});
        if (fail_free) throw std::runtime_error("free failed");
        return address + 9;
    }
    void ResetNested(std::uint32_t object) override {
        calls.push_back({"reset-nested", object});
    }
};

constexpr std::uint32_t kRecord = 0x9000;

template <std::size_t N>
void CheckRecordLayout(
    const std::array<std::uint32_t, N>& fields,
    std::uint32_t nested,
    std::uint32_t (*release)(std::uint32_t,
                             pal4::IGameDataRecordTextRuntime&)) {
    Runtime runtime;
    std::vector<Call> expected;
    if (nested) expected.push_back({"reset-nested", kRecord + nested});
    for (std::size_t index = 0; index < fields.size(); ++index) {
        const auto field = kRecord + fields[index];
        const auto header = 0x5000 +
            static_cast<std::uint32_t>(index) * 0x100;
        runtime.words[field] = header + 1;
        runtime.words[field + 4] = 10;
        runtime.words[field + 8] = 20;
        runtime.bytes[header] = index % 2 == 0 ? 2 : 0xff;
        expected.push_back({"read-byte", header});
        if (index % 2 == 0)
            expected.push_back({"write-byte", header, 1});
        else
            expected.push_back({"free", header});
        for (std::uint32_t offset = 0; offset < 12; offset += 4)
            expected.push_back({"write-word", field + offset, 0});
    }
    const auto last_header = 0x5000 +
        static_cast<std::uint32_t>(fields.size() - 1) * 0x100;
    const auto expected_result = fields.size() % 2 == 0
        ? last_header + 9 : last_header;
    Check(release(kRecord, runtime) == expected_result &&
              runtime.calls == expected,
          "record releases exact text fields in native order");
    for (const auto offset : fields) {
        Check(runtime.words[kRecord + offset] == 0 &&
                  runtime.words[kRecord + offset + 4] == 0 &&
                  runtime.words[kRecord + offset + 8] == 0,
              "each text triplet is cleared");
    }
}

void CheckLayouts() {
    CheckRecordLayout(std::array<std::uint32_t, 4>{
                          120, 104, 88, 72}, 0,
                      pal4::SkillDataRecordText::Release);
    CheckRecordLayout(std::array<std::uint32_t, 6>{
                          180, 164, 148, 44, 28, 12}, 192,
                      pal4::StuntDataRecordText::Release);
    CheckRecordLayout(std::array<std::uint32_t, 6>{
                          124, 108, 92, 48, 32, 16}, 136,
                      pal4::MagicDataRecordText::Release);
}

void CheckNullAndRefcounts() {
    Runtime empty;
    Check(pal4::SkillDataRecordText::Release(kRecord, empty) == 0 &&
              empty.calls == std::vector<Call>{
                  {"write-word", kRecord + 120, 0},
                  {"write-word", kRecord + 124, 0},
                  {"write-word", kRecord + 128, 0},
                  {"write-word", kRecord + 104, 0},
                  {"write-word", kRecord + 108, 0},
                  {"write-word", kRecord + 112, 0},
                  {"write-word", kRecord + 88, 0},
                  {"write-word", kRecord + 92, 0},
                  {"write-word", kRecord + 96, 0},
                  {"write-word", kRecord + 72, 0},
                  {"write-word", kRecord + 76, 0},
                  {"write-word", kRecord + 80, 0}},
          "null fields clear metadata without reading references");

    Runtime zero_reference;
    zero_reference.words[kRecord + 72] = 0x5001;
    zero_reference.bytes[0x5000] = 0;
    Check(pal4::SkillDataRecordText::Release(
              kRecord, zero_reference) == 0x5009 &&
              zero_reference.calls[zero_reference.calls.size() - 4] ==
                  Call{"free", 0x5000},
          "zero reference frees header and forwards final free result");
}

void CheckFailureOrder() {
    Runtime runtime;
    runtime.words[kRecord + 180] = 0x5001;
    runtime.bytes[0x5000] = 0xff;
    runtime.fail_free = true;
    try {
        static_cast<void>(pal4::StuntDataRecordText::Release(
            kRecord, runtime));
        Check(false, "free failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "free failed" &&
                  runtime.calls == std::vector<Call>{
                      {"reset-nested", kRecord + 192},
                      {"read-byte", 0x5000}, {"free", 0x5000}} &&
                  runtime.words[kRecord + 180] == 0x5001,
              "failure stops before clearing the active field");
    }
}

}  // namespace

int main() {
    try {
        CheckLayouts();
        CheckNullAndRefcounts();
        CheckFailureOrder();
        std::cout << "game_data_record_text_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
