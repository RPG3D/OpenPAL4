#include "pal4/script_npc_fly_move.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float left, float right) {
    return std::fabs(left - right) < 0.001f;
}

class Runtime final : public pal4::IScriptNpcFlyRuntime {
public:
    pal4::NpcFlyWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t npc = 0x5000u;
    bool allocation_fails = false;
    bool negative_x = false;
    pal4::NpcFlyVector start{1.0f, 2.0f, 3.0f};
    pal4::NpcFlyVector written{};

    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x4000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x4000u && name == "NPC01", "fly NPC lookup");
        calls.emplace_back("find NPC");
        return npc;
    }
    void SetAnimation(std::uint32_t handle,
                      std::string_view name) override {
        Check(handle == npc && name == "C02", "fly animation");
        calls.emplace_back("animation C02");
    }
    void SetAnimationFlag(std::uint32_t handle,
                          std::uint32_t flag) override {
        Check(handle == npc && flag == 1u, "fly animation flag");
        calls.emplace_back("flag 1");
    }
    pal4::NpcFlyVector ReadPosition(std::uint32_t handle) override {
        Check(handle == npc, "fly start position receiver");
        calls.emplace_back("read position");
        return start;
    }
    pal4::NpcFlyWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x54u, "fly wait size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::NpcFlyWaitState& handler,
                       std::uint8_t mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && mode == 7u && queue == 0x6054u,
              "fly wait constructor");
        calls.emplace_back("construct");
    }
    void AssignName(pal4::NpcFlyWaitState& handler,
                    std::string_view name) override {
        Check(&handler == &state && name == "NPC01-Npc-Rotate" &&
                  handler.phase == 2u &&
                  handler.vtable_address == 0x849690u,
              "fly handler name and phase");
        calls.emplace_back("name");
    }
    float DistanceFromVector(const pal4::NpcFlyVector& vector) override {
        Check(vector[0] == (negative_x ? -3.0f : 3.0f) &&
                  vector[1] == 0.0f && vector[2] == 4.0f,
              "fly distance input");
        calls.emplace_back("distance");
        return 5.0f;
    }
    void ClampFacingVector(pal4::NpcFlyVector& vector) override {
        Check(vector[0] == (negative_x ? -3.0f : 3.0f) &&
                  vector[2] == 4.0f, "fly facing input");
        vector = {negative_x ? -0.6f : 0.6f, 0.0f, 0.8f};
        calls.emplace_back("clamp");
    }
    float AngleFromZ(float normalized_z) override {
        Check(Near(normalized_z, 0.8f), "fly angle input");
        calls.emplace_back("angle");
        return 1.0f;
    }
    void SetRotation(std::uint32_t handle,
                     const pal4::NpcFlyVector& rotation,
                     std::uint32_t mode) override {
        const auto expected = static_cast<float>(
            (negative_x ? 6.283185307179586 - 1.0 : 1.0) * 57.29578);
        Check(handle == npc && mode == 0u &&
                  rotation[0] == 0.0f && rotation[2] == 0.0f &&
                  Near(rotation[1], expected), "fly initial yaw");
        calls.emplace_back("rotation");
    }
    void FlushWait(pal4::NpcFlyWaitState& handler) override {
        Check(&handler == &state && handler.npc == npc &&
                  handler.start == start &&
                  handler.target == pal4::NpcFlyVector{
                      negative_x ? -2.0f : 4.0f, 2.0f, 7.0f} &&
                  handler.speed == 69.0f &&
                  Near(handler.duration, 5.0f / 69.0f) &&
                  handler.elapsed == 0.0f, "fly state before flush");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::NpcFlyWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "fly handler queue");
        calls.emplace_back("add handler");
    }
    std::uint32_t CleanupName() override {
        calls.emplace_back("cleanup name");
        return 0x1234u;
    }
    float Interpolate(float first, float second,
                       float ratio) override {
        Check(ratio == 0.25f || ratio == 1.0f,
              "fly interpolation ratio");
        calls.emplace_back("interpolate");
        return first + (second - first) * ratio;
    }
    void SetPosition(std::uint32_t handle,
                     const pal4::NpcFlyVector& position) override {
        Check(handle == npc, "fly position setter");
        written = position;
        calls.emplace_back("set position");
    }
    void CloseWait(pal4::NpcFlyWaitState& handler) override {
        Check(&handler == &state, "fly close receiver");
        calls.emplace_back("close");
    }
    std::int32_t SetIdleAction(std::uint32_t handle,
                               std::string_view action) override {
        Check(handle == npc && action == "C01", "fly idle action");
        calls.emplace_back("idle C01");
        return 19;
    }
};

void CreateCases() {
    Runtime runtime;
    pal4::ScriptNpcFlyMove fly(runtime);
    Check(fly.Create("NPC01", {4.0f, 2.0f, 7.0f}, 7u) == 0x1234u &&
              runtime.calls == std::vector<std::string>{
                  "npc manager", "find NPC", "animation C02", "flag 1",
                  "allocate", "executor", "construct", "name",
                  "read position", "distance", "clamp", "angle",
                  "rotation", "flush", "executor", "add handler",
                  "cleanup name"},
          "fly creation sets action, direction and named 84-byte handler");

    Runtime negative;
    negative.negative_x = true;
    pal4::ScriptNpcFlyMove negative_fly(negative);
    Check(negative_fly.Create("NPC01", {-2.0f, 2.0f, 7.0f}, 7u) == 0x1234u,
          "negative X direction uses wrapped yaw");

    Runtime missing;
    missing.npc = 0u;
    pal4::ScriptNpcFlyMove missing_fly(missing);
    Check(missing_fly.Create("NPC01", {4.0f, 2.0f, 7.0f}, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "find NPC"},
          "missing NPC skips animation and handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptNpcFlyMove failed_fly(failed);
    Check(failed_fly.Create("NPC01", {4.0f, 2.0f, 7.0f}, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "npc manager", "find NPC", "animation C02", "flag 1",
                  "allocate", "executor", "add handler", "cleanup name"},
          "failed fly allocation still queues null");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptNpcFlyMove fly(runtime);
    auto& state = runtime.state;
    state.npc = runtime.npc;
    state.start = runtime.start;
    state.target = {5.0f, 6.0f, 7.0f};
    state.duration = 2.0f;
    fly.Update(state, 0.5);
    Check(state.elapsed == 0.5f &&
              runtime.written == pal4::NpcFlyVector{2, 3, 4} &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "interpolate", "interpolate",
                  "set position"},
          "fly update interpolates three coordinates");
    runtime.calls.clear();
    fly.Update(state, 1.5);
    Check(state.elapsed == 2.0f && runtime.written == state.target &&
              runtime.calls == std::vector<std::string>{
                  "close", "interpolate", "interpolate", "interpolate",
                  "set position"},
          "fly completion closes before writing final position");
    state.duration = 0.0f;
    runtime.calls.clear();
    fly.Update(state, 0.5);
    Check(runtime.calls == std::vector<std::string>{"close"},
          "nonpositive fly duration closes immediately");

    runtime.calls.clear();
    Check(fly.Finish(state) == 19 &&
              runtime.calls == std::vector<std::string>{
                  "close", "idle C01"},
          "fly finalizer closes then restores C01");
}

}  // namespace

int main() {
    try {
        CreateCases();
        UpdateCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
