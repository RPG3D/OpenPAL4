#include "pal4/script_player_blend.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPlayerBlendRuntime {
public:
    pal4::PlayerBlendWaitState state{};
    std::vector<std::string> calls;
    bool actor_exists = true;
    bool allocation_fails = false;
    bool blending_in = false;
    bool npc = false;
    bool npc_info_exists = true;
    std::uint32_t expected_material = 0xffffffffu;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x4000u && index == 2, "blend actor lookup");
        calls.emplace_back("indexed");
        return actor_exists ? 0x5000u : 0u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("npc manager");
        return 0x8000u;
    }
    std::uint32_t NpcByName(std::uint32_t manager,
                            std::string_view name) override {
        Check(manager == 0x8000u && (name == "NPC01" || name.empty()),
              "blend NPC lookup");
        calls.emplace_back("npc lookup");
        return actor_exists ? 0x5000u : 0u;
    }
    std::string_view ActorName(std::uint32_t actor) override {
        Check(actor == 0x5000u, "blend actor name receiver");
        calls.emplace_back("actor name");
        return "NPC01";
    }
    std::uint32_t NpcInfoByName(std::uint32_t manager,
                                std::string_view name) override {
        Check(manager == 0x8000u && name == "NPC01",
              "blend NPC info lookup");
        calls.emplace_back("npc info");
        return npc_info_exists ? 0x9000u : 0u;
    }
    void ProcessNpcEvents(std::uint32_t actor,
                          std::uint32_t info,
                          std::uint32_t mode) override {
        Check(actor == 0x5000u && info == 0x9000u && mode == 0u,
              "blend NPC event operands");
        calls.emplace_back("npc events");
    }
    pal4::PlayerBlendWaitState* Allocate(std::uint32_t size) override {
        Check(size == 0x2cu, "blend wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void Construct(pal4::PlayerBlendWaitState& candidate,
                   std::uint8_t mode, std::uint32_t queue) override {
        Check(&candidate == &state && mode == 7u && queue == 0x6054u,
              "blend wait constructor");
        calls.emplace_back("construct");
    }
    void Flush(pal4::PlayerBlendWaitState& candidate) override {
        Check(&candidate == &state && state.actor == 0x5000u &&
                  state.duration == 1.0f && state.elapsed == 0.0f &&
                  state.vtable_address == (npc ?
                      (blending_in ? 0x8495acu : 0x8495a0u) :
                      (blending_in ? 0x849594u : 0x849588u)),
              "blend handler fields before flush");
        calls.emplace_back("flush");
    }
    void SetMaterialFlags(std::uint32_t actor,
                          std::uint32_t flags) override {
        Check(actor == 0x5000u && flags == expected_material,
              "clump material flags");
        calls.emplace_back("material");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x7000u;
    }
    void RemoveFromArena(std::uint32_t arena,
                         std::uint32_t actor) override {
        Check(arena == 0x7000u && actor == 0x5000u,
              "blend arena remove operands");
        calls.emplace_back("remove");
    }
    void AddToArena(std::uint32_t arena,
                    std::uint32_t actor) override {
        Check(arena == 0x7000u && actor == 0x5000u,
              "blend arena add operands");
        calls.emplace_back("add arena");
    }
    void SetStateFlag(std::uint32_t actor,
                      std::uint32_t value) override {
        Check(actor == 0x5000u && value == 1u,
              "blend-in state flag");
        calls.emplace_back("state flag");
    }
    float Interpolate(float from, float to,
                      float fraction, std::uint32_t mode) override {
        Check(from == (blending_in ? 0.0f : 255.0f) && to == (blending_in ? 255.0f : 0.0f) && fraction == 0.5f &&
                  mode == 0u,
              "blend-out interpolation operands");
        calls.emplace_back("interpolate");
        return 127.5f;
    }
    std::int32_t ConvertAlpha(float value) override {
        Check(value == 127.5f, "blend-out alpha conversion input");
        calls.emplace_back("convert alpha");
        return 128;
    }
    void Close(pal4::PlayerBlendWaitState& candidate) override {
        Check(&candidate == &state, "blend close receiver");
        calls.emplace_back("close");
    }
    std::uint32_t AddHandler(std::uint32_t queue,
                             pal4::PlayerBlendWaitState* candidate) override {
        Check(queue == 0x6054u &&
                  candidate == (allocation_fails ? nullptr : &state),
              "blend handler queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
};

void StartCases() {
    Runtime out;
    pal4::ScriptPlayerBlend blend_out(out);
    Check(blend_out.BlendOut(2, 1.0f, 7u) == 0x1234u &&
              out.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor",
                  "construct", "flush", "material", "executor",
                  "add handler"},
          "blend-out handler setup and material flags");

    Runtime in;
    in.blending_in = true;
    in.expected_material = 0x00ffffffu;
    pal4::ScriptPlayerBlend blend_in(in);
    Check(blend_in.BlendIn(2, 1.0f, 7u) == 0x1234u &&
              in.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor",
                  "construct", "flush", "arena", "remove", "state flag",
                  "arena", "add arena", "material", "executor",
                  "add handler"},
          "blend-in removes, changes state and re-adds before material flags");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerBlend missing_blend(missing);
    Check(missing_blend.BlendOut(2, 1.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "indexed"},
          "missing player skips blend allocation");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerBlend failed_blend(failed);
    Check(failed_blend.BlendIn(2, 1.0f, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "indexed", "allocate", "executor",
                  "add handler"},
          "failed blend allocation still queues null");
}

void CallbackCases() {
    Runtime runtime;
    pal4::ScriptPlayerBlend blend(runtime);
    auto& state = runtime.state;
    state.actor = 0x5000u;
    state.duration = 1.0f;
    runtime.expected_material = 0x80ffffffu;
    blend.UpdateOut(state, 0.5);
    Check(state.elapsed == 0.5f &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "convert alpha", "material"},
          "partial blend packs alpha into material high byte");

    runtime.calls.clear();
    runtime.expected_material = 0xffffffffu;
    blend.UpdateOut(state, 1.0);
    Check(state.elapsed == 1.0f &&
              runtime.calls == std::vector<std::string>{
                  "material", "arena", "remove", "close"},
          "completed fade clamps, removes and closes");

    runtime.calls.clear();
    blend.FinishOut(state);
    Check(runtime.calls == std::vector<std::string>{
              "material", "arena", "remove", "close"},
          "explicit finalize has the same removal sequence");
    runtime.calls.clear();
    blend.Cleanup(state);
    Check(runtime.calls == std::vector<std::string>{
              "material", "close"},
          "generic effect cleanup resets material then closes");
}

void NpcCases() {
    Runtime out;
    out.npc = true;
    pal4::ScriptPlayerBlend out_blend(out);
    Check(out_blend.BlendNpcOut("NPC01", 1.0f, 7u) == 0x1234u &&
              out.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate", "executor",
                  "construct", "flush", "material", "executor",
                  "add handler"},
          "NPC fade-out uses distinct vtable and shared material setup");

    out.calls.clear();
    out.expected_material = 0x80ffffffu;
    out_blend.UpdateOut(out.state, 0.5);
    Check(out.calls == std::vector<std::string>{
              "interpolate", "convert alpha", "material"},
          "NPC fade-out callback preserves shared alpha interpolation");
    out.calls.clear();
    out.expected_material = 0xffffffffu;
    out_blend.FinishOut(out.state);
    Check(out.calls == std::vector<std::string>{
              "material", "arena", "remove", "close"},
          "NPC fade-out finalizer removes actor after resetting material");

    Runtime in;
    in.npc = true;
    in.blending_in = true;
    in.expected_material = 0x00ffffffu;
    pal4::ScriptPlayerBlend in_blend(in);
    Check(in_blend.BlendNpcIn("NPC01", 1.0f, 7u) == 0x1234u &&
              in.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate", "executor",
                  "construct", "flush", "actor name", "npc manager",
                  "npc info", "arena", "remove", "state flag",
                  "npc events", "arena", "add arena", "material",
                  "executor", "add handler"},
          "NPC fade-in processes info after state change, before arena add");

    in.calls.clear();
    in.expected_material = 0x80ffffffu;
    in_blend.UpdateIn(in.state, 0.5);
    Check(in.calls == std::vector<std::string>{"interpolate", "convert alpha", "material"},
          "fade-in interpolates alpha in the opposite direction");
    in.calls.clear();
    in.expected_material = 0xffffffffu;
    in_blend.UpdateIn(in.state, 0.5);
    Check(in.state.elapsed == 1.0f &&
          in.calls == std::vector<std::string>{"material", "close"},
          "fade-in completes opaque and keeps the actor in the arena");

    Runtime no_info;
    no_info.npc = true;
    no_info.blending_in = true;
    no_info.npc_info_exists = false;
    no_info.expected_material = 0x00ffffffu;
    pal4::ScriptPlayerBlend no_info_blend(no_info);
    Check(no_info_blend.BlendNpcIn("", 1.0f, 7u) == 0x1234u &&
              no_info.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate", "executor",
                  "construct", "flush", "actor name", "npc manager",
                  "npc info", "arena", "remove", "state flag",
                  "arena", "add arena", "material", "executor",
                  "add handler"},
          "NPC fade-in skips events when info lookup fails");

    Runtime missing;
    missing.actor_exists = false;
    pal4::ScriptPlayerBlend missing_blend(missing);
    Check(missing_blend.BlendNpcOut("NPC01", 1.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc manager", "npc lookup"},
          "missing NPC skips fade allocation");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPlayerBlend failed_blend(failed);
    Check(failed_blend.BlendNpcIn("NPC01", 1.0f, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "npc manager", "npc lookup", "allocate", "executor",
                  "add handler"},
          "NPC fade-in queues null when allocation fails");
}

}  // namespace

int main() {
    try {
        StartCases();
        CallbackCases();
        NpcCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
