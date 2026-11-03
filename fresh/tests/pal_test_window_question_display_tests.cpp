#include "pal4/pal_test_window_question_display.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::PalTestWindowQuestionDisplayRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::string> strings;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> word_reads;
    std::vector<std::uint32_t> byte_reads;
    std::vector<std::string> converted;
    std::vector<std::pair<std::uint32_t, std::u32string>> text_writes;
    std::vector<std::pair<std::uint32_t, bool>> selections;
    std::uint32_t process_count{};
    bool fail_conversion{};

    std::uint32_t ReadWord(std::uint32_t address) override {
        word_reads.push_back(address);
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        byte_reads.push_back(address);
        const auto item = bytes.find(address);
        return item == bytes.end() ? 0u : item->second;
    }
    std::string ReadCString(std::uint32_t address) override {
        return strings.at(address);
    }
    std::string ConvertToUtf8(std::string_view text) override {
        converted.emplace_back(text);
        if (fail_conversion) throw std::runtime_error("conversion");
        if (text == "Two") return "\xE4\xBA\x8C";
        return std::string(text);
    }
    void SetText(std::uint32_t window,
                 std::u32string_view text) override {
        text_writes.emplace_back(window, text);
    }
    void SetSelected(std::uint32_t window, bool selected) override {
        selections.emplace_back(window, selected);
    }
    void ProcessAnswer(pal4::PalTestWindowState&) override {
        ++process_count;
    }
};

pal4::PalTestWindowState MakeState() {
    pal4::PalTestWindowState state{};
    state.Write(0x40u, 0x2100u);
    state.Write(0x58u, 0x2200u);
    state.Write(0x5cu, 0x2300u);
    state.Write(0x60u, 0x2400u);
    state.Write(0x6cu, 0x9000u);
    state.Write(0x70u, 0x9008u);
    state.Write(0x78u, 1u);
    return state;
}

void CheckActiveQuestion() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words = {{0x9004u, 0xa000u}, {0xa008u, 0xb000u},
                     {0xa018u, 0xb100u}, {0xa028u, 0xb200u},
                     {0xa038u, 0u}};
    runtime.strings = {{0xb000u, "Question"}, {0xb100u, "One"},
                       {0xb200u, "Two"}};
    pal4::PalTestWindowQuestionDisplay::Update(state, runtime);
    Check(runtime.word_reads == std::vector<std::uint32_t>{
              0x9004u, 0xa008u, 0xa018u, 0xa028u, 0xa038u},
          "record and text pointer offsets match IDA");
    Check(runtime.converted == std::vector<std::string>{
              "Question", "One", "Two", ""} &&
                  runtime.text_writes ==
                      std::vector<std::pair<std::uint32_t, std::u32string>>{
                          {0x2100u, U"Question"}, {0x2200u, U"One"},
                          {0x2300u, U"\u4e8c"}, {0x2400u, U""}} &&
                  runtime.selections ==
                      std::vector<std::pair<std::uint32_t, bool>>{
                          {0x2200u, false}, {0x2300u, false},
                          {0x2400u, false}, {0x2200u, true}} &&
                  runtime.process_count == 0u,
          "question text, UTF-8 answer and first selection are displayed");
    runtime.selections.clear();
    pal4::PalTestWindowQuestionDisplay::SetSelectedAnswer(state, 2u, runtime);
    Check(runtime.selections ==
              std::vector<std::pair<std::uint32_t, bool>>{
                  {0x2200u, false}, {0x2300u, false},
                  {0x2400u, false}, {0x2400u, true}},
          "selected answer resets all three then selects requested one");
}

void CheckExhaustedQuestion() {
    auto state = MakeState();
    state.Write(0x78u, 2u);
    Runtime runtime;
    pal4::PalTestWindowQuestionDisplay::Update(state, runtime);
    Check(runtime.word_reads.empty() && runtime.converted.empty() &&
                  runtime.text_writes ==
                      std::vector<std::pair<std::uint32_t, std::u32string>>{
                          {0x2100u, U""}, {0x2200u, U""},
                          {0x2300u, U""}, {0x2400u, U""}} &&
                  runtime.process_count == 1u,
          "exhausted collection clears four controls before result");
    state.Write(0x6cu, 0u);
    state.Write(0x78u, 0u);
    runtime = Runtime{};
    pal4::PalTestWindowQuestionDisplay::Update(state, runtime);
    Check(runtime.process_count == 1u,
          "null collection is immediately exhausted");
}

void CheckSelectedIndex() {
    const auto state = MakeState();
    Runtime runtime;
    runtime.bytes[0x2200u + 0x4b2u] = 0u;
    runtime.bytes[0x2300u + 0x4b2u] = 0u;
    runtime.bytes[0x2400u + 0x4b2u] = 1u;
    Check(pal4::PalTestWindowQuestionDisplay::SelectedAnswerIndex(
              state, runtime) == 2 &&
                  runtime.byte_reads == std::vector<std::uint32_t>{
                      0x26b2u, 0x27b2u, 0x28b2u},
          "selection scans answer flag at +0x4b2");
    runtime.byte_reads.clear();
    runtime.bytes[0x2200u + 0x4b2u] = 1u;
    Check(pal4::PalTestWindowQuestionDisplay::SelectedAnswerIndex(
              state, runtime) == 0 && runtime.byte_reads.size() == 1u,
          "first selected answer stops scan");
    runtime.byte_reads.clear();
    runtime.bytes[0x2200u + 0x4b2u] = 0u;
    runtime.bytes[0x2400u + 0x4b2u] = 0u;
    Check(pal4::PalTestWindowQuestionDisplay::SelectedAnswerIndex(
              state, runtime) == -1 && runtime.byte_reads.size() == 3u,
          "no selection returns minus one after three reads");
}

void CheckConversionFailure() {
    auto state = MakeState();
    Runtime runtime;
    runtime.words = {{0x9004u, 0xa000u}, {0xa008u, 0xb000u}};
    runtime.strings = {{0xb000u, "Question"}};
    runtime.fail_conversion = true;
    try {
        pal4::PalTestWindowQuestionDisplay::Update(state, runtime);
        Check(false, "conversion failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.text_writes.empty() && runtime.selections.empty() &&
                      runtime.process_count == 0u,
              "conversion failure stops before text and selection writes");
    }
}

}  // namespace

int main() {
    CheckActiveQuestion();
    CheckExhaustedQuestion();
    CheckSelectedIndex();
    CheckConversionFailure();
    std::cout << "Pal test window question display tests passed\n";
}
