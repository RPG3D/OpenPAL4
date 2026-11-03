#include "pal4/actor_animation_control.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t actor = 0x1000u;

struct SearchRuntime final : pal4::IActorAnimationSearchRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> texts;
    bool found = true;
    bool include_match = true;
    std::vector<std::string> trace;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::string_view TextAt(std::uint32_t address) override {
        return texts[address];
    }
    bool EqualIgnoringCase(std::string_view a, std::string_view b) override {
        return a.size() == b.size() &&
            std::equal(a.begin(), a.end(), b.begin(),
                [](char left, char right) {
                    auto lower = [](char c) {
                        return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
                    };
                    return lower(left) == lower(right);
                });
    }
    void SetAnimationFound(bool value) override {
        found = value;
        trace.push_back(value ? "found" : "reset");
    }
    bool AnimationFound() override { return found; }
    void ProcessAnimations(
        std::uint32_t state, std::uint32_t search,
        const std::function<std::uint32_t(std::uint32_t,
                                          std::uint32_t)>& visitor) override {
        Check(state == actor + 368u && search == 0x9000u,
              "animation search state and key");
        trace.push_back("process");
        Check(visitor(0x9100u, search) == 0x9100u,
              "mismatching animation survives callback");
        if (include_match)
            Check(visitor(0x9200u, search) == 0u,
                  "matching animation stops callback");
    }
    std::uint32_t EventRouter() override { return 0; }
    std::uint32_t Dispatch(std::uint32_t, std::uint32_t,
                           const std::array<std::uint32_t, 2>&,
                           std::uint32_t) override { return 0; }
};

struct ControlRuntime final : pal4::IActorAnimationControlRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string>* trace = nullptr;
    std::int64_t converted = 0x123;
    float converted_input = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::int64_t FloatToInteger(float value) override {
        converted_input = value;
        return converted;
    }
    void InvokeActorSlot24(std::uint32_t target,
                           std::uint32_t color) override {
        trace->push_back("slot:" + std::to_string(target) + ":" +
                         std::to_string(color));
    }
    std::uint32_t ProcessClumpAtomics(std::uint32_t clump,
                                      std::uint32_t callback) override {
        Check(callback == 0x44u, "atomic callback forwarded");
        trace->push_back("atomics:" + std::to_string(clump));
        return clump + 7u;
    }
    void RenderCamera(std::uint32_t target,
                      std::uint32_t argument) override {
        Check(target == actor && argument == 9u, "camera arguments");
        trace->push_back("camera");
    }
    void CleanupCamera(std::uint32_t target) override {
        Check(target == actor, "camera cleanup actor");
        trace->push_back("cleanup-camera");
    }
};

struct ResourceRuntime final : pal4::IActorResourceRuntime {
    std::uint32_t attachment = actor + 1088u;
    std::map<std::uint32_t, std::string> stored;
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == actor + 20u) return 0x5000u;
        if (address == attachment + 20u) return 0x6000u;
        Check(address == attachment + 4u, "attachment read must stay in selected hand");
        return 0u;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == attachment + 460u && value == 1u, "load hand active flag");
    }
    std::string_view TextAt(std::uint32_t) override { return {}; }
    bool EqualIgnoringCase(std::string_view, std::string_view) override {
        return false;
    }
    bool CombatInactive() override { return false; }
    void ResetActorProperties(std::uint32_t target) override {
        Check(target == attachment, "reset selected attachment only");
    }
    void StartResourceContext(bool alternate) override {
        last_alternate = alternate;
    }
    void LoadComponentClump(std::uint32_t target, bool alternate,
                            std::string_view name) override {
        Check(target == attachment && alternate == last_alternate &&
              name == "weapon", "attachment clump parameters");
    }
    void SetTexture(std::uint32_t target, std::string_view name) override {
        Check(target == attachment && name == "texture", "attachment texture");
    }
    void EndResourceContext() override {}
    void UpdateAnimation(std::uint32_t context, std::string_view name,
                         std::uint32_t id) override {
        Check(context == 0x5000u && id == 27u,
              "attachment animation context and id");
        animation_name = std::string(name);
    }
    bool StoreDialogText(std::uint32_t field,
                         std::string_view value) override {
        Check(field == attachment + 428u && value == "dialog",
              "attachment dialog text");
        stored[field] = value;
        return true;
    }
    void StoreText(std::uint32_t field,
                   std::string_view value) override {
        Check((field == attachment + 412u && value == "primary") ||
              (field == attachment + 444u && value == "secondary"),
              "attachment text placement");
        stored[field] = value;
    }
    void UpdateRenderState(std::uint32_t target) override {
        Check(target == attachment, "attachment render update");
    }
    std::uint32_t CleanupComponent(std::uint32_t) override { return 0; }
    bool last_alternate = false;
    std::string animation_name;
};

struct RenderRuntime final : pal4::IActorRenderStateRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string>* trace = nullptr;
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        trace->push_back("active:" + std::to_string(address) + ":" +
                         std::to_string(value));
    }
    std::string_view TextAt(std::uint32_t) override { return {}; }
    void RemoveEffect(std::uint32_t address) override {
        trace->push_back("remove:" + std::to_string(address));
    }
    void HandleEvent(std::uint32_t handle, std::uint32_t context,
                     bool force) override {
        Check(context == 0xaau && !force, "child effect context");
        trace->push_back("event:" + std::to_string(handle));
    }
    std::uint32_t CombatCamera() override { return 0; }
    pal4::ActorEffectHandle LoadXmlEffect(std::uint32_t,
        std::string_view, std::uint32_t) override { return {}; }
    pal4::ActorEffectHandle CreateEffect(std::uint32_t,
        std::string_view) override { return {}; }
    void ResetTemporaryEffect() override {}
    void ConfigureEffect(std::uint32_t, const std::array<float, 3>&,
                         bool) override {}
    std::uint32_t EffectFrame(std::uint32_t) override { return 0; }
    void AddFrameChild(std::uint32_t, std::uint32_t) override {}
    void UpdateEffectProperties(std::uint32_t) override {}
    void TouchSkillSystem() override {}
    bool DialogNeedsUpdate(std::uint32_t) override { return false; }
    bool IsReadyToShow(std::uint32_t) override { return false; }
};

void SearchCallback() {
    SearchRuntime runtime;
    runtime.words[0x9004u] = 0x9300u;
    runtime.words[0x9114u] = 0x9400u;
    runtime.words[0x9214u] = 0x9500u;
    runtime.texts[0x9300u] = "run";
    runtime.texts[0x9400u] = "idle";
    runtime.texts[0x9500u] = "RUN";
    Check(pal4::ActorAnimationControl::FindAnimation(actor, 0x9000u, runtime) &&
          runtime.trace == std::vector<std::string>{
              "reset", "process", "found"},
          "resume resets and searches using existing compare callback");
    runtime.trace.clear();
    runtime.include_match = false;
    Check(!pal4::ActorAnimationControl::FindAnimation(actor, 0x9000u,
                                                     runtime) &&
          runtime.trace == std::vector<std::string>{"reset", "process"},
          "search clears prior match and reports no hit");
}

void ColorAndClumps() {
    std::vector<std::string> trace;
    ControlRuntime runtime;
    runtime.trace = &trace;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[actor + 644u] = 0x6000u;
    runtime.words[actor + 1108u] = 0x7000u;
    constexpr auto color = 0x23ffffffu;
    Check(pal4::ActorAnimationControl::SetAlpha(actor, 0.5f, 0x44u,
                                               runtime) == 0x7007u &&
          runtime.converted_input == 127.5f &&
          trace == std::vector<std::string>{
              "slot:4096:" + std::to_string(color), "atomics:20480",
              "slot:4720:" + std::to_string(color), "atomics:24576",
              "slot:5184:" + std::to_string(color), "atomics:28672"},
          "color byte and main/left/right clump order");
    Check(pal4::ActorAnimationControl::LeftHandClump(actor, runtime) ==
              0x6000u &&
          pal4::ActorAnimationControl::RightHandClump(actor, runtime) ==
              0x7000u, "hand clump accessors");
    trace.clear();
    runtime.words[actor + 644u] = 0;
    runtime.words[actor + 1108u] = 0;
    Check(pal4::ActorAnimationControl::SetAlpha(actor, 0.5f, 0x44u,
                                               runtime) == 0u &&
          trace == std::vector<std::string>{
              "slot:4096:" + std::to_string(color), "atomics:20480"},
          "missing children preserve zero return after main clump");
}

void AttachmentLoad() {
    ResourceRuntime runtime;
    const pal4::ActorAttachmentLoadArgs args{
        "weapon", "texture", "dialog", "primary", "secondary", 27u, true};
    Check(pal4::ActorAnimationControl::LoadRightHand(actor, args,
                                                     runtime) == 0u &&
          runtime.animation_name == "hook_RHWeapon" &&
          runtime.last_alternate, "right hand load arguments");
    const auto right = runtime.stored;
    runtime.attachment = actor + 624u;
    Check(pal4::ActorAnimationControl::LoadLeftHand(actor, args,
                                                    runtime) == 0u &&
          runtime.animation_name == "hook_LHWeapon",
          "left hand load arguments");
    for (const auto& [field, value] : right)
        Check(runtime.stored.at(field) == value,
              "loading left hand preserves previously loaded right hand");
    Check(runtime.stored.size() == 6u &&
          !runtime.stored.contains(actor + 428u),
          "each hand owns distinct fields and leaves body text intact");
}

void ActivationAndCleanup() {
    std::vector<std::string> trace;
    ControlRuntime control;
    control.trace = &trace;
    control.words[actor + 20u] = 0x5000u;
    control.words[actor + 644u] = 0x6000u;
    control.words[actor + 1108u] = 0x7000u;
    RenderRuntime render;
    render.trace = &trace;
    pal4::ActorAnimationControl::Activate(actor, 9u, 0x44u, 0xaau,
                                           control, render);
    Check(trace == std::vector<std::string>{
              "atomics:20480", "atomics:24576", "atomics:28672",
              "camera", "remove:5116", "remove:5108", "event:5124",
              "active:5180:1", "remove:5580", "remove:5572",
              "event:5588", "active:5644:1"},
          "activate processes clumps then camera and both children");
    trace.clear();
    pal4::ActorAnimationControl::Deactivate(actor, 0xaau,
                                             control, render);
    Check(trace == std::vector<std::string>{
              "cleanup-camera", "remove:5580", "remove:5572",
              "event:5588", "active:5644:0", "remove:5116",
              "remove:5108", "event:5124", "active:5180:0"},
          "deactivate cleans camera then right and left children");
    trace.clear();
    pal4::ActorAnimationControl::SetLeftHandActive(actor, 0u, 0xaau,
                                                    render);
    Check(trace.empty(), "unchanged child state is a no-op");
    pal4::ActorAnimationControl::SetLeftHandActive(actor, 2u, 0xaau,
                                                    render);
    Check(render.bytes[actor + 624u + 460u] == 2u &&
          trace.back() == "active:5180:2",
          "child state wrapper preserves the full input byte");
}

}  // namespace

int main() {
    try {
        SearchCallback();
        ColorAndClumps();
        AttachmentLoad();
        ActivationAndCleanup();
        std::cout << "actor_animation_control_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
