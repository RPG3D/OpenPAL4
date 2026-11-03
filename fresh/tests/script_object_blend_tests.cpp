#include "pal4/script_object_blend.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptObjectBlendRuntime {
public:
    pal4::ObjectBlendWaitState state{};
    std::vector<std::string> calls;
    std::uint32_t object = 0x5000u;
    bool allocation_fails = false;
    bool has_animated = true;
    float start_alpha = -1.0f;
    float end_alpha = -1.0f;
    float ratio = -1.0f;
    std::uint8_t alpha = 128u;
    std::uint32_t flags = 0u;
    std::uint8_t expected_mode = 7u;
    std::uint32_t expected_vtable = 0x849618u;

    std::uint32_t ObjectManager() override {
        calls.emplace_back("manager");
        return 0x4000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x4000u && name == "box", "blend lookup");
        calls.emplace_back("find");
        return object;
    }
    pal4::ObjectBlendWaitState* AllocateWait(
        std::uint32_t bytes) override {
        Check(bytes == 0x2cu, "blend wait size");
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
                  queue == 0x6054u, "blend wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::ObjectBlendWaitState& handler) override {
        Check(&handler == &state &&
                  handler.vtable_address == expected_vtable &&
                  handler.object == object && handler.duration == 2.0f &&
                  handler.elapsed == 0.0f, "blend state before flush");
        calls.emplace_back("flush");
    }
    std::uint32_t AddHandler(
        std::uint32_t queue,
        pal4::ObjectBlendWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "blend queue handler");
        calls.emplace_back("add");
        return 0x1234u;
    }
    void SetMaterialFlags(std::uint32_t handle,
                          std::uint32_t value) override {
        Check(handle == object, "blend material receiver");
        flags = value;
        calls.emplace_back("flags");
    }
    bool HasAnimatedObject(std::uint32_t handle) override {
        Check(handle == object, "animated object receiver");
        calls.emplace_back("has animated");
        return has_animated;
    }
    void RemoveAnimatedObject(std::uint32_t handle) override {
        Check(handle == object, "remove animated object");
        calls.emplace_back("remove");
    }
    void SetVisible(std::uint32_t handle, bool visible) override {
        Check(handle == object && visible, "show animated object");
        calls.emplace_back("visible");
    }
    void AddAnimatedObject(std::uint32_t handle) override {
        Check(handle == object, "add animated object");
        calls.emplace_back("add animated");
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
        Check(&handler == &state, "blend close receiver");
        calls.emplace_back("close");
    }
};

void CreateCases() {
    Runtime out;
    pal4::ScriptObjectBlend fade(out);
    Check(fade.BlendOut("box", 2.0f, 7u) == 0x1234u &&
              out.flags == 0xffffffffu &&
              out.calls == std::vector<std::string>{
                  "manager", "find", "allocate", "executor",
                  "construct", "flush", "flags", "executor", "add"},
          "fade-out creation resets flags after handler flush");

    Runtime in;
    in.expected_vtable = 0x849624u;
    pal4::ScriptObjectBlend fade_in(in);
    Check(fade_in.BlendIn("box", 2.0f, 7u) == 0x1234u &&
              in.flags == 0x00ffffffu &&
              in.calls == std::vector<std::string>{
                  "manager", "find", "allocate", "executor",
                  "construct", "flush", "remove", "visible",
                  "add animated", "flags", "executor", "add"},
          "fade-in removes, shows and re-adds object before zero alpha");

    Runtime missing;
    missing.object = 0u;
    pal4::ScriptObjectBlend missing_fade(missing);
    Check(missing_fade.BlendOut("box", 2.0f, 7u) == 0u &&
              missing.calls == std::vector<std::string>{
                  "manager", "find"}, "missing object skips fade");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptObjectBlend failed_fade(failed);
    Check(failed_fade.BlendOut("box", 2.0f, 7u) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "manager", "find", "allocate", "executor", "add"},
          "allocation failure queues null without material changes");
}

void UpdateCases() {
    Runtime runtime;
    pal4::ScriptObjectBlend fade(runtime);
    auto& state = runtime.state;
    state.object = runtime.object;
    state.duration = 2.0f;
    fade.UpdateOut(state, 0.5f);
    Check(state.elapsed == 0.5f && runtime.ratio == 0.25f &&
              runtime.start_alpha == 255.0f &&
              runtime.end_alpha == 0.0f &&
              runtime.flags == 0x80ffffffu &&
              runtime.calls == std::vector<std::string>{
                  "has animated", "interpolate", "flags"},
          "partial fade-out packs interpolated alpha into high byte");

    runtime.calls.clear();
    fade.UpdateOut(state, 1.5f);
    Check(state.elapsed == 2.0f &&
              runtime.flags == 0xffffffffu &&
              runtime.calls == std::vector<std::string>{
                  "has animated", "flags", "remove", "close"},
          "completed fade-out restores flags then removes object");

    state.duration = 0.0f;
    runtime.calls.clear();
    fade.UpdateOut(state, 0.5f);
    Check(runtime.calls == std::vector<std::string>{"close"},
          "nonpositive fade-out duration closes immediately");
    state.duration = 2.0f;
    runtime.has_animated = false;
    runtime.calls.clear();
    fade.UpdateOut(state, 0.5f);
    Check(runtime.calls == std::vector<std::string>{
              "has animated", "close"},
          "fade-out closes when animated object is absent");

    runtime.calls.clear();
    fade.FinishOut(state);
    Check(runtime.calls == std::vector<std::string>{
              "has animated", "close"},
          "fade-out finalizer skips missing animated object");
    runtime.has_animated = true;
    runtime.calls.clear();
    fade.FinishOut(state);
    Check(runtime.calls == std::vector<std::string>{
              "has animated", "flags", "remove", "close"},
          "fade-out finalizer restores flags before removal");

    state.elapsed = 0.0f;
    runtime.calls.clear();
    fade.UpdateIn(state, 0.5f);
    Check(state.elapsed == 0.5f && runtime.ratio == 0.25f &&
              runtime.start_alpha == 0.0f &&
              runtime.end_alpha == 255.0f &&
              runtime.flags == 0x80ffffffu &&
              runtime.calls == std::vector<std::string>{
                  "interpolate", "flags"},
          "partial fade-in interpolates alpha upward");
    runtime.calls.clear();
    fade.UpdateIn(state, 3.0f);
    Check(state.elapsed == 2.0f &&
              runtime.flags == 0xffffffffu &&
              runtime.calls == std::vector<std::string>{"flags", "close"},
          "completed fade-in clamps elapsed and restores flags");
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
