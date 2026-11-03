#include "pal4/magic_data_record_copy.h"

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
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {
        throw std::runtime_error("Magic constructor has no raw byte span");
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
    for (std::uint32_t offset = 0; offset < 152; offset += 4)
        runtime.words[record + offset] = value + offset;
}

void CheckRepeated() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    Check(pal4::MagicDataRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 304, 0x9000, runtime) ==
              0x5000 + 304 && runtime.calls.size() == 34,
          "two Magic records receive ten scalars and seven texts each");
    for (std::uint32_t index = 0; index < 2; ++index) {
        const auto record = 0x5000 + index * 152;
        const auto start = index * 17;
        Check(runtime.calls[start] == Call{"word", record, 100} &&
                  runtime.calls[start + 2] ==
                      Call{"word", record + 8, 108} &&
                  runtime.calls[start + 3] ==
                      Call{"text", record + 12, 0x9000 + 12} &&
                  runtime.calls[start + 5] ==
                      Call{"text", record + 44, 0x9000 + 44} &&
                  runtime.calls[start + 6] ==
                      Call{"word", record + 60, 160} &&
                  runtime.calls[start + 12] ==
                      Call{"word", record + 84, 184} &&
                  runtime.calls[start + 13] ==
                      Call{"text", record + 88, 0x9000 + 88} &&
                  runtime.calls[start + 16] ==
                      Call{"text", record + 136, 0x9000 + 136},
              "Magic field regions retain native assignment order");
    }
    runtime.calls.clear();
    Check(pal4::MagicDataRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty range performs no assignment");
}

void CheckBackward() {
    Runtime runtime;
    Seed(runtime, 0x5000, 100);
    Seed(runtime, 0x5000 + 152, 200);
    Check(pal4::MagicDataRecordCopy::AssignBackward(
              0x5000, 0x5000 + 304,
              0x5000 + 456, runtime) == 0x5000 + 152 &&
              runtime.calls.size() == 34 &&
              runtime.calls[0] ==
                  Call{"word", 0x5000 + 304, 200} &&
              runtime.calls[17] ==
                  Call{"word", 0x5000 + 152, 100},
          "backward copy begins with last source record");
    Check(runtime.words[0x5000 + 152] == 100 &&
              runtime.words[0x5000 + 304] == 200,
          "overlapping move preserves source values");
    runtime.calls.clear();
    Check(pal4::MagicDataRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, runtime) == 0x7000 &&
              runtime.calls.empty(),
          "empty backward range returns destination end");
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    for (const auto offset : {12u, 28u, 44u, 88u,
                              104u, 120u, 136u})
        runtime.bytes[0x9000 + offset] =
            static_cast<std::uint8_t>(offset);
    Check(pal4::MagicDataRecordCopy::Construct(
              0x5000, 0x9000, runtime) == 0x5000 + 136 + 7 &&
              runtime.calls.size() == 33 &&
              runtime.calls[0] == Call{"word", 0x5000, 100} &&
              runtime.calls[2] ==
                  Call{"word", 0x5000 + 8, 108} &&
              runtime.calls[3] ==
                  Call{"byte", 0x5000 + 12, 12} &&
              runtime.calls[4] ==
                  Call{"reset-text", 0x5000 + 12} &&
              runtime.calls[5] ==
                  Call{"text", 0x5000 + 12, 0x9000 + 12} &&
              runtime.calls[12] ==
                  Call{"word", 0x5000 + 60, 160} &&
              runtime.calls[18] ==
                  Call{"word", 0x5000 + 84, 184} &&
              runtime.calls[19] ==
                  Call{"byte", 0x5000 + 88, 88} &&
              runtime.calls[28] ==
                  Call{"byte", 0x5000 + 136, 136} &&
              runtime.calls[29] ==
                  Call{"word", 0x5000 + 140, 0} &&
              runtime.calls[31] ==
                  Call{"word", 0x5000 + 148, 0} &&
              runtime.calls[32] ==
                  Call{"text", 0x5000 + 136, 0x9000 + 136},
          "constructor initializes seven texts after scalar regions");
    runtime.calls.clear();
    Check(pal4::MagicDataRecordCopy::Construct(
              0, 0x9000, runtime) == 0 && runtime.calls.empty(),
          "null destination performs no construction");
}

void CheckFailure() {
    Runtime assigned;
    Seed(assigned, 0x9000, 100);
    assigned.fail_text = 2;
    try {
        static_cast<void>(pal4::MagicDataRecordCopy::AssignRepeated(
            0x5000, 0x5000 + 304, 0x9000, assigned));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  assigned.calls.size() == 5 &&
                  assigned.calls.back() ==
                      Call{"text", 0x5000 + 28, 0x9000 + 28} &&
                  assigned.words[0x5000 + 60] == 0,
              "failure stops before middle scalar region");
    }

    Runtime constructed;
    Seed(constructed, 0x9000, 100);
    constructed.fail_text = 1;
    try {
        static_cast<void>(pal4::MagicDataRecordCopy::Construct(
            0x5000, 0x9000, constructed));
        Check(false, "constructor text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  constructed.calls.size() == 6 &&
                  constructed.calls.back() ==
                      Call{"text", 0x5000 + 12, 0x9000 + 12},
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
        std::cout << "magic_data_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
