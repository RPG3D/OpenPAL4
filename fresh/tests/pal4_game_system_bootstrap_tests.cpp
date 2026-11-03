#include "pal4/pal4_game_system_bootstrap.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Callback {
    std::uint32_t system;
    std::uint32_t offset;
    std::uint32_t function;
    bool operator==(const Callback&) const = default;
};

struct Runtime final : pal4::IPal4GameSystemBootstrapRuntime {
    std::vector<std::string> calls;
    std::vector<Callback> callbacks;
    std::uint32_t render_lookups = 0u;
    std::uint32_t game_lookups = 0u;
    std::int32_t state_result = -71;
    void SetGlobalFrameCallback(std::uint32_t global,
                                std::uint32_t callback) override {
        Check(global == 0x94d56cu && callback == 0x5c7f30u,
              "global frame callback address and target");
        calls.emplace_back("global callback");
    }
    std::uint32_t RenderSystem() override {
        calls.emplace_back("render system");
        return 0x8000u + ++render_lookups * 0x100u;
    }
    void SetRenderCallback(std::uint32_t system,
                           std::uint32_t offset,
                           std::uint32_t callback) override {
        calls.emplace_back("render callback");
        callbacks.push_back({system, offset, callback});
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game instance");
        return 0x10000u + ++game_lookups * 0x1000u;
    }
    void InitializeGameSystems(std::uint32_t game) override {
        Check(game == 0x11000u, "first game instance owns system init");
        calls.emplace_back("initialize systems");
    }
    std::uint32_t RegisterArenaEffectManager() override {
        calls.emplace_back("arena effects");
        return 0x3000u;
    }
    void SetUpdateHandler(std::uint32_t manager,
                          std::uint32_t callback) override {
        Check(manager == 0x3000u && callback == 0x5c8150u,
              "effect manager update handler");
        calls.emplace_back("update handler");
    }
    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera subsystem");
        return 0x4000u;
    }
    std::uint32_t ActiveCameraObject(std::uint32_t subsystem) override {
        Check(subsystem == 0x4000u, "active camera subsystem");
        calls.emplace_back("active camera");
        return 0x5000u;
    }
    std::uint32_t CameraContainer(std::uint32_t active_camera) override {
        Check(active_camera == 0x5000u, "active camera iterator");
        calls.emplace_back("camera container");
        return 0x6000u;
    }
    void InitializeTexture(std::uint32_t texture_object,
                           std::uint32_t container, std::uint32_t width,
                           std::uint32_t height) override {
        Check(texture_object == 0x12178u && container == 0x6000u &&
                  width == 256u && height == 256u,
              "game texture offset, camera container and dimensions");
        calls.emplace_back("texture");
    }
    std::int32_t InitializeStateAndUi(std::uint32_t game,
                                      std::uint32_t first,
                                      std::uint32_t second) override {
        Check(game == 0x13000u && first == 0u && second == 0u,
              "third game instance and zero state arguments");
        calls.emplace_back("state and ui");
        return state_result;
    }
};

}  // namespace

int main() {
    try {
        Runtime runtime;
        Check(pal4::Pal4GameSystemBootstrap::Initialize(runtime) == -71,
              "bootstrap forwards state/UI signed return");
        constexpr std::array<std::uint32_t, 12> offsets{
            0x30u, 0x34u, 0x38u, 0x3cu, 0x40u, 0x44u,
            0x48u, 0x54u, 0x58u, 0x5cu, 0x60u, 0x64u};
        Check(runtime.render_lookups == 12u &&
                  runtime.callbacks.size() == offsets.size() &&
                  runtime.game_lookups == 3u,
              "each render slot reacquires system; game is acquired three times");
        for (std::size_t i = 0; i < offsets.size(); ++i) {
            const auto expected = i == 2u ? 0x5c83d0u :
                                  i == 10u ? 0x5c8410u :
                                  i == 11u ? 0x5c8440u : 0x5c83a0u;
            Check(runtime.callbacks[i] == Callback{
                      0x8000u + static_cast<std::uint32_t>(i + 1u) * 0x100u,
                      offsets[i], expected},
                  "all render slot addresses and callback targets");
            Check(runtime.calls[1u + 2u * i] == "render system" &&
                      runtime.calls[2u + 2u * i] == "render callback",
                  "each callback follows its own render-system lookup");
        }
        Check(runtime.calls[25] == "game instance" &&
                  runtime.calls[26] == "initialize systems" &&
                  runtime.calls[27] == "arena effects" &&
                  runtime.calls[28] == "update handler" &&
                  runtime.calls[29] == "camera subsystem" &&
                  runtime.calls[30] == "active camera" &&
                  runtime.calls[31] == "game instance" &&
                  runtime.calls[32] == "camera container" &&
                  runtime.calls[33] == "texture" &&
                  runtime.calls[34] == "game instance" &&
                  runtime.calls[35] == "state and ui",
              "game systems, effects, camera texture and UI startup order");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
