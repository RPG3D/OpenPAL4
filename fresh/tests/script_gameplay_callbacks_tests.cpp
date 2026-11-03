#include "pal4/script_gameplay_callbacks.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct PostEventRuntime final : pal4::IScriptPostEventRuntime {
    bool has_state = true;
    std::uint32_t query_result = 1u;
    unsigned game_gets = 0;
    unsigned state_reads = 0;
    std::vector<std::string> calls;

    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0x4034u + 0x100u * game_gets++;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4034u + 0x100u * state_reads + 0x18cu,
              "game state slot for each singleton access");
        calls.emplace_back("read-state");
        return has_state ? 0x7000u + state_reads++ : 0u;
    }
    std::uint32_t QueryCurrentState(std::uint32_t state) override {
        Check(state == 0x7001u, "second current-state pointer");
        calls.emplace_back("query-state");
        return query_result;
    }
    std::uint32_t InitializeStateAndUi(
        std::uint32_t game, std::uint32_t state,
        std::uint32_t document) override {
        Check(game == 0x4234u && state == 5u && document == 0u,
              "post-event state-five transition");
        calls.emplace_back("initialize-state");
        return 0x1234u;
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade");
        return 0x8000u;
    }
    std::uint32_t SetTradeItemInfo(
        std::uint32_t trade, std::string_view id,
        std::uint32_t flag) override {
        Check(trade == 0x8000u && id == "0" && flag == 0u,
              "post-event trade reset operands");
        calls.emplace_back("trade-info");
        return 0x1256u;
    }
};

struct MonsterRuntime final : pal4::IMonsterPursuitCommandRuntime {
    std::vector<std::string> calls;
    void SetStopPursuitFlag(std::uint32_t monster) override {
        calls.push_back("stop:" + std::to_string(monster));
    }
    std::uint32_t MonsterManager() override {
        calls.emplace_back("monster-manager");
        return 0x1000u;
    }
    std::uint32_t Iterate(std::uint32_t manager, Callback callback,
                           std::uint32_t context) override {
        Check(manager == 0x1000u && context == 0u,
              "monster iterator context");
        calls.emplace_back("iterate");
        Check(callback(0x2000u, *this) == 0x2000u &&
              callback(0x2001u, *this) == 0x2001u,
              "callback returns original monster address");
        return 0xabcdu;
    }
};

struct CameraRuntime final : pal4::IScriptCameraModeRuntime {
    std::uint32_t current_mode = 0u;
    unsigned manager_gets = 0;
    std::vector<std::string> calls;
    std::uint32_t CameraSubsystem() override {
        calls.emplace_back("camera-manager");
        return 0x1000u + 0x100u * manager_gets++;
    }
    void SetModeByIndex(std::uint32_t manager,
                        std::uint32_t mode) override {
        Check(manager == 0x1000u && mode == 17u, "camera mode operands");
        calls.emplace_back("set-mode");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x11c0u, "second camera manager active mode");
        calls.emplace_back("read-mode");
        return current_mode;
    }
    std::uint32_t ActiveCameraInternal(std::uint32_t manager) override {
        Check(manager == 0x1200u, "third manager internal camera");
        calls.emplace_back("internal-camera");
        return 0x3000u;
    }
    void SetPosition(std::uint32_t camera,
                     std::uint32_t x, std::uint32_t y,
                     std::uint32_t z, std::uint32_t keep_look_at) override {
        Check(camera == 0x3000u && x == 0u && y == 0u && z == 0u &&
              keep_look_at == 1u, "camera position reset operands");
        calls.emplace_back("set-position");
    }
    std::uint32_t ActiveCameraObject(std::uint32_t manager) override {
        Check(manager == 0x1200u || manager == 0x1300u,
              "active camera manager access");
        calls.emplace_back("active-camera");
        return 0x4000u;
    }
    void SetLookAt(std::uint32_t camera,
                   std::uint32_t flag) override {
        Check(camera == 0x4000u && flag == 1u,
              "active camera look-at refresh");
        calls.emplace_back("look-at");
    }
    std::uint32_t ActorControl() override {
        calls.emplace_back("actor-control");
        return 0x5000u;
    }
    void FlushMainCameraTailYaw(std::uint32_t actor) override {
        Check(actor == 0x5000u, "camera actor flush operand");
        calls.emplace_back("flush-yaw");
    }
    std::uint32_t PlayerManager() override {
        calls.emplace_back("player-manager");
        return 0x6000u;
    }
    std::int32_t SetPlayerState(std::uint32_t manager,
                                std::uint8_t sync) override {
        Check(manager == 0x6000u && sync <= 1u,
              "player sync flag narrowed to low byte");
        calls.push_back("player-state:" + std::to_string(sync));
        return -7;
    }
};

void PostEventBranches() {
    PostEventRuntime bypass;
    Check(pal4::ScriptPostEventHandler::Handle(0x101u, bypass) == 0u &&
          bypass.calls.empty(), "nonzero, non-one mode returns low byte");

    PostEventRuntime absent;
    absent.has_state = false;
    Check(pal4::ScriptPostEventHandler::Handle(1u, absent) == 0x34u &&
          absent.calls == std::vector<std::string>{"game", "read-state"},
          "missing state returns game pointer low byte");

    PostEventRuntime not_ready;
    not_ready.query_result = 0x128au;
    Check(pal4::ScriptPostEventHandler::Handle(0u, not_ready) == 0x8au &&
          not_ready.calls == std::vector<std::string>{
              "game", "read-state", "game", "read-state", "query-state"},
          "non-ready state returns virtual result low byte");

    PostEventRuntime state;
    Check(pal4::ScriptPostEventHandler::Handle(1u, state) == 0x34u &&
          state.calls == std::vector<std::string>{
              "game", "read-state", "game", "read-state", "query-state",
              "game", "initialize-state"},
          "mode one initializes game state five");

    PostEventRuntime trade;
    Check(pal4::ScriptPostEventHandler::Handle(0u, trade) == 0x56u &&
          trade.calls == std::vector<std::string>{
              "game", "read-state", "game", "read-state", "query-state",
              "trade", "trade-info"},
          "mode zero enters trade UI with item zero");
}

void MonsterPursuit() {
    MonsterRuntime runtime;
    Check(pal4::MonsterPursuitController::StopOne(0x3000u, runtime) ==
              0x3000u &&
          runtime.calls == std::vector<std::string>{"stop:12288"},
          "single callback returns its monster");
    runtime.calls.clear();
    Check(pal4::MonsterPursuitController::StopAll(runtime) == 0xabcdu &&
          runtime.calls == std::vector<std::string>{
              "monster-manager", "iterate", "stop:8192", "stop:8193"},
          "manager-wide iteration invokes same callback and returns result");
}

void CameraModes() {
    for (const auto mode : {0u, 5u}) {
        CameraRuntime runtime;
        runtime.current_mode = mode;
        Check(pal4::ScriptCameraModeHandler::SetMode(17u, 0u, runtime) == -7 &&
              runtime.calls == std::vector<std::string>{
                  "camera-manager", "set-mode", "camera-manager",
                  "read-mode", "camera-manager", "active-camera",
                  "look-at", "actor-control", "flush-yaw",
                  "player-manager", "player-state:0"},
              "active mode zero or five always refreshes look-at and actor");
    }

    CameraRuntime no_sync;
    no_sync.current_mode = 3u;
    Check(pal4::ScriptCameraModeHandler::SetMode(
              17u, 0x100u, no_sync) == -7 &&
          no_sync.calls == std::vector<std::string>{
              "camera-manager", "set-mode", "camera-manager", "read-mode",
              "camera-manager", "internal-camera", "set-position",
              "player-manager", "player-state:0"},
          "other mode resets position and high flag byte is ignored");

    CameraRuntime sync;
    sync.current_mode = 3u;
    Check(pal4::ScriptCameraModeHandler::SetMode(17u, 0x101u, sync) == -7 &&
          sync.calls == std::vector<std::string>{
              "camera-manager", "set-mode", "camera-manager", "read-mode",
              "camera-manager", "internal-camera", "set-position",
              "camera-manager", "active-camera", "look-at",
              "player-manager", "player-state:1"},
          "other mode with low sync byte also refreshes look-at");
}

}  // namespace

int main() {
    try {
        PostEventBranches();
        MonsterPursuit();
        CameraModes();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
