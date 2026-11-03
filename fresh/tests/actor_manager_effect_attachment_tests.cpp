#include "pal4/actor_manager_effect_attachment.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IActorManagerEffectAttachmentRuntime {
    std::uint32_t clump = 0x4000u;
    bool valid = true;
    std::vector<std::string> trace;
    pal4::ActorEffectHandle original{10u, 20u};
    std::uint32_t copy_count = 0u;

    std::uint32_t ChildClump(std::uint32_t actor) override {
        Check(actor == 0x1000u, "actor clump argument");
        trace.emplace_back("clump");
        return clump;
    }
    std::uint32_t CombatEffectGroup() override {
        trace.emplace_back("camera");
        return 0x5000u;
    }
    pal4::ActorEffectHandle CreateEffect(
        std::uint32_t camera, std::string_view resource,
        const std::array<float, 3>& initial_position) override {
        Check(camera == 0x5000u && resource == "EFF" &&
              initial_position == std::array<float, 3>{0.0f, 1.0f, 1.0f},
              "effect creation arguments");
        trace.emplace_back("create");
        return original;
    }
    bool IsValid(pal4::ActorEffectHandle effect) override {
        Check(effect.id == original.id, "effect validity argument");
        trace.emplace_back("valid");
        return valid;
    }
    bool MatchesPrimaryMode(std::string_view mode) override {
        trace.emplace_back("primary?");
        return mode == "primary";
    }
    bool MatchesSecondaryMode(std::string_view mode) override {
        trace.emplace_back("secondary?");
        return mode == "secondary";
    }
    std::array<float, 3> ActorSlot448Position(
        std::uint32_t actor) override {
        Check(actor == 0x1000u, "primary position actor");
        trace.emplace_back("slot448");
        return {4.0f, 5.0f, 6.0f};
    }
    std::array<float, 3> ChildPosition(std::uint32_t actor) override {
        Check(actor == 0x1000u, "child position actor");
        trace.emplace_back("position");
        return {7.0f, 8.0f, 9.0f};
    }
    std::uint32_t ChildRotationVector(std::uint32_t actor) override {
        Check(actor == 0x1000u, "child matrix actor");
        trace.emplace_back("matrix");
        return 0x6000u;
    }
    void SetVectorProperty(pal4::ActorEffectHandle effect,
                           const std::array<float, 3>& value,
                           bool enabled, std::uint32_t slot) override {
        Check(effect.id == original.id, "vector effect");
        if (slot == 23u) {
            Check(!enabled &&
                  (value == std::array<float, 3>{4.0f, 5.0f, 6.0f} ||
                   value == std::array<float, 3>{7.0f, 2.0f, 9.0f}),
                  "position vector and flag");
            trace.emplace_back("vector23");
        } else {
            Check(slot == 21u && enabled &&
                  value == std::array<float, 3>{90.0f, 0.0f, 0.0f},
                  "default orientation vector and flag");
            trace.emplace_back("vector21");
        }
    }
    void SetRotationVectorProperty(pal4::ActorEffectHandle effect,
                           std::uint32_t matrix, bool enabled) override {
        Check(effect.id == original.id && matrix == 0x6000u && !enabled,
              "secondary matrix property");
        trace.emplace_back("matrix21");
    }
    void SetFacetProperty(pal4::ActorEffectHandle effect,
                         std::string_view text,
                         std::uint32_t actor_field) override {
        Check(effect.id == original.id && text == "T1" &&
              actor_field == 0x12d4u, "effect text and actor field");
        trace.emplace_back("text");
    }
    std::uint32_t EffectFrame(pal4::ActorEffectHandle effect) override {
        Check(effect.id == original.id, "effect user data argument");
        trace.emplace_back("user-data");
        return 0x7000u;
    }
    void ProcessClumpUserData(std::uint32_t subject,
                              std::string_view mode,
                              std::uint32_t data) override {
        Check(subject == clump && mode == "other" && data == 0x7000u,
              "clump user data arguments");
        trace.emplace_back("clump-data");
    }
    void EnableTasks(pal4::ActorEffectHandle effect) override {
        Check(effect.id == original.id, "effect update argument");
        trace.emplace_back("update");
    }
    pal4::ActorEffectHandle CopyEffect(
        pal4::ActorEffectHandle effect) override {
        Check(effect.id == original.id, "effect copy argument");
        trace.emplace_back("copy");
        ++copy_count;
        return {effect.id, effect.owner + copy_count};
    }
    void DispatchAttachEffect(std::uint32_t actor,
                              pal4::ActorEffectHandle effect) override {
        Check(actor == 0x1000u && effect.id == original.id &&
              effect.owner == original.owner + 1u,
              "ae_attach_effect payload");
        trace.emplace_back("event");
    }
    void ResetTemporary(pal4::ActorEffectHandle effect) override {
        Check(effect.id == original.id &&
              (effect.owner == original.owner ||
               effect.owner == original.owner + 1u),
              "temporary effect reset");
        trace.emplace_back("reset");
    }
    void ResetHandleAt(std::uint32_t address) override {
        Check(address == 0x8004u, "embedded handle offset");
        trace.emplace_back("reset-embedded");
    }
};

void MissingClumpAndInvalidEffect() {
    Runtime runtime;
    runtime.clump = 0u;
    const auto empty = pal4::ActorManagerEffectAttachment::Create(
        0x1000u, "EFF", "other", runtime);
    Check(empty.id == 0xffffffffu && empty.owner == 0u &&
          runtime.trace == std::vector<std::string>{"clump"},
          "missing clump returns empty effect");

    runtime.clump = 0x4000u;
    runtime.valid = false;
    runtime.trace.clear();
    const auto invalid = pal4::ActorManagerEffectAttachment::Create(
        0x1000u, "EFF", "other", runtime);
    Check(invalid.id == 10u && invalid.owner == 21u &&
          runtime.trace == std::vector<std::string>{
              "clump", "camera", "create", "valid", "copy", "reset"},
          "invalid effect is copied without update or event");
}

void PrimaryMode() {
    Runtime runtime;
    const auto result = pal4::ActorManagerEffectAttachment::Create(
        0x1000u, "EFF", "primary", runtime);
    Check(result.id == 10u && result.owner == 22u &&
          runtime.trace == std::vector<std::string>{
              "clump", "camera", "create", "valid", "primary?",
              "slot448", "vector23", "update", "copy", "event",
              "reset", "copy", "reset"},
          "primary mode attaches actor slot 448 position and emits event");
}

void SecondaryMode() {
    Runtime runtime;
    (void)pal4::ActorManagerEffectAttachment::Create(
        0x1000u, "EFF", "secondary", runtime);
    Check(runtime.trace == std::vector<std::string>{
              "clump", "camera", "create", "valid", "primary?",
              "secondary?", "position", "text", "vector23", "matrix",
              "matrix21", "update", "copy", "event", "reset",
              "copy", "reset"},
          "secondary mode forces Y=2 and applies actor child matrix");
}

void DefaultMode() {
    Runtime runtime;
    (void)pal4::ActorManagerEffectAttachment::Create(
        0x1000u, "EFF", "other", runtime);
    Check(runtime.trace == std::vector<std::string>{
              "clump", "camera", "create", "valid", "primary?",
              "secondary?", "vector21", "text", "user-data",
              "clump-data", "update", "copy", "event", "reset",
              "copy", "reset"},
          "default mode configures 90-degree vector and clump data");
}

void ResetEmbedded() {
    Runtime runtime;
    pal4::ActorManagerEffectAttachment::ResetEmbedded(0x8000u, runtime);
    Check(runtime.trace == std::vector<std::string>{"reset-embedded"},
          "embedded effect reset forwards object plus four");
}

}  // namespace

int main() {
    try {
        MissingClumpAndInvalidEffect();
        PrimaryMode();
        SecondaryMode();
        DefaultMode();
        ResetEmbedded();
        std::cout << "actor_manager_effect_attachment_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
