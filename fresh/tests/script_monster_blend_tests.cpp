#include "pal4/script_monster_blend.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptMonsterBlendRuntime {
public:
    pal4::ObjectBlendWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t monster = 0x5000u;
    std::uint32_t info = 0x7000u;
    bool allocation_fails = false;
    bool c01_succeeds = false;
    std::uint8_t alpha = 128u;
    std::uint8_t expected_mode = 7u;
    std::uint32_t expected_vtable = 0x849630u;
    std::uint32_t flags = 0u;
    float start_alpha = -1.0f;
    float end_alpha = -1.0f;
    float ratio = -1.0f;

    std::uint32_t MonsterManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindMonster(std::uint32_t manager,
                              std::string_view name) override {
        Check(manager == 0x4000u && name == "M01", "monster lookup");
        calls.emplace_back("find monster");
        return monster;
    }
    pal4::ObjectBlendWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x2cu, "monster blend allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::ObjectBlendWaitState& handler,
                       std::uint8_t mode, std::uint32_t queue) override {
        Check(&handler == &state && mode == expected_mode &&
                  queue == 0x6054u, "monster wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::ObjectBlendWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == expected_vtable &&
                  handler.object == monster && handler.duration == 2.0f &&
                  handler.elapsed == 0.0f, "monster wait layout");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue,
        pal4::ObjectBlendWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "monster wait queue");
        calls.emplace_back("add handler");
        return 0x1234u;
    }
    void SetMaterialFlags(std::uint32_t handle,
                          std::uint32_t value) override {
        Check(handle == monster, "monster material receiver");
        flags = value;
        calls.emplace_back("flags");
    }
    void LeaveCharacter(std::uint32_t handle) override {
        Check(handle == monster, "leave character receiver");
        calls.emplace_back("leave");
    }
    void EnterCharacter(std::uint32_t handle) override {
        Check(handle == monster, "enter character receiver");
        calls.emplace_back("enter");
    }
    void SetStateFlag(std::uint32_t handle,
                      std::uint32_t flag) override {
        Check(handle == monster && flag == 1u, "monster state flag");
        calls.emplace_back("state flag");
    }
    std::string_view MonsterName(std::uint32_t handle) override {
        Check(handle == monster, "monster name receiver");
        calls.emplace_back("name");
        return "internal-name";
    }
    std::uint32_t FindMonsterInfo(
        std::uint32_t manager, std::string_view name) override {
        Check(manager == 0x4000u && name == "internal-name",
              "monster info lookup");
        calls.emplace_back("find info");
        return info;
    }
    bool SetAction(std::uint32_t handle,
                   std::string_view action, float blend) override {
        Check(handle == monster && blend == 0.5f, "monster action operands");
        calls.emplace_back("action:" + std::string(action));
        return action == "C01" ? c01_succeeds : true;
    }
    void ReplaceComponent(std::uint32_t handle,
                          std::uint32_t record,
                          std::uint32_t mode) override {
        Check(handle == monster && record == info && mode == 1u,
              "monster component operands");
        calls.emplace_back("replace");
    }
    std::uint8_t InterpolateAlpha(float start, float end,
                                  float progress) override {
        start_alpha = start;
        end_alpha = end;
        ratio = progress;
        calls.emplace_back("interpolate");
        return alpha;
    }
    void CloseWait(pal4::ObjectBlendWaitState& handler) override {
        Check(&handler == &state, "monster wait close");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    Runtime out;
    pal4::ScriptMonsterBlend fade(out);
    Check(fade.BlendOut("M01", 2.0f, 7u) == 0x1234u &&
              out.flags == 0xffffffffu &&
              out.calls == std::vector<std::string>{
                  "manager", "find monster", "allocate", "executor",
                  "construct", "flush", "flags", "executor",
                  "add handler"},
          "monster fade-out creates handler then resets flags");

    Runtime in;
    in.expected_vtable = 0x84963cu;
    pal4::ScriptMonsterBlend fade_in(in);
    Check(fade_in.BlendIn("M01", 2.0f, 7u) == 0x1234u &&
              in.flags == 0x00ffffffu &&
              in.calls == std::vector<std::string>{
                  "manager", "find monster", "allocate", "executor",
                  "construct", "flush", "name", "manager", "find info",
                  "leave", "state flag", "enter", "action:C01",
                  "action:Z01", "replace", "flags", "executor",
                  "add handler"},
          "monster fade-in restores actor and falls back to Z01");

    Runtime no_info;
    no_info.expected_vtable = 0x84963cu;
    no_info.info = 0u;
    no_info.c01_succeeds = true;
    pal4::ScriptMonsterBlend no_info_fade(no_info);
    Check(no_info_fade.BlendIn("M01", 2.0f, 7u) == 0x1234u,
          "monster fade-in without info still queues");
    Check(no_info.calls == std::vector<std::string>{
              "manager", "find monster", "allocate", "executor",
              "construct", "flush", "name", "manager", "find info",
              "leave", "state flag", "enter", "action:C01", "flags",
              "executor", "add handler"},
          "existing C01 skips fallback and absent info skips replacement");

    Runtime missing;
    missing.monster = 0u;
    pal4::ScriptMonsterBlend missing_fade(missing);
    Check(missing_fade.BlendOut("M01", 2.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "find monster"},
          "missing monster skips fade handler");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptMonsterBlend failed_fade(failed);
    Check(failed_fade.BlendOut("M01", 2.0f, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "find monster", "allocate", "executor",
                  "add handler"},
          "failed monster wait allocation queues null");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptMonsterBlend fade(runtime);
    auto& state = runtime.state;
    state.object = runtime.monster;
    state.duration = 2.0f;
    fade.UpdateOut(state, 0.5f);
    Check(state.elapsed == 0.5f && runtime.ratio == 0.25f &&
              runtime.start_alpha == 255.0f &&
              runtime.end_alpha == 0.0f &&
              runtime.flags == 0x80ffffffu &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "flags"},
          "monster partial fade-out packs alpha");
    runtime.calls.clear();
    fade.UpdateOut(state, 1.5f);
    Check(state.elapsed == 2.0f &&
              runtime.calls == std::vector<std::string>{
                  "flags", "leave", "close"},
          "monster fade-out completion leaves arena before close");
    runtime.calls.clear();
    fade.FinishOut(state);
    Check(runtime.calls == std::vector<std::string>{
              "flags", "leave", "close"},
          "monster finalizer leaves unconditionally");

    state.elapsed = 0.0f;
    runtime.calls.clear();
    fade.UpdateIn(state, 0.5f);
    Check(runtime.start_alpha == 0.0f &&
              runtime.end_alpha == 255.0f &&
              runtime.ratio == 0.25f &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "flags"},
          "monster vtable shares fade-in update behavior");
    runtime.calls.clear();
    fade.UpdateIn(state, 2.0f);
    Check(state.elapsed == 2.0f &&
              runtime.flags == 0xffffffffu &&
              runtime.calls == std::vector<std::string>{"flags", "close"},
          "monster fade-in completion restores flags");
    runtime.calls.clear();
    state.elapsed=0.5f;
    fade.FinishIn(state);
    Check(runtime.flags==0xffffffffu && runtime.calls==std::vector<std::string>{"flags","close"},
          "cancelled monster fade-in restores colors and retains arena membership");
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
