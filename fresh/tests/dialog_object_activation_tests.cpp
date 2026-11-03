#include "pal4/dialog_object.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectActivationRuntime {
    std::vector<std::string> calls;
    std::uint32_t text_address = 0x3000u;
    std::uint32_t effect = 0x4000u;
    std::uint32_t frame = 0x5000u;
    std::uint32_t written = 0xffffffffu;
    bool enabled = true;
    bool active = false;
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        Check(address == 0x2000u && offset == 4u, "record text word");
        calls.emplace_back("read-name"); return text_address;
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        Check(address == 0x2000u && offset == 16u, "record effect word");
        calls.emplace_back("write-effect"); written = value;
    }
    std::string_view ReadText(std::uint32_t address) override {
        Check(address == 0x3000u, "text address");
        calls.emplace_back("text"); return "sound";
    }
    std::uint32_t ResolveTextProperty(std::uint32_t manager,
                                      std::string_view name) override {
        Check(manager == 0x1000u &&
              name == (text_address ? "sound" : ""), "property lookup");
        calls.emplace_back("property"); return 0x6000u;
    }
    std::uint32_t HandleEffectEvent(std::uint32_t property,
                                    std::uint32_t owner,
                                    std::uint32_t parent) override {
        Check(property == 0x6000u && owner == 0x7000u && parent == 0x8000u,
              "event arguments");
        calls.emplace_back("handle"); return effect;
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled"); return enabled;
    }
    bool EffectIsActive(std::uint32_t value) override {
        Check(value == effect, "activity effect");
        calls.emplace_back("active"); return active;
    }
    void ProcessEffectEvent(std::uint32_t parent,
                            std::uint32_t value) override {
        Check(parent == 0x8000u && value == effect, "process arguments");
        calls.emplace_back("process");
    }
    std::uint32_t EffectFrame(std::uint32_t value) override {
        Check(value == effect, "frame effect");
        calls.emplace_back("frame"); return frame;
    }
    void AddFrameChild(std::uint32_t parent,
                       std::uint32_t child) override {
        Check(parent == 0x9000u && child == frame, "frame attachment");
        calls.emplace_back("add-child");
    }
};
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    state.words[3u] = 0x8000u;
    state.words[4u] = 0x1000u;
    state.words[28u] = 0x2000u;
    state.words[29u] = 0x2000u;
    state.words[43u] = 0x9000u;
    Runtime runtime;
    Check(pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.calls.empty(), "empty effect range succeeds");
    state.words[29u] = 0x2014u;
    runtime.effect = 0u;
    Check(!pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.written == 0u &&
          runtime.calls == std::vector<std::string>{
              "read-name", "text", "property", "handle", "write-effect"},
          "null effect stops before processing");
    runtime.effect = 0x4000u;
    runtime.frame = 0u;
    runtime.calls.clear();
    Check(!pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "read-name", "text", "property", "handle", "write-effect",
              "enabled", "process", "frame"},
          "missing effect frame fails after process");
    runtime.frame = 0x5000u;
    runtime.calls.clear();
    Check(pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.written == 0x4000u &&
          runtime.calls == std::vector<std::string>{
              "read-name", "text", "property", "handle", "write-effect",
              "enabled", "process", "frame", "frame", "add-child"},
          "enabled effect attaches frame after two frame queries");
    runtime.text_address = 0u;
    runtime.enabled = false;
    runtime.active = true;
    runtime.calls.clear();
    Check(pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "read-name", "property", "handle", "write-effect", "enabled",
              "active"},
          "active effect skips processing when system disabled");
    runtime.active = false;
    runtime.calls.clear();
    Check(pal4::DialogObject::ActivateEffects(state, 0x7000u, runtime) &&
          runtime.calls == std::vector<std::string>{
              "read-name", "property", "handle", "write-effect", "enabled",
              "active", "process", "frame", "frame", "add-child"},
          "inactive effect processes when system disabled");
}
