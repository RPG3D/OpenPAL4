#include "pal4/game_data_shop_property_record_copy.h"

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
    std::string name;
    std::uint32_t destination;
    std::uint32_t source;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDataShopPropertyRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<Call> calls;
    std::uint32_t text_calls = 0;
    std::uint32_t fail_text = 0;
    bool fail_compare = false;

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
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_text)
            throw std::runtime_error("text failed");
        for (std::uint32_t offset = 0; offset < 16; offset += 4)
            words[destination + offset] = words[source + offset];
    }
    bool RecordsEqual(std::uint32_t left,
                      std::uint32_t right) override {
        calls.push_back({"compare", left, right});
        if (fail_compare) throw std::runtime_error("compare failed");
        return words[left] == words[right];
    }
};

void FillRecord(Runtime& runtime, std::uint32_t address,
                std::uint32_t seed) {
    for (std::uint32_t i = 0; i < 14; ++i)
        runtime.words[address + i * 4] = seed + i;
}

void CheckRecord(Runtime& runtime, std::uint32_t destination,
                 std::uint32_t seed) {
    for (std::uint32_t i = 0; i < 14; ++i)
        Check(runtime.words[destination + i * 4] == seed + i,
              "all 56 record bytes are assigned");
}

void CheckRepeated() {
    Runtime runtime;
    FillRecord(runtime, 0x9000, 100);
    FillRecord(runtime, 0x5000, 1);
    FillRecord(runtime, 0x5000 + 56, 2);
    Check(pal4::GameDataShopPropertyRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 2 * 56, 0x9000, runtime) ==
              0x5000 + 2 * 56 &&
              runtime.calls == std::vector<Call>{
                  {"word", 0x5000, 100},
                  {"text", 0x5000 + 4, 0x9000 + 4},
                  {"text", 0x5000 + 20, 0x9000 + 20},
                  {"text", 0x5000 + 36, 0x9000 + 36},
                  {"word", 0x5000 + 52, 113},
                  {"word", 0x5000 + 56, 100},
                  {"text", 0x5000 + 56 + 4, 0x9000 + 4},
                  {"text", 0x5000 + 56 + 20, 0x9000 + 20},
                  {"text", 0x5000 + 56 + 36, 0x9000 + 36},
                  {"word", 0x5000 + 56 + 52, 113}},
          "repeated assignment follows three text fields and two scalars");
    CheckRecord(runtime, 0x5000, 100);
    CheckRecord(runtime, 0x5000 + 56, 100);
    runtime.calls.clear();
    Check(pal4::GameDataShopPropertyRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(),
          "empty assignment preserves destination end");

    runtime.calls.clear();
    Check(pal4::GameDataShopPropertyRecordCopy::AssignRepeated(
              0x5000, 0x5000 + 56, 0x5000, runtime) == 0x5000 + 56 &&
              runtime.calls[1] == Call{"text", 0x5000 + 4, 0x5000 + 4} &&
              runtime.calls[2] == Call{"text", 0x5000 + 20, 0x5000 + 20} &&
              runtime.calls[3] == Call{"text", 0x5000 + 36, 0x5000 + 36},
          "self-assignment is forwarded to text ownership runtime");
}

void CheckBackward() {
    Runtime runtime;
    FillRecord(runtime, 0x5000, 100);
    FillRecord(runtime, 0x5000 + 56, 200);
    FillRecord(runtime, 0x5000 + 112, 300);
    Check(pal4::GameDataShopPropertyRecordCopy::AssignBackward(
              0x5000, 0x5000 + 2 * 56,
              0x5000 + 3 * 56, runtime) == 0x5000 + 56 &&
              runtime.calls.front() ==
                  Call{"word", 0x5000 + 112, 200} &&
              runtime.calls[5] ==
                  Call{"word", 0x5000 + 56, 100},
          "backward assignment processes final source record first");
    CheckRecord(runtime, 0x5000, 100);
    CheckRecord(runtime, 0x5000 + 56, 100);
    CheckRecord(runtime, 0x5000 + 112, 200);
    runtime.calls.clear();
    Check(pal4::GameDataShopPropertyRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, runtime) == 0x7000 &&
              runtime.calls.empty(),
          "empty backward range preserves destination end");
}

void CheckInterrupted() {
    Runtime runtime;
    FillRecord(runtime, 0x9000, 100);
    FillRecord(runtime, 0x5000, 1);
    runtime.fail_text = 2;
    try {
        static_cast<void>(
            pal4::GameDataShopPropertyRecordCopy::AssignRepeated(
                0x5000, 0x5000 + 56, 0x9000, runtime));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  runtime.calls.size() == 3 &&
                  runtime.words[0x5000] == 100 &&
                  runtime.words[0x5000 + 4] == 101 &&
                  runtime.words[0x5000 + 20] == 6 &&
                  runtime.words[0x5000 + 52] == 14,
              "failure stops before later text and tail scalar assignments");
    }

    Runtime backward;
    FillRecord(backward, 0x5000, 100);
    FillRecord(backward, 0x5000 + 56, 200);
    FillRecord(backward, 0x5000 + 112, 300);
    backward.fail_text = 3;
    try {
        static_cast<void>(
            pal4::GameDataShopPropertyRecordCopy::AssignBackward(
                0x5000, 0x5000 + 2 * 56,
                0x5000 + 3 * 56, backward));
        Check(false, "backward text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  backward.calls.size() == 4 &&
                  backward.words[0x5000 + 112] == 200 &&
                  backward.words[0x5000 + 112 + 20] == 205 &&
                  backward.words[0x5000 + 112 + 36] == 309 &&
                  backward.words[0x5000 + 56] == 200,
              "backward failure stops before tail and prior record");
    }
}

void CheckConstruct() {
    Runtime runtime;
    FillRecord(runtime, 0x9000, 100);
    runtime.bytes[0x9004] = 7;
    runtime.bytes[0x9014] = 8;
    runtime.bytes[0x9024] = 9;
    pal4::GameDataShopPropertyRecordCopy::Construct(0x5000, 0x9000, runtime);
    Check(runtime.words[0x5000] == 100 &&
              runtime.words[0x5000 + 52] == 113 &&
              runtime.bytes[0x5004] == 7 &&
              runtime.bytes[0x5014] == 8 &&
              runtime.bytes[0x5024] == 9 &&
              runtime.calls.front() == Call{"word", 0x5000, 100} &&
              runtime.calls.back() == Call{"word", 0x5000 + 52, 113},
          "copy construction writes scalar ends and initializes text bytes");
    for (std::uint32_t offset : {4u, 20u, 36u}) {
        Check(runtime.calls[(offset - 4) / 16 * 5 + 5] ==
                  Call{"text", 0x5000 + offset, 0x9000 + offset},
              "text assignment follows each initialized text field");
    }
    runtime.calls.clear();
    pal4::GameDataShopPropertyRecordCopy::Construct(0, 0x9000, runtime);
    Check(runtime.calls.empty(), "null destination performs no construction");

    Runtime interrupted;
    FillRecord(interrupted, 0x9000, 100);
    interrupted.fail_text = 2;
    try {
        pal4::GameDataShopPropertyRecordCopy::Construct(
            0x5000, 0x9000, interrupted);
        Check(false, "constructor text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  interrupted.words[0x5000 + 52] == 0 &&
                  interrupted.calls.back() ==
                      Call{"text", 0x5000 + 20, 0x9000 + 20},
              "constructor stops before third text and tail");
    }
}

void CheckAssignAndUnique() {
    Runtime runtime;
    FillRecord(runtime, 0x9000, 100);
    Check(pal4::GameDataShopPropertyRecordCopy::Assign(
              0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.size() == 5 &&
              runtime.calls[1] == Call{"text", 0x5004, 0x9004},
          "single assignment returns destination and preserves field order");
    CheckRecord(runtime, 0x5000, 100);

    Runtime unique;
    for (std::uint32_t i = 0; i < 4; ++i)
        FillRecord(unique, 0x8000 + 56 * i, 100 + i * 100);
    unique.words[0x8000 + 56] = 100;
    unique.words[0x8000 + 3 * 56] = 100;
    Check(pal4::GameDataShopPropertyRecordCopy::CopyUnique(
              0x8000, 0x8000 + 4 * 56, 0x5000, unique) ==
              0x5000 + 3 * 56,
          "unique copy keeps first of each adjacent equal run");
    Check(unique.words[0x5000] == 100 &&
              unique.words[0x5000 + 56] == 300 &&
              unique.words[0x5000 + 112] == 100 &&
              unique.calls[5] ==
                  Call{"compare", 0x8000, 0x8000 + 56} &&
              unique.calls[6] ==
                  Call{"compare", 0x8000, 0x8000 + 112} &&
              unique.calls[12] ==
                  Call{"compare", 0x8000 + 112, 0x8000 + 168},
          "comparison uses last retained source, and copies selected records");
    unique.calls.clear();
    Check(pal4::GameDataShopPropertyRecordCopy::CopyUnique(
              0x8000, 0x8000, 0x5000, unique) == 0x5000 &&
              unique.calls.empty(), "empty unique range performs no work");

    unique.fail_compare = true;
    try {
        static_cast<void>(
            pal4::GameDataShopPropertyRecordCopy::CopyUnique(
                0x8000, 0x8000 + 112, 0x6000, unique));
        Check(false, "comparison failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "compare failed" &&
                  unique.calls.size() == 6 &&
                  unique.words[0x6000] == 100 &&
                  unique.words[0x6000 + 56] == 0,
              "comparison failure follows first copy and prevents later copy");
    }
}

}  // namespace

int main() {
    try {
        CheckRepeated();
        CheckBackward();
        CheckInterrupted();
        CheckConstruct();
        CheckAssignAndUnique();
        std::cout << "game_data_shop_property_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
