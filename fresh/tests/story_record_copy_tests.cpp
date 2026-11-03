#include "pal4/story_record_copy.h"
#include "pal4/story_desc_record_copy.h"

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

struct Runtime final : pal4::IStoryRecordCopyRuntime {
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
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) override {
        calls.push_back({"text", destination, source});
        if (++text_calls == fail_text)
            throw std::runtime_error("text failed");
        for (std::uint32_t offset : {4u, 8u, 12u})
            words[destination + offset] = words[source + offset];
    }
    bool RecordsEqual(std::uint32_t left,
                      std::uint32_t right) override {
        calls.push_back({"compare", left, right});
        if (fail_compare) throw std::runtime_error("compare failed");
        return words[left] == words[right];
    }
};

void CheckConstruct() {
    Runtime runtime;
    runtime.words[0x9000] = 55;
    runtime.bytes[0x9004] = 3;
    runtime.bytes[0x9014] = 7;
    runtime.words[0x9008] = 0xa000;
    runtime.words[0x9018] = 0xb000;
    pal4::StoryRecordCopy::Construct(0x5000, 0x9000, runtime);
    Check(runtime.calls == std::vector<Call>{
              {"word", 0x5000, 55},
              {"byte", 0x5004, 3},
              {"word", 0x5008, 0},
              {"word", 0x500c, 0},
              {"word", 0x5010, 0},
              {"text", 0x5004, 0x9004},
              {"byte", 0x5014, 7},
              {"word", 0x5018, 0},
              {"word", 0x501c, 0},
              {"word", 0x5020, 0},
              {"text", 0x5014, 0x9014}} &&
              runtime.words[0x5000] == 55 &&
              runtime.words[0x5008] == 0xa000 &&
              runtime.words[0x5018] == 0xb000,
          "constructor initializes both text slots before assigning each");

    Runtime null_destination;
    pal4::StoryRecordCopy::Construct(0, 0x9000, null_destination);
    Check(null_destination.calls.empty(),
          "null destination skips construction");
}

void CheckFailure() {
    for (std::uint32_t fail_at : {1u, 2u}) {
        Runtime runtime;
        runtime.fail_text = fail_at;
        try {
            pal4::StoryRecordCopy::Construct(0x5000, 0x9000, runtime);
            Check(false, "text failure must propagate");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == "text failed" &&
                      runtime.calls.back().name == "text" &&
                      runtime.text_calls == fail_at &&
                      runtime.calls.size() == (fail_at == 1 ? 6u : 11u),
                  "constructor stops at the failed text assignment");
        }
    }
}

void CheckRepeatedAndBackward() {
    Runtime repeated;
    repeated.words[0x9000] = 55;
    repeated.words[0x9008] = 0xa000;
    repeated.words[0x9018] = 0xb000;
    Check(pal4::StoryRecordCopy::AssignRepeated(
              0x5000, 0x5048, 0x9000, repeated) == 0x5048 &&
              repeated.calls == std::vector<Call>{
                  {"word", 0x5000, 55},
                  {"text", 0x5004, 0x9004},
                  {"text", 0x5014, 0x9014},
                  {"word", 0x5024, 55},
                  {"text", 0x5028, 0x9004},
                  {"text", 0x5038, 0x9014}} &&
              repeated.words[0x5008] == 0xa000 &&
              repeated.words[0x503c] == 0xb000,
          "repeated assignment writes ID and two text slots per row");
    repeated.calls.clear();
    Check(pal4::StoryRecordCopy::AssignRepeated(
              0x5000, 0x5000, 0x9000, repeated) == 0x5000 &&
              repeated.calls.empty(),
          "empty forward range returns its end");

    Runtime backward;
    backward.words[0x5000] = 10;
    backward.words[0x5024] = 20;
    Check(pal4::StoryRecordCopy::AssignBackward(
              0x5000, 0x5048, 0x506c, backward) == 0x5024 &&
              backward.calls == std::vector<Call>{
                  {"word", 0x5048, 20},
                  {"text", 0x504c, 0x5028},
                  {"text", 0x505c, 0x5038},
                  {"word", 0x5024, 10},
                  {"text", 0x5028, 0x5004},
                  {"text", 0x5038, 0x5014}} &&
              backward.words[0x5024] == 10 &&
              backward.words[0x5048] == 20,
          "overlapping move assigns from end toward front");
    backward.calls.clear();
    Check(pal4::StoryRecordCopy::AssignBackward(
              0x5000, 0x5000, 0x7000, backward) == 0x7000 &&
              backward.calls.empty(),
          "empty backward range preserves destination end");
}

void CheckAssignmentFailure() {
    Runtime failed;
    failed.fail_text = 2;
    failed.words[0x9000] = 55;
    try {
        static_cast<void>(pal4::StoryRecordCopy::AssignRepeated(
            0x5000, 0x5048, 0x9000, failed));
        Check(false, "assignment failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  failed.calls.size() == 3 &&
                  failed.calls.back() == Call{"text", 0x5014, 0x9014} &&
                  failed.words[0x5024] == 0,
              "failed text assignment stops before next record");
    }
}

void CheckStoryDescConstruct() {
    Runtime runtime;
    runtime.words[0x9000] = 77;
    runtime.bytes[0x9004] = 2;
    runtime.bytes[0x9014] = 3;
    pal4::StoryDescRecordCopy::Construct(0x6000, 0x9000, runtime);
    Check(runtime.calls.size() == 11 &&
              runtime.calls.front() == Call{"word", 0x6000, 77} &&
              runtime.calls[5] == Call{"text", 0x6004, 0x9004} &&
              runtime.calls.back() == Call{"text", 0x6014, 0x9014},
          "StoryDesc construction preserves both text initialization stages");
    Runtime empty;
    pal4::StoryDescRecordCopy::Construct(0, 0x9000, empty);
    Check(empty.calls.empty(), "StoryDesc null destination is skipped");
}

void CheckAssignAndUnique() {
    Runtime assigned;
    assigned.words[0x9000] = 55;
    Check(pal4::StoryRecordCopy::Assign(
              0x5000, 0x9000, assigned) == 0x5000 &&
              assigned.calls == std::vector<Call>{
                  {"word", 0x5000, 55},
                  {"text", 0x5004, 0x9004},
                  {"text", 0x5014, 0x9014}},
          "single Story assignment writes ID and two text slots");

    Runtime unique;
    unique.words[0x9000] = 3;
    unique.words[0x9024] = 3;
    unique.words[0x9048] = 1;
    unique.words[0x906c] = 3;
    Check(pal4::StoryRecordCopy::CopyUnique(
              0x9000, 0x9090, 0x5000, unique) == 0x506c &&
              unique.words[0x5000] == 3 &&
              unique.words[0x5024] == 1 &&
              unique.words[0x5048] == 3 &&
              unique.calls[3] ==
                  Call{"compare", 0x9000, 0x9024} &&
              unique.calls[4] ==
                  Call{"compare", 0x9000, 0x9048} &&
              unique.calls[8] ==
                  Call{"compare", 0x9048, 0x906c},
          "compaction retains the first of each adjacent equal run");
    unique.calls.clear();
    Check(pal4::StoryRecordCopy::CopyUnique(
              0x9000, 0x9000, 0x5000, unique) == 0x5000 &&
              unique.calls.empty(),
          "empty unique range does not read or assign records");

    Runtime failed;
    failed.words[0x9000] = 3;
    failed.fail_compare = true;
    try {
        static_cast<void>(pal4::StoryRecordCopy::CopyUnique(
            0x9000, 0x9048, 0x5000, failed));
        Check(false, "comparison failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "compare failed" &&
                  failed.calls.size() == 4 &&
                  failed.calls.back() ==
                      Call{"compare", 0x9000, 0x9024} &&
                  failed.words[0x5000] == 3,
              "failed comparison follows first record assignment");
    }
}

}  // namespace

int main() {
    try {
        CheckConstruct();
        CheckFailure();
        CheckRepeatedAndBackward();
        CheckAssignmentFailure();
        CheckStoryDescConstruct();
        CheckAssignAndUnique();
        std::cout << "story_record_copy_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
