#include "pal4/skill_data_record_copy.h"

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
    std::uint32_t first;
    std::uint32_t second = 0;
    std::uint32_t third = 0;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    unsigned text_calls = 0;
    unsigned fail_text = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"word", address, value});
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    void CopyBytes(std::uint32_t destination,
                   std::uint32_t source,
                   std::uint32_t count) override {
        calls.push_back({"copy-bytes", destination, source, count});
        for (std::uint32_t offset = 0; offset < count; ++offset)
            bytes[destination + offset] = bytes[source + offset];
    }
    void ResetText(std::uint32_t destination) override {
        calls.push_back({"reset-text", destination});
        for (std::uint32_t offset : {4u, 8u, 12u})
            words[destination + offset] = 0;
    }
    std::uint32_t AssignText(std::uint32_t destination,
                             std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_text)
            throw std::runtime_error("text failed");
        for (std::uint32_t offset = 0; offset < 16; offset += 4)
            words[destination + offset] = words[source + offset];
        return destination + 7;
    }
};

void Seed(Runtime& runtime, std::uint32_t record,
          std::uint32_t value) {
    for (std::uint32_t offset = 0; offset < 132; offset += 4)
        runtime.words[record + offset] = value + offset;
}

void CheckRepeated() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    Check(pal4::SkillDataRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 264, 0x9000, runtime) ==
              0x5000 + 264 && runtime.calls.size() == 42,
          "two records receive seventeen words and four texts each");
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto record = 0x5000 + index * 132;
        const auto start = index * 21;
        Check(runtime.calls[start] == Call{"word", record, 100} &&
                  runtime.calls[start + 16] ==
                      Call{"word", record + 64, 164} &&
                  runtime.calls[start + 17] ==
                      Call{"text", record + 68, 0x9000 + 68} &&
                  runtime.calls[start + 18] ==
                      Call{"text", record + 84, 0x9000 + 84} &&
                  runtime.calls[start + 19] ==
                      Call{"text", record + 100, 0x9000 + 100} &&
                  runtime.calls[start + 20] ==
                      Call{"text", record + 116, 0x9000 + 116},
              "scalar and text fields follow native record order");
    }
    runtime.calls.clear();
    Check(pal4::SkillDataRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty range does not assign");
    Check(pal4::SkillDataRecordCopy::AssignRepeated(
              0x9000, 0x9000 + 132, 0x9000, runtime) ==
              0x9000 + 132 && runtime.calls.back() ==
                  Call{"text", 0x9000 + 116, 0x9000 + 116},
          "self-assignment reaches text ownership runtime");
}

void CheckBackward() {
    Runtime runtime;
    Seed(runtime, 0x5000, 100);
    Seed(runtime, 0x5000 + 132, 200);
    Check(pal4::SkillDataRecordCopy::AssignBackward(
              0x5000, 0x5000 + 264,
              0x5000 + 396, runtime) == 0x5000 + 132 &&
              runtime.calls.size() == 42 &&
              runtime.calls[0] ==
                  Call{"word", 0x5000 + 264, 200} &&
              runtime.calls[21] ==
                  Call{"word", 0x5000 + 132, 100},
          "backward copy starts with last source for overlapping shift");
    Check(runtime.words[0x5000 + 132] == 100 &&
              runtime.words[0x5000 + 264] == 200,
          "overlapping copy retains each source record");
    runtime.calls.clear();
    Check(pal4::SkillDataRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, runtime) == 0x7000 &&
              runtime.calls.empty(),
          "empty backward range returns destination end");
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    for (std::uint32_t offset = 20; offset <= 68; ++offset)
        runtime.bytes[0x9000 + offset] =
            static_cast<std::uint8_t>(offset);
    runtime.bytes[0x9000 + 84] = 7;
    runtime.bytes[0x9000 + 100] = 8;
    runtime.bytes[0x9000 + 116] = 9;
    Check(pal4::SkillDataRecordCopy::Construct(
              0x5000, 0x9000, runtime) == 0x5000 + 116 + 7 &&
              runtime.calls.size() == 23 &&
              runtime.calls[0] == Call{"word", 0x5000, 100} &&
              runtime.calls[4] == Call{"word", 0x5000 + 16, 116} &&
              runtime.calls[5] ==
                  Call{"copy-bytes", 0x5000 + 20, 0x9000 + 20, 49} &&
              runtime.calls[9] ==
                  Call{"text", 0x5000 + 68, 0x9000 + 68} &&
              runtime.calls[10] ==
                  Call{"byte", 0x5000 + 84, 7} &&
              runtime.calls[11] ==
                  Call{"reset-text", 0x5000 + 84} &&
              runtime.calls[12] ==
                  Call{"text", 0x5000 + 84, 0x9000 + 84} &&
              runtime.calls.back() ==
                  Call{"text", 0x5000 + 116, 0x9000 + 116} &&
              runtime.bytes[0x5000 + 68] == 68,
          "constructor copies 49-byte prefix then initializes four texts");
    runtime.calls.clear();
    Check(pal4::SkillDataRecordCopy::Construct(
              0, 0x9000, runtime) == 0 && runtime.calls.empty(),
          "null destination has no effects");
}

void CheckFailure() {
    Runtime repeated;
    Seed(repeated, 0x9000, 100);
    repeated.fail_text = 2;
    try {
        static_cast<void>(pal4::SkillDataRecordCopy::AssignRepeated(
            0x5000, 0x5000 + 264, 0x9000, repeated));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  repeated.calls.size() == 19 &&
                  repeated.calls.back() ==
                      Call{"text", 0x5000 + 84, 0x9000 + 84} &&
                  repeated.words[0x5000 + 132] == 0,
              "failed text assignment stops later fields and records");
    }

    Runtime constructed;
    Seed(constructed, 0x9000, 100);
    constructed.fail_text = 1;
    try {
        static_cast<void>(pal4::SkillDataRecordCopy::Construct(
            0x5000, 0x9000, constructed));
        Check(false, "constructor text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  constructed.calls.size() == 10 &&
                  constructed.calls.back() ==
                      Call{"text", 0x5000 + 68, 0x9000 + 68} &&
                  constructed.words[0x5000 + 72] == 0,
              "constructor initializes first text before assignment");
    }
}

}  // namespace

int main() {
    try {
        CheckRepeated();
        CheckBackward();
        CheckConstruct();
        CheckFailure();
        std::cout << "skill_data_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
