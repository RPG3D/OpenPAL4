#include "pal4/combat_system_render.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;
constexpr std::uint32_t kFrame = 0x5000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemRenderRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    Runtime() {
        words[kObject + 424u] = 0x2000u;
        words[kObject + 128u] = 0x4000u;
        words[0x2000u + 24u] = 4u;
        words[0x3000u + 1348u] = 7u;
        words[kFrame] = 0x1234u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void PlaySound(std::uint32_t sound, std::uint32_t second,
                   std::uint32_t third) override {
        Check(sound == 0x8000u && second == 2u && third == 3u,
              "sound call forwards all arguments");
        calls.emplace_back("sound");
    }
    void RenderWorld(std::uint32_t frame) override {
        Check(frame == kFrame, "world render frame");
        calls.emplace_back("world");
    }
    void RenderGameObject(std::uint32_t object,
                          std::uint32_t frame) override {
        Check(object == 0x2000u && frame == kFrame,
              "game object pointer and frame");
        calls.emplace_back("game-object");
    }
    void RenderActors(std::uint32_t frame_value) override {
        Check(frame_value == 0x1234u, "actor render frame word");
        calls.emplace_back("actors");
    }
    void RenderUi(std::uint32_t frame) override {
        Check(frame == kFrame, "UI render frame");
        calls.emplace_back("ui");
    }
    void RenderConsole(std::uint32_t frame_value) override {
        Check(frame_value == 0x1234u, "console render frame word");
        calls.emplace_back("console");
    }
    std::uint32_t GetGameState() override {
        calls.emplace_back("game-state");
        return 0x3000u;
    }
    std::uint32_t CountEffects(std::uint32_t game_object) override {
        Check(game_object == 0x2000u, "effect count object");
        calls.emplace_back("effects");
        return 11u;
    }
    std::string GetBgmName() override {
        calls.emplace_back("bgm");
        return "field.mp3";
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x4000u, "world name pointer +128");
        calls.emplace_back("world-name");
        return "River";
    }
    void DrawText(std::string_view text,
                  std::uint32_t left, std::uint32_t top,
                  std::uint32_t right, std::uint32_t bottom,
                  std::uint32_t flags, std::uint32_t color,
                  std::uint32_t font_size) override {
        Check(left == 0u && right == 400u && flags == 1u &&
                  color == 0xff00ff00u && font_size == 16u,
              "debug text fixed rectangle style");
        calls.emplace_back("text:" + std::to_string(top) + ":" +
                           std::to_string(bottom) + ":" +
                           std::string(text));
    }
};

void SoundAndRenderStates() {
    Runtime runtime;
    Check(pal4::CombatSystemRender::PlaySoundEffect(
              0x8000u, 2u, 3u, runtime) == 0x8000u &&
              runtime.calls == std::vector<std::string>{"sound"},
          "sound wrapper returns first argument");
    runtime.calls.clear();
    runtime.words[kObject + 508u] = 0u;
    pal4::CombatSystemRender::Render(kObject, kFrame, runtime);
    Check(runtime.calls == std::vector<std::string>{"ui", "console"},
          "state zero renders UI and console only");

    runtime.calls.clear();
    runtime.words[kObject + 508u] = 1u;
    pal4::CombatSystemRender::Render(kObject, kFrame, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "world", "game-object", "actors", "ui", "console"},
          "active state renders world, actors, UI and console");
    runtime.calls.clear();
    runtime.words[kObject + 508u] = 6u;
    runtime.bytes[kObject + 516u] = 1u;
    pal4::CombatSystemRender::Render(kObject, kFrame, runtime);
    Check(runtime.calls == std::vector<std::string>{"ui", "console"},
          "inactive state six skips world and actors");
    runtime.calls.clear();
    runtime.words[kObject + 508u] = 7u;
    pal4::CombatSystemRender::Render(kObject, kFrame, runtime);
    Check(runtime.calls.empty(), "state seven renders nothing");
}

void DebugText() {
    Runtime runtime;
    pal4::CombatSystemRender::DrawDebugInfo(kObject, kFrame, runtime);
    Check(runtime.calls.empty(), "debug flag off skips every service");
    runtime.bytes[kObject + 518u] = 1u;
    pal4::CombatSystemRender::DrawDebugInfo(kObject, kFrame, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "game-state", "effects",
              "text:0:20:FPS: 007 PlayingEffect:4 RenderingObject:11",
              "bgm", "text:30:50:Bgm:field.mp3", "world-name",
              "text:60:80:World:River"},
          "debug text data, formatting and three line positions");

    runtime.calls.clear();
    runtime.words[kObject + 508u] = 2u;
    pal4::CombatSystemRender::Render(kObject, kFrame, runtime);
    Check(runtime.calls[0] == "world" &&
              runtime.calls[4] == "console" &&
              runtime.calls.back() == "text:60:80:World:River",
          "render calls debug info after console in active state");
}

}  // namespace

int main() {
    try {
        SoundAndRenderStates();
        DebugText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
