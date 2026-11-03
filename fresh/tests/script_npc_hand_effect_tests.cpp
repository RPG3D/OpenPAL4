#include "pal4/script_npc_hand_effect.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptNpcHandEffectRuntime {
    std::uint32_t npc_name = 0x4000u;
    std::uint32_t effect_name = 0x5000u;
    std::uint32_t npc = 0x6000u;
    std::uint32_t gob_name = 0x8000u;
    std::uint32_t render_name = 0x8100u;
    std::uint32_t render_length = 3u;
    std::uint32_t object = 0x9000u;
    bool different = true;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x1004u) {
            calls.emplace_back("npc name");
            return npc_name;
        }
        if (address == 0x2004u) {
            calls.emplace_back("effect name");
            return effect_name;
        }
        if (address == 0x3008u) {
            calls.emplace_back("gob length");
            return 4u;
        }
        if (address == 0x4008u) {
            calls.emplace_back("render length");
            return render_length;
        }
        if (address == 0x3004u) {
            calls.emplace_back("gob name");
            return gob_name;
        }
        if (address == 0x4004u) {
            calls.emplace_back("render name");
            return render_name;
        }
        Check(address == object + 0x14u, "GOB type offset");
        calls.emplace_back("object type");
        return 0x55u;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x3000u;
    }
    std::uint32_t NpcManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                           std::uint32_t name) override {
        Check(manager == 0x7000u &&
                  name == (npc_name == 0u ? 0x3000u : npc_name),
              "NPC lookup operands");
        calls.emplace_back("find");
        return npc;
    }
    std::uint32_t SetupLeftHandEffect(std::uint32_t actor,
                                      std::uint32_t name) override {
        Check(actor == npc &&
                  name == (effect_name == 0u ? 0x3000u : effect_name),
              "left NPC effect operands");
        calls.emplace_back("left");
        return 0x81u;
    }
    std::uint32_t SetupRightHandEffect(std::uint32_t actor,
                                       std::uint32_t name) override {
        Check(actor == npc &&
                  name == (effect_name == 0u ? 0x3000u : effect_name),
              "right NPC effect operands");
        calls.emplace_back("right");
        return 0x82u;
    }
    std::uint32_t UpdateWeaponEffects(std::uint32_t actor) override {
        Check(actor == npc, "NPC effect cleanup operand");
        calls.emplace_back("update");
        return 0x83u;
    }
    bool NameDiffersFromEmptyError(std::uint32_t object_address,
                                   std::uint32_t length) override {
        Check(object_address == 0x3000u && length == 4u,
              "GOB name comparison operands");
        calls.emplace_back("compare");
        return different;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object manager");
        return 0x7100u;
    }
    std::uint32_t FindObject(std::uint32_t manager,
                             std::uint32_t name) override {
        Check(manager == 0x7100u &&
                  name == (gob_name == 0u ? 0x3000u : gob_name),
              "GOB lookup operands");
        calls.emplace_back("find object");
        return object;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x7200u;
    }
    void RemoveAnimatedObject(std::uint32_t arena,
                              std::uint32_t found_object) override {
        Check(arena == 0x7200u && found_object == object,
              "GOB arena removal operands");
        calls.emplace_back("remove");
    }
    std::uint32_t RenderUnit(std::uint32_t actor, std::uint32_t type,
                             const std::array<float, 3>& scale,
                             std::uint32_t name, bool left_hand) override {
        Check(actor == npc && type == 0u &&
                  scale == std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
                  name == (render_name == 0u ? 0x3000u : render_name),
              "NPC unit render operands");
        calls.emplace_back(left_hand ? "unit left" : "unit right");
        return 0x84u;
    }
    std::uint32_t RenderObject(std::uint32_t actor, std::uint32_t type,
                               std::uint32_t transform, std::uint32_t name,
                               bool left_hand) override {
        Check(actor == npc && type == 0x55u &&
                  transform == object + 0xb8u &&
                  name == (render_name == 0u ? 0x3000u : render_name),
              "NPC object render operands");
        calls.emplace_back(left_hand ? "object left" : "object right");
        return 0x85u;
    }
    std::uint32_t CleanupWeapons(std::uint32_t actor) override {
        Check(actor == npc, "NPC cleanup operand");
        calls.emplace_back("cleanup");
        return 0x86u;
    }
};

void AttachBranches() {
    for (auto hand : {0u, 1u, 255u}) {
        Runtime runtime;
        runtime.npc_name = 0u;
        runtime.effect_name = 0u;
        Check(pal4::ScriptNpcHandEffect::Attach(
                  0x1000u, 0x2000u, static_cast<std::uint8_t>(hand),
                  runtime) == (hand == 0u ? 0x82u : 0x81u) &&
                  runtime.calls == std::vector<std::string>{
                      "npc name", "empty", "manager", "find",
                      "effect name", "empty", hand == 0u ? "right" : "left"},
              "NPC hand selection and both empty-name fallbacks");
    }
    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptNpcHandEffect::Attach(
              0x1000u, 0x2000u, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips effect name and setup");
}

void DetachBranches() {
    Runtime found;
    Check(pal4::ScriptNpcHandEffect::Detach(0x1000u, found) == 0x83u &&
              found.calls == std::vector<std::string>{
                  "npc name", "manager", "find", "update"},
          "NPC detach forwards weapon effect update");
    Runtime missing;
    missing.npc = 0u;
    Check(pal4::ScriptNpcHandEffect::Detach(0x1000u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips weapon effect update");
}

std::uint32_t AttachGob(std::uint8_t hand, Runtime& runtime) {
    return pal4::ScriptNpcHandEffect::AttachGob(
        0x3000u, 0x4000u, 0x1000u, hand, runtime);
}

void GobBranches() {
    Runtime missing_npc;
    missing_npc.npc = 0u;
    Check(AttachGob(0u, missing_npc) == 0u &&
              missing_npc.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips GOB comparison");
    Runtime empty;
    empty.different = false;
    empty.render_length = 0u;
    Check(AttachGob(0u, empty) == 0x4000u &&
              empty.calls == std::vector<std::string>{
                  "npc name", "manager", "find", "gob length", "compare",
                  "render length"},
          "empty GOB and render name return render string object");
    Runtime missing_object;
    missing_object.object = 0u;
    Check(AttachGob(1u, missing_object) == 0u &&
              missing_object.calls == std::vector<std::string>{
                  "npc name", "manager", "find", "gob length", "compare",
                  "gob name", "object manager", "find object"},
          "missing GOB skips arena and render");
    for (auto hand : {0u, 1u, 255u}) {
        Runtime unit;
        unit.different = false;
        unit.render_name = 0u;
        Check(AttachGob(static_cast<std::uint8_t>(hand), unit) == 0x84u &&
                  unit.calls == std::vector<std::string>{
                      "npc name", "manager", "find", "gob length",
                      "compare", "render length", "render name", "empty",
                      hand == 0u ? "unit right" : "unit left"},
              "NPC unit render selects hand and fallback name");
        Runtime object;
        object.gob_name = 0u;
        Check(AttachGob(static_cast<std::uint8_t>(hand), object) == 0x85u &&
                  object.calls == std::vector<std::string>{
                      "npc name", "manager", "find", "gob length",
                      "compare", "gob name", "empty", "object manager",
                      "find object", "arena", "remove", "render name",
                      "object type", hand == 0u ? "object right" :
                                                 "object left"},
              "NPC object render removes GOB and uses transform");
    }
    Runtime detach;
    Check(pal4::ScriptNpcHandEffect::DetachGob(0x1000u, detach) == 0x86u &&
              detach.calls == std::vector<std::string>{
                  "npc name", "manager", "find", "cleanup"},
          "NPC GOB detach forwards cleanup result");
    Runtime missing_detach;
    missing_detach.npc = 0u;
    Check(pal4::ScriptNpcHandEffect::DetachGob(
                  0x1000u, missing_detach) == 0u &&
              missing_detach.calls == std::vector<std::string>{
                  "npc name", "manager", "find"},
          "missing NPC skips GOB cleanup");
}

}  // namespace

int main() {
    try {
        AttachBranches();
        DetachBranches();
        GobBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
