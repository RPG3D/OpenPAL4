#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectResetRuntime {
    std::vector<std::string> calls;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    bool enabled = true;
    bool active = false;
    void CleanupObjectCollection(std::uint32_t container) override {
        Check(container == 0x5000u, "object cleanup receives this dialog's task container");
        calls.emplace_back("objects");
    }
    void DestroyTaskContainer(std::uint32_t container) override {
        Check(container == 0x5000u, "task container destruction");
        calls.emplace_back("destroy-container");
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "task container free");
        calls.emplace_back("free");
    }
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        calls.emplace_back("read:" + std::to_string(address));
        Check(offset == 16u, "effect record slot");
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        calls.emplace_back("write:" + std::to_string(address));
        Check(offset == 4u && value == 0u, "task handle clear");
        words[{address, offset}] = value;
    }
    bool EffectsEnabled() override {
        calls.emplace_back("enabled"); return enabled;
    }
    bool EffectIsActive(std::uint32_t effect) override {
        Check(effect == 0x6000u, "effect activity");
        calls.emplace_back("active"); return active;
    }
    void SetEffectMode(std::uint32_t effect, std::uint32_t mode) override {
        Check(effect == 0x6000u && mode == 0u, "effect virtual +48 mode zero");
        calls.emplace_back("mode:0");
    }
    void ReleaseEffectReferences(std::uint32_t effect) override {
        Check(effect == 0x6000u, "four effect attachment releases");
        calls.emplace_back("release-refs");
    }
    void ProcessEffectEvent(std::uint32_t owner,
                            std::uint32_t effect) override {
        Check(owner == 0x9000u && effect == 0x6000u,
              "effect reset event");
        calls.emplace_back("event");
    }
};
pal4::DialogObjectState State() {
    pal4::DialogObjectState state{};
    state.words[1u] = 0x10203040u;
    state.words[2u] = 0x5000u;
    state.words[3u] = 0x9000u;
    state.words[20u] = 0xaabbccddu;
    state.words[24u] = 0x2000u;
    state.words[25u] = 0x2010u;
    state.words[28u] = 0x3000u;
    state.words[29u] = 0x3028u;
    for (auto index = 44u; index <= 52u; ++index)
        state.words[index] = 0xffffffffu;
    return state;
}
Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words[{0x2000u, 4u}] = 0x1111u;
    runtime.words[{0x2008u, 4u}] = 0x2222u;
    runtime.words[{0x3000u, 16u}] = 0x6000u;
    runtime.words[{0x3014u, 16u}] = 0u;
    return runtime;
}
}  // namespace

int main() {
    auto state = State();
    auto runtime = MakeRuntime();
    Check(pal4::DialogObject::ResetRuntimeState(state, runtime) == 0x3028u &&
          state.words[2u] == 0u && state.words[1u] == 0x10203001u &&
          state.words[20u] == 0xaa000000u &&
          runtime.words.at({0x2000u, 4u}) == 0u &&
          runtime.words.at({0x2008u, 4u}) == 0u &&
          runtime.words.at({0x3000u, 16u}) == 0x6000u &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free", "write:8192",
              "write:8200", "read:12288", "enabled", "mode:0",
              "release-refs", "event", "read:12308"},
          "manager cleanup, task reset and enabled effect order");
    const auto one = std::bit_cast<std::uint32_t>(1.0f);
    Check(state.words[44u] == 0u && state.words[45u] == one &&
          state.words[46u] == one && state.words[47u] == one &&
          state.words[48u] == one && state.words[49u] == one &&
          state.words[50u] == 0u && state.words[51u] == 0u &&
          state.words[52u] == 0u, "native transform defaults");

    state = State();
    state.words[2u] = 0u;
    runtime = MakeRuntime();
    runtime.enabled = false;
    runtime.active = true;
    Check(pal4::DialogObject::ResetRuntimeState(state, runtime) == 0x3028u &&
          state.words[1u] == 0x10203040u &&
          state.words[20u] == 0xaabbccddu &&
          state.words[44u] == 0xffffffffu &&
          runtime.calls == std::vector<std::string>{
              "write:8192", "write:8200", "read:12288", "enabled",
              "active", "read:12308"},
          "without manager transforms stay and active disabled effect skips");

    state = State();
    runtime = MakeRuntime();
    runtime.enabled = false;
    Check(pal4::DialogObject::ResetRuntimeState(state, runtime) == 0x3028u &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free", "write:8192",
              "write:8200", "read:12288", "enabled", "active", "mode:0",
              "release-refs", "event", "read:12308"},
          "inactive disabled effect follows release path");

    state = State();
    runtime = MakeRuntime();
    state.words[25u] = state.words[24u];
    state.words[29u] = state.words[28u];
    Check(pal4::DialogObject::ResetRuntimeState(state, runtime) == 0x3000u &&
          runtime.calls == std::vector<std::string>{
              "objects", "destroy-container", "free"},
          "empty arrays still release manager and return effect end");
}
