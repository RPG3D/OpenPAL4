#include "pal4/skill_data_record_copy.h"
#include "pal4/stunt_data_record_copy.h"
#include "pal4/magic_data_record_copy.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataRecordCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> text_calls;
    std::vector<std::uint32_t> word_writes;
    unsigned fail_text = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        word_writes.push_back(address);
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void CopyBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {
        throw std::runtime_error("unexpected raw copy");
    }
    void ResetText(std::uint32_t) override {
        throw std::runtime_error("unexpected text reset");
    }
    std::uint32_t AssignText(std::uint32_t destination,
                             std::uint32_t source) override {
        text_calls.emplace_back(destination, source);
        if (text_calls.size() == fail_text)
            throw std::runtime_error("text failed");
        for (std::uint32_t offset = 0; offset < 16; offset += 4)
            words[destination + offset] = words[source + offset];
        return destination;
    }
};

using AssignFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t,
    pal4::IGameDataRecordCopyRuntime&);
using UniqueFunction = std::uint32_t (*)(
    std::uint32_t, std::uint32_t, std::uint32_t,
    const std::function<bool(std::uint32_t, std::uint32_t)>&,
    pal4::IGameDataRecordCopyRuntime&);

struct RecordCase {
    std::uint32_t bytes;
    std::uint32_t scalar_words;
    std::uint32_t words_before_first_text;
    std::uint32_t text_fields;
    std::uint32_t final_text_offset;
    AssignFunction assign;
    UniqueFunction copy_unique;
};

void Seed(Runtime& runtime, std::uint32_t record,
          std::uint32_t bytes, std::uint32_t key,
          std::uint32_t marker) {
    for (std::uint32_t offset = 0; offset < bytes; offset += 4)
        runtime.words[record + offset] = marker + offset;
    runtime.words[record] = key;
    runtime.words[record + 4] = marker;
}

void CheckAssign(const RecordCase& kind) {
    Runtime runtime;
    Seed(runtime, 0x9000, kind.bytes, 7, 100);
    Check(kind.assign(0x8000, 0x9000, runtime) == 0x8000 &&
              runtime.words[0x8000] == 7 &&
              runtime.words[0x8004] == 100 &&
              runtime.word_writes.size() == kind.scalar_words &&
              runtime.text_calls.size() == kind.text_fields &&
              runtime.text_calls.back() ==
                  std::pair{0x8000 + kind.final_text_offset,
                            0x9000 + kind.final_text_offset},
          "single assignment copies exact scalar and text field count");

    runtime.text_calls.clear();
    runtime.word_writes.clear();
    Check(kind.assign(0x9000, 0x9000, runtime) == 0x9000 &&
              runtime.text_calls.size() == kind.text_fields,
          "self-assignment reaches every text field");

    Runtime failed;
    Seed(failed, 0x9000, kind.bytes, 7, 100);
    failed.fail_text = 1;
    try {
        static_cast<void>(kind.assign(0x8000, 0x9000, failed));
        Check(false, "text failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  failed.text_calls.size() == 1 &&
                  failed.word_writes.size() == kind.words_before_first_text,
              "single assignment stops at failed text field");
    }
}

void CheckUnique(const RecordCase& kind) {
    Runtime runtime;
    const auto stride = kind.bytes;
    const std::uint32_t begin = 0x5000;
    const std::uint32_t destination = 0x8000;
    Seed(runtime, begin, stride, 2, 100);
    Seed(runtime, begin + stride, stride, 2, 200);
    Seed(runtime, begin + 2 * stride, stride, 1, 300);
    Seed(runtime, begin + 3 * stride, stride, 2, 400);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> comparisons;
    const auto equal = [&](std::uint32_t left, std::uint32_t right) {
        comparisons.emplace_back(left, right);
        return runtime.words[left] == runtime.words[right];
    };
    Check(kind.copy_unique(begin, begin + 4 * stride,
                           destination, equal, runtime) ==
              destination + 3 * stride &&
              runtime.words[destination] == 2 &&
              runtime.words[destination + 4] == 100 &&
              runtime.words[destination + stride] == 1 &&
              runtime.words[destination + stride + 4] == 300 &&
              runtime.words[destination + 2 * stride] == 2 &&
              runtime.words[destination + 2 * stride + 4] == 400 &&
              comparisons ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {begin, begin + stride},
                      {begin, begin + 2 * stride},
                      {begin + 2 * stride, begin + 3 * stride}} &&
              runtime.text_calls.size() == 3 * kind.text_fields,
          "copy unique collapses adjacent runs and keeps later recurrence");

    Runtime empty;
    unsigned comparisons_for_empty = 0;
    const auto never_equal = [&](std::uint32_t, std::uint32_t) {
        ++comparisons_for_empty;
        return false;
    };
    Check(kind.copy_unique(begin, begin, destination,
                           never_equal, empty) == destination &&
              comparisons_for_empty == 0 &&
              empty.text_calls.empty(),
          "empty range leaves destination untouched");

    Runtime failed;
    Seed(failed, begin, stride, 2, 100);
    failed.fail_text = 1;
    try {
        static_cast<void>(kind.copy_unique(
            begin, begin + stride, destination,
            never_equal, failed));
        Check(false, "first copied record failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "text failed" &&
                  failed.text_calls.size() == 1 &&
                  comparisons_for_empty == 0,
              "copy failure precedes key comparison");
    }
}

}  // namespace

int main() {
    try {
        for (const RecordCase kind : {
                 RecordCase{132, 17, 17, 4, 116,
                            pal4::SkillDataRecordCopy::Assign,
                            pal4::SkillDataRecordCopy::CopyUnique},
                 RecordCase{208, 24, 2, 7, 192,
                            pal4::StuntDataRecordCopy::Assign,
                            pal4::StuntDataRecordCopy::CopyUnique},
                 RecordCase{152, 10, 3, 7, 136,
                            pal4::MagicDataRecordCopy::Assign,
                            pal4::MagicDataRecordCopy::CopyUnique}}) {
            CheckAssign(kind);
            CheckUnique(kind);
        }
        std::cout << "game_data_record_copy_unique_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
