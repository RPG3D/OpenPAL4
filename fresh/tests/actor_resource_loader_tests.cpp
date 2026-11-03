#include "pal4/actor_resource_loader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr auto actor = 0x1000u;

struct Runtime final : pal4::IActorResourceRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> texts;
    std::map<std::uint32_t, std::string> stored;
    std::vector<std::string> trace;
    bool inactive = false;
    bool load_creates_clump = true;
    bool dialog_allocation_succeeds = true;
    std::uint32_t cleanup_result = 0x1234u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
        trace.push_back("flag");
    }
    std::string_view TextAt(const std::uint32_t address) override {
        return texts[address];
    }
    bool EqualIgnoringCase(const std::string_view left,
                           const std::string_view right) override {
        trace.push_back("compare");
        return left.size() == right.size() &&
               std::equal(left.begin(), left.end(), right.begin(),
                   [](const char a, const char b) {
                       return std::tolower(static_cast<unsigned char>(a)) ==
                              std::tolower(static_cast<unsigned char>(b));
                   });
    }
    bool CombatInactive() override {
        trace.push_back("combat-gate");
        return inactive;
    }
    void ResetActorProperties(const std::uint32_t address) override {
        Check(address == actor, "reset actor address");
        trace.push_back("reset");
    }
    void StartResourceContext(const bool alternate) override {
        trace.push_back(alternate ? "context-alternate" : "context-default");
    }
    void LoadComponentClump(const std::uint32_t address,
                            const bool alternate,
                            const std::string_view name) override {
        Check(address == actor && name == "NPC_A" && alternate,
              "alternate clump load arguments");
        trace.push_back("load-clump");
        if (load_creates_clump) words[actor + 20u] = 0x5000u;
    }
    void SetTexture(const std::uint32_t address,
                    const std::string_view name) override {
        Check(address == actor && name == "skin.dds",
              "texture arguments");
        trace.push_back("texture");
    }
    void EndResourceContext() override {
        trace.push_back("context-end");
    }
    void UpdateAnimation(const std::uint32_t context,
                         const std::string_view name,
                         const std::uint32_t id) override {
        Check(context == 0x7000u && name == "idle" && id == 4u,
              "animation arguments");
        trace.push_back("animation");
    }
    bool StoreDialogText(const std::uint32_t field,
                         const std::string_view value) override {
        Check(field == actor + 428u, "dialog field offset");
        trace.push_back("dialog");
        if (dialog_allocation_succeeds)
            stored[field] = std::string(value);
        return dialog_allocation_succeeds;
    }
    void StoreText(const std::uint32_t field,
                   const std::string_view value) override {
        trace.push_back(field == actor + 412u ? "primary" : "secondary");
        stored[field] = std::string(value);
    }
    void UpdateRenderState(const std::uint32_t address) override {
        Check(address == actor, "render actor address");
        trace.push_back("render");
    }
    std::uint32_t CleanupComponent(const std::uint32_t address) override {
        Check(address == actor, "cleanup actor address");
        trace.push_back("cleanup-clump");
        return cleanup_result;
    }
};

pal4::ActorLoadRequest Request(const bool alternate) {
    return {0x7000u, "idle", "NPC_A", "skin.dds", 4u,
            "first", "dialog", "third", alternate};
}

void InactiveGate() {
    Runtime runtime;
    runtime.inactive = true;
    Check(pal4::ActorResourceLoader::Load(actor, Request(false), runtime) ==
              1u &&
          runtime.trace == std::vector<std::string>{"combat-gate"} &&
          runtime.bytes.empty(),
          "inactive combat path has no actor side effects");
}

void ExistingClump() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[actor + 4u] = 0x6000u;
    runtime.texts[0x6000u] = "npc_a";
    Check(pal4::ActorResourceLoader::Load(actor, Request(false), runtime) ==
              0u &&
          runtime.trace == std::vector<std::string>{
              "combat-gate", "flag", "compare", "context-default",
              "texture", "context-end", "dialog", "primary",
              "secondary", "render"} &&
          runtime.bytes[actor + 460u] == 1u &&
          runtime.stored[actor + 428u] == "dialog" &&
          runtime.stored[actor + 412u] == "first" &&
          runtime.stored[actor + 444u] == "third",
          "matching clump changes texture and text without reloading");
}

void ReloadAndFailedDialogAllocation() {
    Runtime runtime;
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[actor + 4u] = 0x6000u;
    runtime.texts[0x6000u] = "other";
    runtime.dialog_allocation_succeeds = false;
    Check(pal4::ActorResourceLoader::Load(actor, Request(true), runtime) ==
              0u &&
          runtime.trace == std::vector<std::string>{
              "combat-gate", "flag", "compare", "reset",
              "context-alternate", "load-clump", "texture",
              "context-end", "animation", "dialog", "primary",
              "secondary", "render"} &&
          runtime.stored.count(actor + 428u) == 0u &&
          runtime.stored[actor + 412u] == "first" &&
          runtime.stored[actor + 444u] == "third",
          "alternate reload preserves later text and render work after dialog failure");
}

void MissingClumpCleanup() {
    Runtime runtime;
    runtime.load_creates_clump = false;
    Check(pal4::ActorResourceLoader::Load(actor, Request(true), runtime) ==
              0x34u &&
          runtime.trace == std::vector<std::string>{
              "combat-gate", "flag", "reset", "context-alternate",
              "load-clump", "texture", "context-end", "animation",
              "cleanup-clump"} &&
          runtime.stored.empty(),
          "failed clump load skips text/render and forwards cleanup low byte");
}

struct ResetRuntime final : pal4::IActorPropertyResetRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> trace;
    std::vector<std::uint32_t> cleared_fields;
    std::uint32_t failed_field = 0;
    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    bool StoreEmptyText(const std::uint32_t field) override {
        trace.push_back("text");
        if (field != failed_field) cleared_fields.push_back(field);
        return field != failed_field;
    }
    void RemoveEffect(const std::uint32_t handle) override {
        trace.push_back("remove:" + std::to_string(handle));
    }
    void HandleEffectEvent(const std::uint32_t handle,
                           const std::uint32_t context) override {
        Check(handle == actor + 404u && context == actor + 444u,
              "reset effect event arguments");
        trace.push_back("event");
    }
    void RemoveFrameChild(const std::uint32_t frame) override {
        Check(frame == 0x6000u, "reset clump frame");
        trace.push_back("remove-frame");
    }
    std::uint8_t CleanupComponent(const std::uint32_t address) override {
        Check(address == actor, "reset component address");
        trace.push_back("cleanup");
        return 0x5au;
    }
};

void PropertyResetBranches() {
    ResetRuntime runtime;
    runtime.failed_field = actor + 412u;
    Check(pal4::ActorPropertyReset::Reset(actor, runtime) == 0x5au &&
          runtime.cleared_fields ==
              std::vector<std::uint32_t>{actor + 428u, actor + 444u} &&
          runtime.trace == std::vector<std::string>{
              "text", "text", "text", "remove:4492", "remove:4484",
              "event", "cleanup"},
          "all text fields are attempted before effect cleanup after one failure");
    runtime.trace.clear();
    runtime.words[actor + 20u] = 0x5000u;
    runtime.words[0x5000u + 4u] = 0x6000u;
    Check(pal4::ActorPropertyReset::Reset(actor, runtime) == 0x5au &&
          runtime.trace == std::vector<std::string>{
              "text", "text", "text", "remove:4492", "remove:4484",
              "event", "remove-frame", "cleanup"},
          "clump frame is detached before component cleanup");
    runtime.trace.clear();
    runtime.words[0x5000u + 4u] = 0;
    (void)pal4::ActorPropertyReset::Reset(actor, runtime);
    Check(std::find(runtime.trace.begin(), runtime.trace.end(),
                    "remove-frame") == runtime.trace.end(),
          "null frame skips RenderWare detach");
}

}  // namespace

int main() {
    try {
        InactiveGate();
        ExistingClump();
        ReloadAndFailedDialogAllocation();
        MissingClumpCleanup();
        PropertyResetBranches();
        std::cout << "actor_resource_loader_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
