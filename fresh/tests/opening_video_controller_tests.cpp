#include "pal4/opening_video_controller.h"
#include "pal4/crt_noop_atexit_registration.h"

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

struct Runtime final : pal4::IOpeningVideoRuntime,
                       pal4::ICrtNoopAtExitRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint8_t playing = 0u;
    bool throw_on_transition = false;
    double observed_delta = 0.0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return memory[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        memory[address] = value;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui-manager");
        return 0x8000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        calls.push_back("ui-flag " + std::to_string(manager) + " " +
                        std::to_string(value));
    }
    std::uint32_t InitializeConsole() override {
        calls.emplace_back("console");
        return 0x8100u;
    }
    std::uint32_t BinkPlayer() override {
        calls.emplace_back("bink-player");
        return 0x8200u;
    }
    void RenderVideo(std::uint32_t player, std::uint32_t record) override {
        calls.push_back("render " + std::to_string(player) + " " +
                        std::to_string(record));
    }
    std::uint8_t IsPlaying(std::uint32_t state) override {
        calls.push_back("is-playing " + std::to_string(state));
        return playing;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("script-executor");
        return 0x8300u;
    }
    void ProcessScriptWaitHandlers(std::uint32_t queue, double delta) override {
        calls.push_back("waits " + std::to_string(queue));
        observed_delta = delta;
    }
    std::uint32_t ScreenEffect() override {
        calls.emplace_back("screen-effect");
        return 0x8400u;
    }
    void AdvanceScreenEffect(std::uint32_t effect, float delta) override {
        calls.push_back("advance " + std::to_string(effect));
        Check(delta == 0.25f, "screen effect receives original float delta");
    }
    std::uint32_t ConstructXmlDocument() override {
        calls.emplace_back("xml-construct");
        return 0x8500u;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game-instance");
        return 0x8600u;
    }
    void InitializeStateAndUI(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override {
        calls.push_back("transition " + std::to_string(game) + " " +
                        std::to_string(state) + " " +
                        std::to_string(document));
        if (throw_on_transition) throw std::runtime_error("transition failed");
    }
    std::uint32_t DestroyXmlDocument(std::uint32_t document) override {
        calls.push_back("xml-destroy " + std::to_string(document));
        return 77u;
    }
    int RegisterAtExit(std::uint32_t callback) override {
        calls.push_back("atexit " + std::to_string(callback));
        return -9;
    }
};

void Lifecycle() {
    Runtime runtime;
    using pal4::OpeningVideoController;
    Check(OpeningVideoController::Construct(0x7000u, runtime) == 0x7000u &&
          runtime.memory[0x7000u] == 0x8485b0u,
          "constructor installs video-controller vtable");
    runtime.calls.clear();
    Check(OpeningVideoController::Initialize(0x7000u, 1u, 99u, runtime) == 1u &&
          runtime.calls == std::vector<std::string>{
              "ui-manager", "ui-flag 32768 1", "console",
              "write 33024 0", "write 28676 1"},
          "initialization sets UI flag, clears console and stores first argument");
    runtime.calls.clear();
    for (std::uint32_t flags = 0u; flags < 4u; ++flags) {
        runtime.calls.clear();
        Check(OpeningVideoController::Destroy(0x7000u, flags, runtime) ==
                  0x7000u &&
              runtime.memory[0x7000u] == 0x8485c8u &&
              runtime.calls.size() == 1u + (flags & 1u) &&
              runtime.calls.front() == "write 28672 8685000",
              "derived deleting destructor restores base vtable before optional free");
        runtime.calls.clear();
        Check(OpeningVideoController::DestroyBase(0x7000u, flags, runtime) ==
                  0x7000u &&
              runtime.calls.size() == 1u + (flags & 1u),
              "base deleting destructor honors low flag bit");
    }
}

void RenderAndUpdate() {
    Runtime runtime;
    using pal4::OpeningVideoController;
    OpeningVideoController::Render(0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{"read 36864"},
          "empty video record skips Bink initialization");
    runtime.memory[0x9000u] = 1u;
    runtime.calls.clear();
    OpeningVideoController::Render(0x9000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "read 36864", "bink-player", "render 33280 36864"},
          "active video record initializes and renders Bink player");

    runtime.memory[0x7000u + 4u] = 1u;
    runtime.calls.clear();
    runtime.playing = 1u;
    Check(OpeningVideoController::Update(0x7000u, 0.25f, runtime) == 1u &&
          runtime.observed_delta == 0.25 &&
          runtime.calls == std::vector<std::string>{
              "script-executor", "waits 33620", "screen-effect",
              "advance 33792", "bink-player", "is-playing 33284"},
          "playing video processes waits and advances screen effect without state switch");

    runtime.playing = 0u;
    runtime.memory[0x7000u + 4u] = 2u;
    runtime.calls.clear();
    Check(OpeningVideoController::Update(0x7000u, 0.25f, runtime) == 0u &&
          runtime.calls.back() == "read 28676",
          "finished video in another state does not initialize game UI");

    runtime.memory[0x7000u + 4u] = 1u;
    runtime.calls.clear();
    Check(OpeningVideoController::Update(0x7000u, 0.25f, runtime) == 77u &&
          std::vector<std::string>(runtime.calls.end() - 5,
                                   runtime.calls.end()) ==
              std::vector<std::string>{
                  "read 28676", "xml-construct", "game-instance",
                  "transition 34304 1 34048", "xml-destroy 34048"},
          "finished state-one video constructs, uses and destroys XML document");

    runtime.calls.clear();
    runtime.throw_on_transition = true;
    bool threw = false;
    try { static_cast<void>(OpeningVideoController::Update(
        0x7000u, 0.25f, runtime)); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw && runtime.calls.back() == "xml-destroy 34048",
          "exception path destroys the temporary XML document");
}

void SharedCrt() {
    Runtime runtime;
    std::uint8_t guard = 0xf0u;
    Check(pal4::CrtNoopAtExitRegistration::Run(guard, runtime) == -9 &&
          guard == 0xf1u &&
          runtime.calls == std::vector<std::string>{"atexit 6161760"},
          "0x58ad60 repeats shared CRT no-op registration");
}

}  // namespace

int main() {
    try {
        Lifecycle();
        RenderAndUpdate();
        SharedCrt();
        std::cout << "opening video controller tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
