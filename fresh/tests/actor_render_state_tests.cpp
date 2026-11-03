#include "pal4/actor_render_state.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr auto actor = 0x1000u;
constexpr auto first = actor + 396u;
constexpr auto second = actor + 388u;
constexpr auto third = actor + 404u;

struct Runtime final : pal4::IActorRenderStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::vector<std::string> trace;
    std::vector<std::string> names;
    std::vector<std::uint32_t> clumps;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> attachments;
    std::vector<std::uint32_t> frame_results{0xb000u, 0xb100u};
    std::uint32_t frame_calls = 0;
    std::uint32_t load_calls = 0;
    std::uint32_t clump_after_first_load = 0;
    std::uint32_t last_event_context = 0;
    std::array<float, 3> rotation{};
    bool enabled = false;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        trace.push_back("active-flag");
    }
    std::string_view TextAt(const std::uint32_t address) override {
        return texts.at(address);
    }
    void RemoveEffect(const std::uint32_t handle) override {
        trace.push_back("remove:" + std::to_string(handle));
    }
    void HandleEvent(const std::uint32_t handle,
                     const std::uint32_t context,
                     const bool force) override {
        Check(handle == third && !force,
              "effect event arguments");
        last_event_context = context;
        trace.push_back("event");
    }
    std::uint32_t CombatCamera() override {
        trace.push_back("camera");
        return 0x9000u;
    }
    pal4::ActorEffectHandle LoadXmlEffect(
        const std::uint32_t camera, const std::string_view name,
        const std::uint32_t clump) override {
        Check(camera == 0x9000u, "combat camera passed to XML effect");
        names.emplace_back(name);
        clumps.push_back(clump);
        trace.push_back("load-xml");
        if (++load_calls == 1u && clump_after_first_load)
            words[actor + 20u] = clump_after_first_load;
        return {load_calls * 10u, load_calls * 10u + 1u};
    }
    pal4::ActorEffectHandle CreateEffect(
        const std::uint32_t camera,
        const std::string_view name) override {
        Check(camera == 0x9000u, "combat camera passed to third effect");
        names.emplace_back(name);
        trace.push_back("create");
        return {30u, 31u};
    }
    void ResetTemporaryEffect() override {
        trace.push_back("reset-temp");
    }
    void ConfigureEffect(const std::uint32_t handle,
                         const std::array<float, 3>& angles,
                         const bool active) override {
        Check(handle == third, "third handle configured");
        rotation = angles;
        enabled = active;
        trace.push_back("configure");
    }
    std::uint32_t EffectFrame(const std::uint32_t handle) override {
        Check(handle == third, "third handle frame lookup");
        trace.push_back("effect-frame");
        return frame_results.at(frame_calls++);
    }
    void AddFrameChild(const std::uint32_t parent,
                       const std::uint32_t child) override {
        attachments.emplace_back(parent, child);
        trace.push_back("add-child");
    }
    void UpdateEffectProperties(const std::uint32_t handle) override {
        Check(handle == third, "third handle properties");
        trace.push_back("update-effect");
    }
    void TouchSkillSystem() override { trace.push_back("skill"); }
    bool DialogNeedsUpdate(const std::uint32_t handle) override {
        Check(handle == third, "dialog handle");
        trace.push_back("dialog-ready");
        return true;
    }
    bool IsReadyToShow(const std::uint32_t handle) override {
        trace.push_back(handle == second ? "second-ready" : "first-ready");
        return true;
    }
};

void NoActorFrame() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[actor + 432u] = 0x6000u;
    runtime.texts[0x6000u] = "effect-one.xml";
    pal4::ActorRenderStateUpdater::Update(actor, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "remove:4492", "remove:4484", "event", "camera",
              "load-xml", "camera", "load-xml", "skill",
              "dialog-ready", "second-ready", "first-ready"} &&
          runtime.names ==
              std::vector<std::string>{"effect-one.xml", ""} &&
          runtime.clumps ==
              std::vector<std::uint32_t>{0x5000u, 0x5000u} &&
          runtime.words[first] == 10u && runtime.words[first + 4u] == 11u &&
          runtime.words[second] == 20u &&
          runtime.words[second + 4u] == 21u &&
          runtime.words[third] == 0u,
          "two XML effects update without third effect when frame is absent");
}

void FrameAttachmentAndClumpReread() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.clump_after_first_load = 0x5100u;
    runtime.words[0x5100u + 4u] = 0x5200u;
    runtime.words[0x5200u + 4u] = 0xa000u;
    runtime.words[actor + 432u] = 0x6000u;
    runtime.words[actor + 416u] = 0x6100u;
    runtime.words[actor + 448u] = 0x6200u;
    runtime.texts[0x6000u] = "first.xml";
    runtime.texts[0x6100u] = "second.xml";
    runtime.texts[0x6200u] = "third.xml";
    pal4::ActorRenderStateUpdater::Update(actor, runtime);
    Check(runtime.clumps ==
              std::vector<std::uint32_t>{0x5000u, 0x5100u} &&
          runtime.names == std::vector<std::string>{
              "first.xml", "second.xml", "third.xml"} &&
          runtime.words[third] == 30u && runtime.words[third + 4u] == 31u &&
          runtime.rotation == std::array<float, 3>{90.0f, 0.0f, 0.0f} &&
          runtime.enabled && runtime.frame_calls == 2u &&
          runtime.attachments ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0xa000u, 0xb100u}},
          "third effect uses refreshed clump, rotation and second frame lookup");
    const auto reset = std::find(runtime.trace.begin(), runtime.trace.end(),
                                 "reset-temp");
    const auto configure = std::find(runtime.trace.begin(), runtime.trace.end(),
                                     "configure");
    const auto attach = std::find(runtime.trace.begin(), runtime.trace.end(),
                                  "add-child");
    const auto update = std::find(runtime.trace.begin(), runtime.trace.end(),
                                  "update-effect");
    Check(reset < configure && configure < attach && attach < update,
          "temporary handle reset and effect setup precede frame attachment");
}

void ThirdEffectWithoutFrame() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[0x5000u + 4u] = 0x5100u;
    runtime.words[0x5100u + 4u] = 0xa000u;
    runtime.frame_results = {0u};
    pal4::ActorRenderStateUpdater::Update(actor, runtime);
    Check(runtime.frame_calls == 1u && runtime.attachments.empty() &&
          runtime.words[third] == 30u &&
          std::count(runtime.trace.begin(), runtime.trace.end(), "event") ==
              2 &&
          std::find(runtime.trace.begin(), runtime.trace.end(),
                    "update-effect") == runtime.trace.end(),
          "unattached third effect is handled and still reaches readiness calls");
}

void ActiveStateTransitions() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    pal4::ActorRenderStateUpdater::SetActiveState(
        actor, 1u, 0xabcdu, runtime);
    Check(runtime.bytes[actor + 460u] == 1u &&
          runtime.trace.front() == "remove:4492" &&
          runtime.trace.back() == "active-flag" &&
          runtime.last_event_context == second,
          "activation with clump updates effects before writing active byte");
    runtime.trace.clear();
    pal4::ActorRenderStateUpdater::SetActiveState(
        actor, 1u, 0xabcdu, runtime);
    Check(runtime.trace.empty(), "unchanged active byte is a no-op");
    pal4::ActorRenderStateUpdater::SetActiveState(
        actor, 0u, 0xabcdu, runtime);
    Check(runtime.bytes[actor + 460u] == 0u &&
          runtime.trace == std::vector<std::string>{
              "remove:4492", "remove:4484", "event", "active-flag"} &&
          runtime.last_event_context == 0xabcdu,
          "deactivation removes effects and forwards caller effect context");
    runtime.words[actor + 20u] = 0;
    runtime.trace.clear();
    pal4::ActorRenderStateUpdater::SetActiveState(
        actor, 2u, 0xabcdu, runtime);
    Check(runtime.bytes[actor + 460u] == 2u &&
          runtime.trace == std::vector<std::string>{
              "remove:4492", "remove:4484", "event", "active-flag"},
          "nonzero activation without clump uses effect cleanup branch");
}

struct ClumpRuntime final : pal4::IActorClumpRenderRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> trace;
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
        trace.push_back("render-flag");
    }
    void RenderClump(const std::uint32_t clump) override {
        Check(clump == 0x5000u, "clump render target");
        trace.push_back("clump");
    }
    void RenderCamera(const std::uint32_t camera,
                      const std::uint32_t argument) override {
        Check(camera == actor + 56u && argument == 7u,
              "camera render target and argument");
        trace.push_back("camera");
    }
};

void ClumpRenderGates() {
    ClumpRuntime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[actor + 36u] = 1u;
    runtime.words[actor + 24u] = 1u;
    runtime.words[actor + 44u] = 1u;
    runtime.words[actor + 52u] = 1u;
    runtime.words[actor + 48u] = 1u;
    pal4::ActorClumpRenderer::Render(actor, 7u, runtime);
    Check(runtime.trace.empty(), "inactive actor skips all rendering");
    runtime.bytes[actor + 460u] = 1u;
    pal4::ActorClumpRenderer::Render(actor, 7u, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "render-flag", "clump", "camera"} &&
          runtime.words[actor + 28u] == 1u,
          "active clump renders before optional camera");
    runtime.trace.clear();
    runtime.words[actor + 24u] = 0u;
    pal4::ActorClumpRenderer::Render(actor, 7u, runtime);
    Check(runtime.trace == std::vector<std::string>{"camera"},
          "camera-only path skips clump render");
    runtime.trace.clear();
    runtime.words[actor + 48u] = 0u;
    pal4::ActorClumpRenderer::Render(actor, 7u, runtime);
    Check(runtime.trace.empty(), "camera gate needs all three fields");
    runtime.words[actor + 36u] = 0u;
    runtime.words[actor + 48u] = 1u;
    pal4::ActorClumpRenderer::Render(actor, 7u, runtime);
    Check(runtime.trace.empty(), "main render gate requires actor +36");
}

}  // namespace

int main() {
    try {
        NoActorFrame();
        FrameAttachmentAndClumpReread();
        ThirdEffectWithoutFrame();
        ActiveStateTransitions();
        ClumpRenderGates();
        std::cout << "actor_render_state_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
