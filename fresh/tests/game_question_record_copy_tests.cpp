#include "pal4/game_question_record_copy.h"

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
    std::uint32_t value;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameQuestionRecordCopyRuntime {
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
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back({"word", address, value});
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back({"byte", address, value});
    }
    void SetErrorFlag(std::uint32_t text, std::uint8_t value) override {
        bytes[text] = value;
        calls.push_back({"flag", text, value});
    }
    std::uint32_t AssignText(std::uint32_t destination,
                             std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_text)
            throw std::runtime_error("text failed");
        words[destination + 4] = words[source + 4];
        return destination + 1;
    }
    bool RecordsEqual(std::uint32_t left,
                      std::uint32_t right) override {
        calls.push_back({"compare", left, right});
        if (fail_compare) throw std::runtime_error("compare failed");
        return words[left] == words[right];
    }
};

void Seed(Runtime& runtime, std::uint32_t record, std::uint32_t value) {
    runtime.words[record] = value;
    runtime.words[record + 68] = value + 1;
    runtime.words[record + 72] = value + 2;
    for (const auto offset : {4u, 20u, 36u, 52u})
        runtime.words[record + offset + 4] = value + offset;
}

void CheckRepeated() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    Check(pal4::GameQuestionRecordCopy::AssignRepeated(
              0x5000, 0x5098, 0x9000, runtime) == 0x5098 &&
              runtime.calls == std::vector<Call>{
                  {"word", 0x5000, 100},
                  {"text", 0x5004, 0x9004},
                  {"text", 0x5014, 0x9014},
                  {"text", 0x5024, 0x9024},
                  {"text", 0x5034, 0x9034},
                  {"word", 0x5044, 101},
                  {"word", 0x5048, 102},
                  {"word", 0x504c, 100},
                  {"text", 0x5050, 0x9004},
                  {"text", 0x5060, 0x9014},
                  {"text", 0x5070, 0x9024},
                  {"text", 0x5080, 0x9034},
                  {"word", 0x5090, 101},
                  {"word", 0x5094, 102}} &&
              runtime.words[0x5008] == 104 &&
              runtime.words[0x5094] == 102,
          "repeated assignment copies four texts and two trailing words");
    runtime.calls.clear();
    Check(pal4::GameQuestionRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, runtime) == 0x5000 &&
              runtime.calls.empty(), "empty forward range returns end");
}

void CheckBackwardAndFailure() {
    Runtime backward;
    Seed(backward, 0x5000, 10);
    Seed(backward, 0x504c, 20);
    Check(pal4::GameQuestionRecordCopy::AssignBackward(
              0x5000, 0x5098, 0x50e4, backward) == 0x504c &&
              backward.calls.front() == Call{"word", 0x5098, 20} &&
              backward.calls[7] == Call{"word", 0x504c, 10} &&
              backward.words[0x504c] == 10 &&
              backward.words[0x5098] == 20 &&
              backward.words[0x5098 + 72] == 22,
          "overlapping question move starts at the last record");
    backward.calls.clear();
    Check(pal4::GameQuestionRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, backward) == 0x7000 &&
              backward.calls.empty(),
          "empty backward range leaves destination end intact");

    Runtime failed;
    failed.fail_text = 3;
    Seed(failed, 0x9000, 100);
    try {
        static_cast<void>(pal4::GameQuestionRecordCopy::AssignRepeated(
            0x5000, 0x504c, 0x9000, failed));
        Check(false, "failed text assignment must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  failed.calls.size() == 4 &&
                  failed.calls.back() == Call{"text", 0x5024, 0x9024} &&
                  failed.words[0x5044] == 0,
              "failure stops before fourth text and trailing scalars");
    }
}

void CheckConstruct() {
    Runtime runtime;
    Seed(runtime, 0x9000, 100);
    for (const auto offset : {4u, 20u, 36u, 52u})
        runtime.bytes[0x9000 + offset] =
            static_cast<std::uint8_t>(offset);
    Check(pal4::GameQuestionRecordCopy::Construct(
              0x5000, 0x9000, runtime) == 0x5035 &&
              runtime.calls.size() == 21 &&
              runtime.calls.front() == Call{"word", 0x5000, 100} &&
              runtime.calls[5] == Call{"text", 0x5004, 0x9004} &&
              runtime.calls[10] == Call{"text", 0x5014, 0x9014} &&
              runtime.calls[15] == Call{"text", 0x5024, 0x9024} &&
              runtime.calls[17] == Call{"flag", 0x5034, 0} &&
              runtime.calls[18] == Call{"text", 0x5034, 0x9034} &&
              runtime.calls[19] == Call{"word", 0x5044, 101} &&
              runtime.calls[20] == Call{"word", 0x5048, 102} &&
              runtime.words[0x5008] == 104 &&
              runtime.words[0x5038] == 152,
          "constructor initializes four texts then copies two trailing words");

    Runtime null_destination;
    Check(pal4::GameQuestionRecordCopy::Construct(
              0, 0x9000, null_destination) == 0 &&
              null_destination.calls.empty(),
          "null destination skips question construction");

    Runtime failed;
    failed.fail_text = 4;
    Seed(failed, 0x9000, 100);
    try {
        static_cast<void>(pal4::GameQuestionRecordCopy::Construct(
            0x5000, 0x9000, failed));
        Check(false, "fourth text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  failed.calls.size() == 19 &&
                  failed.calls.back() ==
                      Call{"text", 0x5034, 0x9034} &&
                  failed.words[0x5044] == 0,
              "failed final text stops before trailing fields");
    }
}

void CheckAssignAndUnique() {
    Runtime assigned;
    Seed(assigned, 0x9000, 100);
    Check(pal4::GameQuestionRecordCopy::Assign(
              0x5000, 0x9000, assigned) == 0x5000 &&
              assigned.calls == std::vector<Call>{
                  {"word", 0x5000, 100},
                  {"text", 0x5004, 0x9004},
                  {"text", 0x5014, 0x9014},
                  {"text", 0x5024, 0x9024},
                  {"text", 0x5034, 0x9034},
                  {"word", 0x5044, 101},
                  {"word", 0x5048, 102}},
          "single question assignment writes all seven fields");

    Runtime unique;
    Seed(unique, 0x9000, 3);
    Seed(unique, 0x904c, 3);
    Seed(unique, 0x9098, 1);
    Seed(unique, 0x90e4, 3);
    Check(pal4::GameQuestionRecordCopy::CopyUnique(
              0x9000, 0x9130, 0x5000, unique) == 0x50e4 &&
              unique.words[0x5000] == 3 &&
              unique.words[0x504c] == 1 &&
              unique.words[0x5098] == 3 &&
              unique.calls[7] ==
                  Call{"compare", 0x9000, 0x904c} &&
              unique.calls[8] ==
                  Call{"compare", 0x9000, 0x9098} &&
              unique.calls[16] ==
                  Call{"compare", 0x9098, 0x90e4},
          "question compaction compares against last retained source");
    unique.calls.clear();
    Check(pal4::GameQuestionRecordCopy::CopyUnique(
              0x9000, 0x9000, 0x5000, unique) == 0x5000 &&
              unique.calls.empty(),
          "empty question range leaves destination untouched");

    Runtime failed;
    Seed(failed, 0x9000, 3);
    failed.fail_compare = true;
    try {
        static_cast<void>(pal4::GameQuestionRecordCopy::CopyUnique(
            0x9000, 0x9098, 0x5000, failed));
        Check(false, "question comparison failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "compare failed" &&
                  failed.calls.size() == 8 &&
                  failed.calls.back() ==
                      Call{"compare", 0x9000, 0x904c} &&
                  failed.words[0x5000] == 3,
              "failed comparison follows first question assignment");
    }
}

}  // namespace

int main() {
    try {
        CheckRepeated();
        CheckBackwardAndFailure();
        CheckConstruct();
        CheckAssignAndUnique();
        std::cout << "game_question_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
