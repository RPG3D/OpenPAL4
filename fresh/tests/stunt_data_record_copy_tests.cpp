#include "pal4/stunt_data_record_copy.h"

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
    for (std::uint32_t offset = 0; offset < 208; offset += 4)
        runtime.words[record + offset] = value + offset;
}

void CheckRepeated() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    Check(pal4::StuntDataRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 416, 0x9000, runtime) ==
              0x5000 + 416 && runtime.calls.size() == 62,
          "two Stunt records receive scalar and seven text fields");
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto record = 0x5000 + index * 208;
        const auto start = index * 31;
        Check(runtime.calls[start] == Call{"word", record, 100} &&
                  runtime.calls[start + 1] ==
                      Call{"word", record + 4, 104} &&
                  runtime.calls[start + 2] ==
                      Call{"text", record + 8, 0x9000 + 8} &&
                  runtime.calls[start + 4] ==
                      Call{"text", record + 40, 0x9000 + 40} &&
                  runtime.calls[start + 5] ==
                      Call{"word", record + 56, 156} &&
                  runtime.calls[start + 26] ==
                      Call{"word", record + 140, 240} &&
                  runtime.calls[start + 27] ==
                      Call{"text", record + 144, 0x9000 + 144} &&
                  runtime.calls[start + 30] ==
                      Call{"text", record + 192, 0x9000 + 192},
              "Stunt scalar and text regions retain native order");
    }
    runtime.calls.clear();
    Check(pal4::StuntDataRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty range performs no assignment");
}

void CheckBackward() {
    Runtime runtime;
    Seed(runtime, 0x5000, 100);
    Seed(runtime, 0x5000 + 208, 200);
    Check(pal4::StuntDataRecordCopy::AssignBackward(
              0x5000, 0x5000 + 416,
              0x5000 + 624, runtime) == 0x5000 + 208 &&
              runtime.calls.size() == 62 &&
              runtime.calls[0] ==
                  Call{"word", 0x5000 + 416, 200} &&
              runtime.calls[31] ==
                  Call{"word", 0x5000 + 208, 100},
          "backward copy begins with last source record");
    Check(runtime.words[0x5000 + 208] == 100 &&
              runtime.words[0x5000 + 416] == 200,
          "overlapping move preserves original source values");
    runtime.calls.clear();
    Check(pal4::StuntDataRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, runtime) == 0x7000 &&
              runtime.calls.empty(),
          "empty backward range returns destination end");
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    for (std::uint32_t offset = 72; offset <= 144; ++offset)
        runtime.bytes[0x9000 + offset] =
            static_cast<std::uint8_t>(offset);
    for (const auto offset : {8u, 24u, 40u, 160u, 176u, 192u})
        runtime.bytes[0x9000 + offset] =
            static_cast<std::uint8_t>(offset);
    Check(pal4::StuntDataRecordCopy::Construct(
              0x5000, 0x9000, runtime) == 0x5000 + 192 + 7 &&
              runtime.calls.size() == 28 &&
              runtime.calls[0] == Call{"word", 0x5000, 100} &&
              runtime.calls[2] == Call{"byte", 0x5000 + 8, 8} &&
              runtime.calls[3] ==
                  Call{"reset-text", 0x5000 + 8} &&
              runtime.calls[4] ==
                  Call{"text", 0x5000 + 8, 0x9000 + 8} &&
              runtime.calls[11] ==
                  Call{"word", 0x5000 + 56, 156} &&
              runtime.calls[15] ==
                  Call{"copy-bytes", 0x5000 + 72,
                       0x9000 + 72, 20} &&
              runtime.calls[16] ==
                  Call{"copy-bytes", 0x5000 + 92,
                       0x9000 + 92, 53} &&
              runtime.calls[17] ==
                  Call{"reset-text", 0x5000 + 144} &&
              runtime.calls[18] ==
                  Call{"text", 0x5000 + 144, 0x9000 + 144} &&
              runtime.calls.back() ==
                  Call{"text", 0x5000 + 192, 0x9000 + 192} &&
              runtime.bytes[0x5000 + 144] == 144,
          "constructor initializes seven texts and copies scalar spans");
    runtime.calls.clear();
    Check(pal4::StuntDataRecordCopy::Construct(
              0, 0x9000, runtime) == 0 && runtime.calls.empty(),
          "null destination performs no construction");
}

void CheckFailure() {
    Runtime assigned;
    Seed(assigned, 0x9000, 100);
    assigned.fail_text = 2;
    try {
        static_cast<void>(pal4::StuntDataRecordCopy::AssignRepeated(
            0x5000, 0x5000 + 416, 0x9000, assigned));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  assigned.calls.size() == 4 &&
                  assigned.calls.back() ==
                      Call{"text", 0x5000 + 24, 0x9000 + 24} &&
                  assigned.words[0x5000 + 56] == 0,
              "failure stops before the middle scalar region");
    }

    Runtime constructed;
    Seed(constructed, 0x9000, 100);
    constructed.fail_text = 1;
    try {
        static_cast<void>(pal4::StuntDataRecordCopy::Construct(
            0x5000, 0x9000, constructed));
        Check(false, "constructor text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  constructed.calls.size() == 5 &&
                  constructed.calls.back() ==
                      Call{"text", 0x5000 + 8, 0x9000 + 8},
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
        std::cout << "stunt_data_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
