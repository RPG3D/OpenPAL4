#include "pal4/system_setting_options.h"
#include "pal4/system_setting_frame_mode.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ISystemSettingOptionActionsRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, bool>> selections;
    std::vector<std::string> calls;
    std::uint32_t applied_clip = 99u;

    Runtime(std::uint32_t offset, std::uint32_t count,
            std::uint32_t source_index, std::uint32_t option_index) {
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto button = 0x5000u + index;
            words[0x4000u + offset + index * 4u] = button;
            words[button + 280u] = option_index + index;
        }
        words[0x6000u + 8u] = 0x5000u + source_index;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    void SetSelected(std::uint32_t button, bool selected) override {
        calls.emplace_back("select");
        selections.emplace_back(button, selected);
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global");
        return 0x7000u;
    }
    void ApplyClipDistance(std::uint32_t data, std::uint32_t index) override {
        calls.emplace_back("clip");
        Check(data == 0x7000u, "clip global data");
        applied_clip = index;
    }
};

void CheckGroup(const Runtime& runtime, std::uint32_t count,
                std::uint32_t source_index) {
    Check(runtime.selections.size() == count + 1u,
          "clear group then select event source");
    for (std::uint32_t index = 0; index < count; ++index) {
        Check(runtime.selections[index] ==
              std::pair<std::uint32_t, bool>{0x5000u + index, false},
              "group reset order");
    }
    Check(runtime.selections.back() ==
          std::pair<std::uint32_t, bool>{0x5000u + source_index, true} &&
          runtime.calls.back() ==
              (runtime.applied_clip == 99u ? "write" : "clip"),
          "source selected before global update");
}

}  // namespace

int main() {
    Runtime sequence(200u, 3u, 2u, 0u);
    Check(pal4::SystemSettingOptions::SelectSequence(
          0x4000u, 0x6000u, sequence) == 1u &&
          sequence.words.at(0x7004u) == 2u,
          "sequence index stored at global +4");
    CheckGroup(sequence, 3u, 2u);

    Runtime clip(212u, 2u, 1u, 0u);
    Check(pal4::SystemSettingOptions::SelectClipDistance(
          0x4000u, 0x6000u, clip) == 1u &&
          clip.applied_clip == 1u &&
          !clip.words.contains(0x700cu),
          "clip delegated to render/effect distance updater");
    CheckGroup(clip, 2u, 1u);

    Runtime auto_mode(220u, 3u, 0u, 1u);
    Check(pal4::SystemSettingOptions::SelectAuto(
          0x4000u, 0x6000u, auto_mode) == 1u &&
          auto_mode.words.at(0x7000u) == 1u,
          "auto_mode stores event button's remapped index");
    CheckGroup(auto_mode, 3u, 0u);

    Runtime intro(232u, 2u, 0u, 0u);
    Check(pal4::SystemSettingOptions::SelectIntro(
          0x4000u, 0x6000u, intro) == 1u &&
          intro.words.at(0x7008u) == 1u,
          "first intro mode maps to true");
    CheckGroup(intro, 2u, 0u);
    Runtime intro_second(232u, 2u, 1u, 0u);
    (void)pal4::SystemSettingOptions::SelectIntro(
        0x4000u, 0x6000u, intro_second);
    Check(intro_second.words.at(0x7008u) == 0u,
          "second intro mode maps to false");
}
