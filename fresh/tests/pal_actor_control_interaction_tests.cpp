#include "pal4/pal_actor_control_dispatch.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IActorControlDispatchRuntime {
    pal4::ActorInteractionInput input{};
    std::unordered_map<std::uint32_t, std::uint32_t> memory;
    std::unordered_map<std::uint32_t,
                       std::vector<pal4::ActorControlEffectPoint>> vertices;
    std::vector<std::uint32_t> candidates;
    std::vector<std::string> events;
    std::uint32_t ray_npc = 0u;
    std::uint32_t ray_dialog = 0u;
    std::uint32_t ray_effect = 0u;
    std::uint32_t position_npc = 0u;
    std::uint32_t position_dialog = 0u;
    double actor_property = 2.0;
    float npc_distance = 1.0f;
    bool dialog_enabled = false;
    bool dialog_in_range = false;
    bool process_dialog = false;

    void Float(std::uint32_t address, float value) {
        memory[address] = std::bit_cast<std::uint32_t>(value);
    }
    pal4::ActorInteractionInput Input() override {
        events.emplace_back("input");
        return input;
    }
    pal4::ActorInteractionPoint CursorClient() override {
        events.emplace_back("cursor");
        return {10, 20};
    }
    pal4::ActorInteractionRay PixelRay(
        std::uint32_t camera, pal4::ActorInteractionPoint point) override {
        Check(camera == 0x300u && point.x == 10 && point.y == 20,
              "pixel ray arguments");
        events.emplace_back("ray");
        return {42.0f};
    }
    std::uint32_t NpcByRay(
        const pal4::ActorInteractionRay& ray) override {
        Check(ray[0] == 42.0f, "npc ray");
        events.emplace_back("npc ray");
        return ray_npc;
    }
    std::uint32_t DialogByRay(
        const pal4::ActorInteractionRay& ray) override {
        Check(ray[0] == 42.0f, "dialog ray");
        events.emplace_back("dialog ray");
        return ray_dialog;
    }
    std::uint32_t EffectByRay(
        std::uint32_t camera, pal4::ActorInteractionPoint point) override {
        Check(camera == 0x300u && point.x == 10,
              "effect ray arguments");
        events.emplace_back("effect ray");
        return ray_effect;
    }
    std::uint32_t NpcByPosition(std::uint32_t position) override {
        Check(position == 0x1000u + 172u, "npc coordinate");
        events.emplace_back("npc position");
        return position_npc;
    }
    std::uint32_t DialogByPosition(
        std::uint32_t position, float property) override {
        Check(position == 0x1000u + 172u && property == 2.0f,
              "dialog coordinate arguments");
        events.emplace_back("dialog position");
        return position_dialog;
    }
    std::vector<std::uint32_t> EffectCandidates() override {
        events.emplace_back("effect list");
        return candidates;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto it = memory.find(address);
        return it == memory.end() ? 0u : it->second;
    }
    float ReadFloat(std::uint32_t address) override {
        return std::bit_cast<float>(ReadWord(address));
    }
    double ActorProperty(std::uint32_t actor) override {
        Check(actor == 0x1000u, "actor property address");
        events.emplace_back("property");
        return actor_property;
    }
    pal4::ActorControlEffectPoint EffectVertex(
        std::uint32_t effect, std::int32_t index) override {
        events.emplace_back("vertex " + std::to_string(index));
        return vertices.at(effect).at(static_cast<std::size_t>(index));
    }
    float PlanarMagnitude(
        pal4::ActorControlEffectPoint offset) override {
        Check(offset.y == 0.0f, "planar distance only");
        events.emplace_back("distance");
        return IActorControlInteractionRuntime::PlanarMagnitude(offset);
    }
    void SetCursor(std::string_view resource) override {
        events.emplace_back("cursor " + std::string(resource));
    }
    double NpcDistance(pal4::PalActorControlState&,
                       std::uint32_t position) override {
        Check(position == ray_npc + 172u, "npc distance position");
        events.emplace_back("npc distance");
        return npc_distance;
    }
    void ExecuteScript(std::uint32_t script) override {
        Check(script == 0x777u, "npc script address");
        events.emplace_back("script");
    }
    bool DialogEnabled(std::uint32_t dialog) override {
        Check(dialog == ray_dialog || dialog == position_dialog,
              "dialog enabled address");
        events.emplace_back("dialog enabled");
        return dialog_enabled;
    }
    bool DialogInRange(std::uint32_t dialog,
                       std::uint32_t position,
                       float property) override {
        Check(dialog == ray_dialog && position == 0x1000u + 172u &&
                  property == 2.0f, "dialog range arguments");
        events.emplace_back("dialog range");
        return dialog_in_range;
    }
    bool ProcessDialogEvent(pal4::PalActorControlState&,
                            std::uint32_t dialog) override {
        Check(dialog == ray_dialog || dialog == position_dialog,
              "dialog event address");
        events.emplace_back("dialog event");
        return process_dialog;
    }
    void AttachDialog(std::uint32_t dialog,
                      std::uint32_t position) override {
        Check((dialog == ray_dialog || dialog == position_dialog) &&
                  position == 0x1000u + 172u,
              "dialog attach arguments");
        events.emplace_back("dialog attach");
    }
    void TriggerEffect(std::uint32_t action) override {
        Check(action == 0x800u || action == 0x801u,
              "effect action address");
        events.emplace_back("effect action");
    }
};

pal4::PalActorControlState ReadyState() {
    pal4::PalActorControlState state{};
    state.words[1] = 0x1000u;
    return state;
}

void ClickPrimary(Runtime& runtime) {
    runtime.input.primary_count = 1;
    runtime.input.primary_mode = 1u;
}

void ClickSecondary(Runtime& runtime) {
    runtime.input.secondary_count = 1;
    runtime.input.secondary_mode = 1u;
}

void ConfigureNpc(Runtime& runtime, std::uint32_t npc) {
    runtime.memory[npc + 0x1f0u] = 1u;
    runtime.memory[npc + 0x654u] = 0x6000u;
    runtime.memory[0x6000u + 64u] = 1u;
    runtime.memory[0x6000u + 28u] = 1u;
    runtime.memory[0x6000u + 24u] = 0x777u;
    runtime.Float(0x6000u + 68u, 3.0f);
}

void ConfigureEffect(Runtime& runtime, std::uint32_t effect,
                     std::vector<pal4::ActorControlEffectPoint> points) {
    runtime.memory[effect + 4u] = 5u;
    runtime.memory[effect + 19u * 4u] = 1u;
    runtime.memory[effect + 84u] = 0x9000u;
    runtime.memory[effect + 88u] = 0x9000u +
        static_cast<std::uint32_t>(points.size()) * 80u;
    runtime.memory[effect + 64u] = 0x800u;
    runtime.vertices[effect] = std::move(points);
    runtime.Float(0x1000u + 172u, 1.0f);
    runtime.Float(0x1000u + 176u, 2.0f);
    runtime.Float(0x1000u + 180u, 3.0f);
    runtime.Float(0x1000u + 236u, 3.0f);
}

void TestGatesAndActiveRelease() {
    auto state = ReadyState();
    Runtime runtime;
    state.words[1] = 0u;
    Check(!pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && runtime.events.empty(),
          "missing actor stops before input access");
    state.words[1] = 0x1000u;
    runtime.input.blocking_count = 1;
    runtime.input.blocking_mode = 0x11u;
    Check(!pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) &&
              runtime.events == std::vector<std::string>{"input"},
          "blocking input suppresses interaction");
    runtime = {};
    state.words[20] = 0x5000u;
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[20] == 0x5000u &&
              runtime.events == std::vector<std::string>{
                  "input", "input", "input"},
          "active target stays selected without release");
    runtime.input.secondary_release = 1u;
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[20] == 0u &&
              state.words[21] == 0u && state.words[22] == 0u,
          "secondary release clears all three target fields");
    state.words[21] = 0x7000u;
    runtime.input.secondary_release = 0u;
    runtime.input.primary_release = 1u;
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[21] == 0u,
          "primary release clears active target");
}

void TestNpcRoutes() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.ray_npc = 0x5000u;
    ConfigureNpc(runtime, runtime.ray_npc);
    ClickPrimary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[20] == 0x5000u &&
              runtime.events.front() == "input" &&
              runtime.events[1] == "cursor" &&
              runtime.events[2] == "ray" &&
              runtime.events[3] == "npc ray" &&
              runtime.events[4] == "dialog ray" &&
              runtime.events[5] == "effect ray" &&
              runtime.events.back() == "script",
          "ray NPC selection queries in order and executes script");

    state = ReadyState();
    runtime = {};
    runtime.position_npc = 0x5100u;
    ConfigureNpc(runtime, runtime.position_npc);
    ClickSecondary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[20] == 0x5100u &&
              runtime.events.back() == "script",
          "secondary click finds NPC by actor coordinate");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "cursor default.cur") != runtime.events.end(),
          "no ray target uses default cursor");
}

void TestDialogRoutes() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.ray_dialog = 0x7000u;
    runtime.memory[0x7000u + 0x184u] = 1u;
    runtime.dialog_enabled = true;
    runtime.dialog_in_range = true;
    runtime.process_dialog = true;
    ClickPrimary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[21] == 0x7000u &&
              runtime.events.back() == "dialog attach",
          "ray dialog commits after range and event gates");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "cursor pickup.ani") != runtime.events.end(),
          "available dialog uses pickup cursor");

    state = ReadyState();
    runtime.events.clear();
    runtime.process_dialog = false;
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[21] == 0u &&
              runtime.events.back() == "dialog event",
          "rejected dialog event still consumes the click");

    state = ReadyState();
    runtime = {};
    runtime.position_dialog = 0x7100u;
    runtime.dialog_enabled = true;
    runtime.process_dialog = true;
    ClickSecondary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[21] == 0x7100u &&
              runtime.events.back() == "dialog attach",
          "secondary click falls back to coordinate dialog");
}

void TestEffectRoutes() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.ray_effect = 0x8000u;
    ConfigureEffect(runtime, 0x8000u,
                    {{20.0f, 2.0f, 3.0f}, {2.0f, 2.0f, 5.0f}});
    ClickPrimary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[22] == 0x8000u &&
              runtime.events.back() == "effect action",
          "ray effect selects nearest planar vertex within radius");

    state = ReadyState();
    runtime.events.clear();
    ConfigureEffect(runtime, 0x8000u, {{4.0f, 2.0f, 7.0f}});
    runtime.Float(0x1000u + 236u, 2.5f);
    Check(!pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[22] == 0u,
          "ray effect rejects equal radius and planar magnitude");

    state = ReadyState();
    runtime = {};
    ConfigureEffect(runtime, 0x8100u, {{3.0f, 2.0f, 3.0f}});
    ConfigureEffect(runtime, 0x8200u, {{2.0f, 2.0f, 3.0f}});
    runtime.memory[0x8200u + 64u] = 0x801u;
    runtime.candidates = {0x8100u, 0x8200u};
    ClickSecondary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[22] == 0x8200u &&
              runtime.events.back() == "effect action",
          "list fallback composes the reconstructed effect selector");
}

void TestCrossBranchPriority() {
    auto state = ReadyState();
    Runtime runtime;
    runtime.ray_npc = 0x5000u;
    ConfigureNpc(runtime, runtime.ray_npc);
    runtime.npc_distance = 4.0f;
    runtime.ray_dialog = 0x7000u;
    runtime.memory[0x7000u + 0x184u] = 1u;
    runtime.dialog_enabled = true;
    runtime.dialog_in_range = true;
    runtime.process_dialog = true;
    ClickPrimary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[20] == 0u &&
              state.words[21] == 0x7000u &&
              runtime.events.back() == "dialog attach",
          "out-of-range NPC falls through to the ray dialog");

    state = ReadyState();
    runtime = {};
    runtime.ray_dialog = 0x7000u;
    runtime.memory[0x7000u + 0x184u] = 1u;
    runtime.dialog_enabled = true;
    runtime.dialog_in_range = false;
    runtime.ray_effect = 0x8000u;
    ConfigureEffect(runtime, 0x8000u, {{2.0f, 2.0f, 5.0f}});
    ClickPrimary(runtime);
    Check(pal4::PalActorControlInteraction::Handle(
              state, 0x300u, runtime) && state.words[21] == 0u &&
              state.words[22] == 0x8000u,
          "out-of-range dialog falls through to ray effect");
}

void TestDistanceToNpc() {
    struct DistanceRuntime : pal4::IActorControlInteractionRuntime {
        std::unordered_map<std::uint32_t, float> values;
        pal4::ActorControlEffectPoint observed{};
        std::uint32_t reads = 0u;
        float ReadFloat(std::uint32_t address) override {
            ++reads;
            return values.at(address);
        }
        double SpatialMagnitude(
            pal4::ActorControlEffectPoint offset) override {
            observed = offset;
            return IActorControlInteractionRuntime::SpatialMagnitude(offset);
        }
    } runtime;
    auto state = ReadyState();
    Check(pal4::PalActorControlInteraction::DistanceToNpc(
              state, 0u, runtime) ==
                  static_cast<double>(std::numeric_limits<float>::max()) &&
              runtime.reads == 0u,
          "null NPC position returns original FLT_MAX sentinel");
    state.words[1] = 0u;
    Check(pal4::PalActorControlInteraction::DistanceToNpc(
              state, 0x5000u, runtime) ==
                  static_cast<double>(std::numeric_limits<float>::max()) &&
              runtime.reads == 0u,
          "null actor returns sentinel without reading memory");
    state.words[1] = 0x1000u;
    runtime.values = {{0x1000u + 172u, 1.0f},
                      {0x1000u + 176u, 2.0f},
                      {0x1000u + 180u, 3.0f},
                      {0x5000u, 4.0f},
                      {0x5004u, 6.0f},
                      {0x5008u, 15.0f}};
    Check(pal4::PalActorControlInteraction::DistanceToNpc(
              state, 0x5000u, runtime) == 13.0 &&
              runtime.observed.x == 3.0f &&
              runtime.observed.y == 4.0f &&
              runtime.observed.z == 12.0f && runtime.reads == 6u,
          "three-dimensional offset reaches magnitude boundary");
    Check(runtime.NpcDistance(state, 0x5000u) == 13.0,
          "default runtime composes the reconstructed distance helper");
}

}  // namespace

int main() {
    TestGatesAndActiveRelease();
    TestNpcRoutes();
    TestDialogRoutes();
    TestEffectRoutes();
    TestCrossBranchPriority();
    TestDistanceToNpc();
}
