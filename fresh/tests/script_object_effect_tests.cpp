#include "pal4/script_object_effect.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptObjectEffectRuntime {
    std::uint32_t object = 0x5000u;
    std::uint32_t object_name = 0x7000u;
    std::uint32_t effect_name = 0x8000u;
    std::uint8_t needs_update = 0u;
    std::uint8_t initial_ready = 0u;
    std::uint8_t loaded_ready = 0u;
    std::uint32_t ready_calls = 0u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x2004u) {
            calls.emplace_back("object name"); return object_name;
        }
        if (address == 0x1004u) {
            calls.emplace_back("effect name"); return effect_name;
        }
        Check(address == object + 0x14u, "object effect type offset");
        calls.emplace_back("type"); return 0x123u;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty"); return 0x9000u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object manager"); return 0x3000u;
    }
    std::uint32_t FindByName(std::uint32_t manager,
                             std::uint32_t name) override {
        Check(manager == 0x3000u && name ==
                  (object_name == 0u ? 0x9000u : object_name),
              "object lookup name fallback");
        calls.emplace_back("find"); return object;
    }
    std::uint8_t NeedsUpdate(std::uint32_t address) override {
        Check(address == object + 0x1ccu, "active effect slot");
        calls.emplace_back("needs update"); return needs_update;
    }
    void UpdateContent(std::uint32_t address) override {
        Check(address == object + 0x1ccu, "update active effect slot");
        calls.emplace_back("update");
    }
    std::uint8_t IsReady(std::uint32_t address) override {
        Check(address == object + 0x1d4u, "loaded effect slot");
        calls.emplace_back("ready");
        return ready_calls++ == 0u ? initial_ready : loaded_ready;
    }
    std::uint8_t DisplayAndCleanup(std::uint32_t address) override {
        Check(address == object + 0x1d4u,
              "display and clean previously loaded effect");
        calls.emplace_back("display"); return 0x96u;
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("effect manager"); return 0x4000u;
    }
    pal4::ScriptEffectHandle LoadFromXml(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t type) override {
        Check(manager == 0x4000u &&
                  name == (effect_name == 0u ? 0x9000u : effect_name) &&
                  type == 0x123u, "XML effect operands");
        calls.emplace_back("load"); return {0x11u, 0x22u};
    }
    pal4::ScriptEffectHandle CreateAtAddress(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t argument,
        std::uint32_t flag) override {
        Check(manager == 0x4000u &&
                  name == (effect_name == 0u ? 0x9000u : effect_name) &&
                  position == object + 0xacu && argument == 0xa000u &&
                  flag == 0u, "fallback effect uses object position");
        calls.emplace_back("create"); return {0x33u, 0x44u};
    }
    void WriteHandle(std::uint32_t address,
                     const pal4::ScriptEffectHandle& handle) override {
        Check((address == object + 0x1d4u && handle.first == 0x11u &&
               handle.second == 0x22u) ||
                  (address == object + 0x1ccu && handle.first == 0x33u &&
                   handle.second == 0x44u),
              "two-word effect handles written to distinct slots");
        calls.emplace_back(address == object + 0x1d4u ?
                           "write loaded" : "write active");
    }
    std::uint8_t ResetHandle(pal4::ScriptEffectHandle& handle) override {
        Check(handle.first == 0x33u && handle.second == 0x44u,
              "fallback temporary handle reset");
        calls.emplace_back("reset"); return 0x95u;
    }
};

void ObjectMissingAndLoadedReady() {
    Runtime missing;
    missing.object_name = 0u;
    missing.object = 0u;
    Check(pal4::ScriptObjectEffect::Play(
                  0x1000u, 0xa000u, 0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "object name", "empty", "object manager", "find"},
          "missing object skips all effect work");
    Runtime ready;
    ready.needs_update = 1u;
    ready.initial_ready = 1u;
    ready.loaded_ready = 2u;
    Check(pal4::ScriptObjectEffect::Play(
                  0x1000u, 0xa000u, 0x2000u, ready) == 2u &&
              ready.calls == std::vector<std::string>{
                  "object name", "object manager", "find", "needs update",
                  "update", "ready", "display", "effect name", "type",
                  "effect manager", "load", "write loaded", "ready"},
          "ready XML effect returns byte without fallback creation");
}

void FallbackCreation() {
    Runtime runtime;
    runtime.effect_name = 0u;
    Check(pal4::ScriptObjectEffect::Play(
                  0x1000u, 0xa000u, 0x2000u, runtime) == 0x95u &&
              runtime.calls == std::vector<std::string>{
                  "object name", "object manager", "find", "needs update",
                  "ready", "effect name", "empty", "type",
                  "effect manager", "load", "write loaded", "ready",
                  "effect name", "empty", "effect manager", "create",
                  "write active", "reset"},
          "unready XML effect creates fallback and resets temporary handle");
}

void UpdateAndShow() {
    Runtime missing;
    missing.object = 0u;
    Check(pal4::ScriptObjectEffect::UpdateAndShow(0x2000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "object name", "object manager", "find"},
          "missing object effect stops before handle access");
    Runtime unready;
    unready.needs_update = 1u;
    Check(pal4::ScriptObjectEffect::UpdateAndShow(0x2000u, unready) == 0u &&
              unready.calls == std::vector<std::string>{
                  "object name", "object manager", "find", "needs update",
                  "update", "ready"},
          "unready effect updates active slot but skips display");
    Runtime ready;
    ready.initial_ready = 2u;
    Check(pal4::ScriptObjectEffect::UpdateAndShow(0x2000u, ready) == 0x96u &&
              ready.calls == std::vector<std::string>{
                  "object name", "object manager", "find", "needs update",
                  "ready", "display"},
          "ready effect displays and forwards display result");
}

}  // namespace

int main() {
    try {
        ObjectMissingAndLoadedReady();
        FallbackCreation();
        UpdateAndShow();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
