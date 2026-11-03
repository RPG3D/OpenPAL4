#include "pal4/role_static_elements.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

struct Runtime final : pal4::IRoleStaticCharacterRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::string>> texts;
    std::vector<std::pair<std::uint32_t, float>> widths;
    std::uint32_t player = 0x8000u;
    std::uint32_t requested_role = 99u;
    std::uint32_t image_window = 0u;
    std::string imageset;
    std::string image;
    std::int32_t hp_max = 200;
    std::int32_t mp_max = 80;
    std::int32_t hp = 50;
    std::int32_t mp = 40;
    std::int32_t rage = 75;

    std::uint32_t GetPlayer(std::uint32_t role_index) override {
        calls.emplace_back("player");
        requested_role = role_index;
        return player;
    }
    std::int32_t GetMaximum(std::uint32_t owner,
                             std::uint32_t stat) override {
        Check(owner == player, "maximum owner");
        calls.emplace_back(stat == 0u ? "max0" : "max2");
        Check(stat == 0u || stat == 2u, "maximum stat");
        return stat == 0u ? hp_max : mp_max;
    }
    std::int32_t GetValue(std::uint32_t owner,
                           std::uint32_t stat) override {
        Check(owner == player, "value owner");
        calls.emplace_back("value" + std::to_string(stat));
        if (stat == 7u) return hp;
        if (stat == 9u) return mp;
        Check(stat == 8u, "value stat");
        return rage;
    }
    void SetText(std::uint32_t window,
                 const std::string& text) override {
        calls.emplace_back("text");
        texts.emplace_back(window, text);
    }
    void SetWidth(std::uint32_t window, float width) override {
        calls.emplace_back("width");
        widths.emplace_back(window, width);
    }
    void SetImage(std::uint32_t window, const char* set,
                  const char* name) override {
        calls.emplace_back("image");
        image_window = window;
        imageset = set;
        image = name;
    }
    std::uint32_t CleanupTextBuffer() override {
        calls.emplace_back("cleanup");
        return 0xabcdu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write-role");
        words[address] = value;
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
};

void Seed(Runtime& runtime) {
    for (std::uint32_t index = 1u; index <= 8u; ++index)
        runtime.words[0x1000u + index * 4u] =
            0x5000u + index * 0x100u;
    runtime.floats[0x1024u] = 100.0f;
    runtime.floats[0x1028u] = 80.0f;
    runtime.floats[0x102cu] = 60.0f;
}

void CheckInvalid(std::int32_t role) {
    Runtime runtime;
    Check(pal4::RoleStaticElements::SetCharacter(
              0x1000u, role, runtime) ==
              static_cast<std::uint32_t>(role) &&
          runtime.calls.empty() && runtime.words.empty(),
          "invalid role returns unchanged without touching state");
}

void CheckMissingPlayer() {
    Runtime runtime;
    Seed(runtime);
    runtime.player = 0u;
    Check(pal4::RoleStaticElements::SetCharacter(
              0x1000u, 2, runtime) == 0u &&
          runtime.words.at(0x1030u) == 2u &&
          runtime.requested_role == 2u &&
          runtime.calls ==
              std::vector<std::string>{"write-role", "player"},
          "role is stored before missing-player exit");
}

void CheckCharacter(std::int32_t role) {
    Runtime runtime;
    Seed(runtime);
    Check(pal4::RoleStaticElements::SetCharacter(
              0x1000u, role, runtime) == 0xabcdu &&
          runtime.words.at(0x1030u) ==
              static_cast<std::uint32_t>(role) &&
          runtime.requested_role ==
              static_cast<std::uint32_t>(role),
          "character index and cleanup return");
    Check(runtime.texts ==
              (std::vector<std::pair<std::uint32_t, std::string>>{
                  {0x5100u, "50/200"}, {0x5200u, "40/80"},
                  {0x5300u, "75/100"}}),
          "HP, MP and rage text");
    Check(runtime.widths.size() == 3u &&
          runtime.widths[0].first == 0x5400u &&
          runtime.widths[1].first == 0x5500u &&
          runtime.widths[2].first == 0x5600u &&
          Near(runtime.widths[0].second, 25.0f) &&
          Near(runtime.widths[1].second, 40.0f) &&
          Near(runtime.widths[2].second, 45.0f),
          "HP, MP and rage widths");
    constexpr const char* images[]{
        "tianhe2", "lingsha2", "mengli2", "ziying2"};
    Check(runtime.image_window == 0x5800u &&
          runtime.imageset == "kuang0" &&
          runtime.image == images[role],
          "role frame image lookup");
    Check(runtime.calls == std::vector<std::string>{
              "write-role", "player", "max0", "value7", "text",
              "max2", "value9", "text", "value8", "text",
              "value7", "max0", "width", "value9", "max2",
              "width", "value8", "width", "image", "cleanup"},
          "player query and UI update order");
}

}  // namespace

int main() {
    try {
        CheckInvalid(-1);
        CheckInvalid(4);
        CheckMissingPlayer();
        for (std::int32_t role = 0; role < 4; ++role)
            CheckCharacter(role);
        std::cout << "role_static_character_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
