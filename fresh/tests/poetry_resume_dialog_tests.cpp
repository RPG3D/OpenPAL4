#include "pal4/poetry_resume_dialog.h"

#include <cstdint>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IPoetryResumeDialogRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, float> floats;
    std::unordered_map<std::uint32_t, float> alphas;
    std::vector<std::string> calls;
    std::vector<std::string> names;
    bool allocate = true;
    std::string prefix = "Poetry/";
    std::uint32_t freed = 0u;
    std::vector<std::uint32_t> faded;
    std::vector<float> scheduled;
    std::uint32_t virtual_first = 0xffffffffu;

    void LoadLayout(std::uint32_t dialog, std::uint32_t layout,
                    std::uint32_t first, std::uint32_t second) override {
        calls.emplace_back("layout");
        Check(dialog == 0x1000u && layout == 0x2345u &&
              first == 0u && second == 0u, "layout arguments");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.emplace_back("write_byte");
        bytes[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.emplace_back("read_byte");
        return bytes.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write_word");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read_word");
        return words.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        calls.emplace_back("write_float");
        floats[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read_float");
        return floats.at(address);
    }
    bool AllocateText(std::uint32_t buffer, std::uint32_t length,
                      std::uint32_t flag) override {
        calls.emplace_back("allocate");
        Check(buffer == 0x1000u + 156u && length == 12u && flag == 1u,
              "buffer allocation arguments");
        return allocate;
    }
    void CopyText(std::uint32_t buffer, std::string_view text) override {
        calls.emplace_back("copy");
        Check(buffer == 0x1000u + 156u && text == "PoetryResume",
              "buffer copy");
    }
    void SetTextSize(std::uint32_t buffer,
                     std::uint32_t length) override {
        calls.emplace_back("set_size");
        Check(buffer == 0x1000u + 156u && length == 12u,
              "text size");
    }
    void InitializeTemporaryString() override {
        calls.emplace_back("init_string");
    }
    void CleanupTemporaryString() override {
        calls.emplace_back("cleanup_string");
    }
    std::string_view ReadPrefix(std::uint32_t address) override {
        Check(address == 0x1000u + 24u, "prefix address");
        return prefix;
    }
    std::uint32_t GetWindow(std::uint32_t dialog,
                            std::string_view name) override {
        Check(dialog == 0x1000u, "window owner");
        names.emplace_back(name);
        return 0x8000u + static_cast<std::uint32_t>(names.size());
    }
    void SetEnabled(std::uint32_t window, bool enabled) override {
        calls.emplace_back("disable");
        Check(window == 0x7000u && !enabled, "disabled window");
    }
    void SetAlpha(std::uint32_t window, float alpha) override {
        calls.emplace_back("alpha");
        alphas[window] = alpha;
        faded.push_back(window);
    }
    float GetAlpha(std::uint32_t window) override {
        calls.emplace_back("get_alpha");
        return alphas.at(window);
    }
    void ScheduleReset(std::uint32_t dialog,
                       std::uint32_t name_address,
                       float delay) override {
        calls.emplace_back("schedule");
        Check(dialog == 0x1000u && name_address == 0x1000u + 156u,
              "reset timer context");
        scheduled.push_back(delay);
    }
    std::uint32_t CallVirtualSlot4(std::uint32_t dialog,
                                   std::uint32_t first,
                                   std::uint32_t second) override {
        calls.emplace_back("virtual_4");
        Check(dialog == 0x1000u && first <= 1u && second == 0u,
              "virtual slot 4 arguments");
        virtual_first = first;
        return 0xabcd0001u;
    }
    void Free(std::uint32_t pointer) override {
        calls.emplace_back("free");
        freed = pointer;
    }
    std::uint32_t DestructBase(std::uint32_t dialog) override {
        calls.emplace_back("destruct_base");
        Check(dialog == 0x1000u, "base destructor owner");
        return 0xdeadu;
    }
};

void CheckInitialize(bool allocate, std::string prefix) {
    Runtime runtime;
    runtime.allocate = allocate;
    runtime.prefix = std::move(prefix);
    runtime.words[0x1000u + 36u] = 0x7000u;
    Check(pal4::PoetryResumeDialog::Initialize(0x1000u, 0x2345u,
                                                runtime) == 0x1000u,
          "initialize return");
    Check(runtime.bytes.at(0x1000u + 156u) == 0x45u &&
          runtime.words.at(0x1000u) == pal4::PoetryResumeDialog::kVtable &&
          runtime.words.at(0x1000u + 152u) == 28u,
          "header fields");
    Check(runtime.names.size() == 28u &&
          runtime.names[0] == runtime.prefix + "No01" &&
          runtime.names[8] == runtime.prefix + "No09" &&
          runtime.names[9] == runtime.prefix + "No10" &&
          runtime.names[27] == runtime.prefix + "No28",
          "all formatted window names");
    for (std::uint32_t index = 0u; index < 28u; ++index) {
        Check(runtime.words.at(0x1000u + 40u + index * 4u) ==
                  0x8001u + index, "window slot sequence");
    }
    Check(runtime.calls.front() == "layout" &&
          runtime.calls.back() == "cleanup_string", "lifecycle order");
    const auto contains = [&](const char* name) {
        for (const auto& call : runtime.calls) if (call == name) return true;
        return false;
    };
    Check(contains("copy") == allocate &&
          contains("set_size") == allocate, "allocation gate");
}

void CheckDestruct(std::uint32_t text, std::uint8_t reference,
                   std::uint32_t expected_free,
                   std::uint8_t expected_reference) {
    Runtime runtime;
    runtime.words[0x1000u + 160u] = text;
    if (text) runtime.bytes[text - 1u] = reference;
    Check(pal4::PoetryResumeDialog::Destruct(0x1000u, runtime) == 0xdeadu,
          "base destructor return");
    Check(runtime.words.at(0x1000u) == pal4::PoetryResumeDialog::kVtable &&
          runtime.words.at(0x1000u + 160u) == 0u &&
          runtime.words.at(0x1000u + 164u) == 0u &&
          runtime.words.at(0x1000u + 168u) == 0u,
          "cleared text buffer");
    Check(runtime.freed == expected_free &&
          (text == 0u || runtime.bytes[text - 1u] == expected_reference) &&
          runtime.calls.back() == "destruct_base", "reference release");
}

void CheckHide(std::int32_t count) {
    Runtime runtime;
    runtime.words[0x1000u + 152u] = static_cast<std::uint32_t>(count);
    runtime.words[0x1000u + 172u] = 4u;
    runtime.words[0x1000u + 176u] = 5u;
    runtime.bytes[0x1000u + 180u] = 1u;
    for (std::int32_t index = 0; index < count; ++index) {
        runtime.words[0x1000u + 40u +
                      static_cast<std::uint32_t>(index) * 4u] =
            0x8000u + static_cast<std::uint32_t>(index);
    }
    Check(pal4::PoetryResumeDialog::Hide(0x1000u, runtime) ==
              0xabcd0001u, "hide return");
    Check(runtime.faded.size() ==
              static_cast<std::size_t>(count > 0 ? count : 0),
          "fade count");
    for (std::size_t index = 0; index < runtime.faded.size(); ++index) {
        Check(runtime.faded[index] == 0x8000u + index,
              "fade order");
        Check(runtime.alphas.at(runtime.faded[index]) == 0.0f,
              "hidden alpha");
    }
    Check(runtime.words.at(0x1000u + 172u) == 0u &&
          runtime.words.at(0x1000u + 176u) == 0u &&
          runtime.bytes.at(0x1000u + 180u) == 0u &&
          runtime.calls.back() == "virtual_4", "hide state reset");
    Check(runtime.virtual_first == 1u, "hide virtual value");
}

void SeedUpdate(Runtime& runtime, std::int32_t current,
                float progress, std::int32_t count = 28) {
    runtime.words[0x1000u + 172u] = static_cast<std::uint32_t>(current);
    runtime.floats[0x1000u + 176u] = progress;
    runtime.bytes[0x1000u + 180u] = 0u;
    for (std::int32_t index = 0; index <= count; ++index) {
        runtime.words[0x1000u + 40u +
                      static_cast<std::uint32_t>(index) * 4u] =
            0x8000u + static_cast<std::uint32_t>(index);
        runtime.alphas[0x8000u + static_cast<std::uint32_t>(index)] =
            index < current ? 0.8f : 0.0f;
    }
    runtime.words[0x1000u + 152u] = static_cast<std::uint32_t>(count);
}

void CheckUpdateGateAndReset() {
    Runtime runtime;
    SeedUpdate(runtime, 1, 0.3f);
    runtime.bytes[0x1000u + 180u] = 1u;
    pal4::PoetryResumeDialog::Update(0x1000u, 0.4, runtime);
    Check(runtime.floats.at(0x1000u + 176u) == 0.3f &&
          runtime.faded.empty(), "paused update");
    pal4::PoetryResumeDialog::ResetState(0x1000u, runtime);
    Check(runtime.bytes.at(0x1000u + 180u) == 0u, "reset pause byte");
    pal4::PoetryResumeDialog::Update(0x1000u, 0.2, runtime);
    Check(std::abs(runtime.alphas.at(0x8000u) - 1.0f) < 0.0001f &&
          std::abs(runtime.alphas.at(0x8001u) - 0.5f) < 0.0001f,
          "previous and current alpha");
}

void CheckUpdateAlphaBranches() {
    Runtime ordinary;
    SeedUpdate(ordinary, 5, 0.5f);
    pal4::PoetryResumeDialog::UpdateAlpha(0x1000u, 0.2f, ordinary);
    Check(ordinary.words.at(0x1000u + 172u) == 6u &&
          ordinary.floats.at(0x1000u + 176u) == 0.0f &&
          ordinary.scheduled.empty(), "ordinary item advance");

    Runtime boundary;
    SeedUpdate(boundary, 6, 0.7f);
    pal4::PoetryResumeDialog::UpdateAlpha(0x1000u, 0.1f, boundary);
    Check(boundary.words.at(0x1000u + 172u) == 6u &&
          boundary.scheduled.empty(), "seventh waits for full alpha");
    pal4::PoetryResumeDialog::UpdateAlpha(0x1000u, 0.3f, boundary);
    Check(boundary.words.at(0x1000u + 172u) == 7u &&
          boundary.floats.at(0x1000u + 176u) == 0.0f &&
          boundary.alphas.at(0x8006u) == 1.0f &&
          boundary.bytes.at(0x1000u + 180u) == 1u &&
          boundary.scheduled == std::vector<float>{1.5f},
          "seventh schedules reset");

    Runtime final_item;
    SeedUpdate(final_item, 28, 0.9f);
    pal4::PoetryResumeDialog::UpdateAlpha(0x1000u, 0.2f, final_item);
    Check(final_item.words.at(0x1000u + 172u) == 29u &&
          final_item.bytes.at(0x1000u + 180u) == 1u &&
          final_item.scheduled == std::vector<float>{2.0f},
          "final item schedules two-second reset");
}

void CheckFadeOut() {
    Runtime runtime;
    SeedUpdate(runtime, 3, 0.4f, 3);
    Check(pal4::PoetryResumeDialog::FadeOut(0x1000u, 0.5f,
                                             runtime) == 3u,
          "fadeout count return");
    Check(runtime.faded == std::vector<std::uint32_t>{
              0x8000u, 0x8001u, 0x8002u}, "fadeout window order");
    for (const auto window : runtime.faded) {
        Check(std::abs(runtime.alphas.at(window) - 0.45f) < 0.0001f,
              "fadeout alpha");
    }
    runtime.faded.clear();
    pal4::PoetryResumeDialog::Update(0x1000u, 2.0, runtime);
    Check(runtime.virtual_first == 0u && runtime.faded.empty(),
          "completed fadeout virtual call");
}

}  // namespace

int main() {
    try {
        CheckInitialize(true, "Poetry/");
        CheckInitialize(false, "");
        CheckDestruct(0u, 0u, 0u, 0u);
        CheckDestruct(0x9000u, 0u, 0x8fffu, 0u);
        CheckDestruct(0x9000u, 0xffu, 0x8fffu, 0xffu);
        CheckDestruct(0x9000u, 3u, 0u, 2u);
        CheckHide(-1);
        CheckHide(0);
        CheckHide(3);
        CheckUpdateGateAndReset();
        CheckUpdateAlphaBranches();
        CheckFadeOut();
        std::cout << "poetry_resume_dialog_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
