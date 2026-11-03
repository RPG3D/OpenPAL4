#include "pal4/combat_result_icon_creator.h"

#include <bit>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t kElement = 0x1000u;
constexpr std::uint32_t kDialog = 0x2000u;

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatResultIconRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::string image;

    Runtime() { words[kElement + 60u] = kDialog; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    void PlaySound(std::string_view name,
                   std::uint32_t mode) override {
        Check(name == "WA44.wav" && mode == 1u,
              "result icon sound");
        calls.emplace_back("sound");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x48u, "icon font element size");
        calls.emplace_back("allocate");
        return 0xa000u;
    }
    std::uint32_t ConstructFontElement(
        std::uint32_t memory) override {
        Check(memory == 0xa000u, "icon wrapper address");
        calls.emplace_back("construct");
        return memory;
    }
    void SetDimensions(std::uint32_t element,
                       float width, float height) override {
        Check(element == 0xa000u && width == 450.0f &&
                  height == 430.0f, "icon dimensions");
        calls.emplace_back("dimensions");
    }
    void SetImageset(std::uint32_t element,
                     std::string_view imageset,
                     std::string_view image_name,
                     float scale) override {
        Check(element == 0xa000u && imageset == "zi0" &&
                  scale == 20.0f, "icon imageset and scale");
        image = image_name;
        calls.emplace_back("imageset");
    }
    void SetFont(std::uint32_t element,
                 std::string_view text,
                 std::string_view font, float scale) override {
        Check(element == 0xa000u && text.empty() &&
                  font == "system" && scale == 1.0f,
              "icon font");
        calls.emplace_back("font");
    }
    void SetFontProperties(std::uint32_t element,
                           float first, float second) override {
        Check(element == 0xa000u && first == 2.0f && second == 3.0f,
              "icon font properties");
        calls.emplace_back("font-properties");
    }
    std::uint32_t ElementList() override {
        calls.emplace_back("list");
        return 0xb000u;
    }
    void AddElement(std::uint32_t list, bool constructed,
                    std::uint32_t element) override {
        Check(list == 0xb000u && constructed && element == 0xa000u,
              "icon list insertion");
        calls.emplace_back("add");
    }
};

void FourScoresAndOutOfRange() {
    constexpr const char* names[]{
        "tedeng2", "yideng2", "erdeng2", "sandeng2"};
    for (std::uint32_t score = 0u; score < 4u; ++score) {
        Runtime runtime;
        runtime.words[kDialog + 428u] = score;
        Check(pal4::CombatResultIconCreator::Create(
                  kElement, runtime) == 1u &&
                  runtime.image == names[score] &&
                  runtime.words[0xa000u] ==
                      pal4::CombatResultIconCreator::kVtable &&
                  runtime.words[0xa000u + 68u] ==
                      std::bit_cast<std::uint32_t>(60.0f) &&
                  runtime.calls == std::vector<std::string>{
                      "sound", "allocate", "construct", "write:40960",
                      "dimensions", "imageset", "write:41028", "font",
                      "font-properties", "list", "add"},
              "score image and ordered initialization");
    }
    for (const auto score : {4u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[kDialog + 428u] = score;
        Check(pal4::CombatResultIconCreator::Create(
                  kElement, runtime) == 1u &&
                  runtime.calls == std::vector<std::string>{"sound"},
              "invalid score plays sound without creating icon");
    }
}

}  // namespace

int main() {
    try {
        FourScoresAndOutOfRange();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
