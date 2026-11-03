#include "pal4/script_player_gob_attachment.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPlayerGobAttachmentRuntime {
    std::uint32_t manager = 0x1000u;
    std::uint32_t player = 0x5000u;
    std::uint32_t current_index = 1u;
    std::uint32_t gob_name = 0x6000u;
    std::uint32_t render_name = 0x7000u;
    std::uint32_t render_length = 3u;
    std::uint32_t object = 0x8000u;
    bool different = true;
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("player manager");
        return manager;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::int32_t index) override {
        Check(manager == 0x1000u && index == -2, "player lookup operands");
        calls.emplace_back("player");
        return player;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == manager + 0x2c50u) {
            calls.emplace_back("current index");
            return current_index;
        }
        if (address == 0x2008u) {
            calls.emplace_back("gob length");
            return 4u;
        }
        if (address == 0x3008u) {
            calls.emplace_back("render length");
            return render_length;
        }
        if (address == 0x2004u) {
            calls.emplace_back("gob name");
            return gob_name;
        }
        if (address == 0x3004u) {
            calls.emplace_back("render name");
            return render_name;
        }
        Check(address == object + 0x14u, "object type offset");
        calls.emplace_back("object type");
        return 0x55u;
    }
    bool NameDiffersFromEmptyError(std::uint32_t string_object,
                                   std::uint32_t length) override {
        Check(string_object == 0x2000u && length == 4u,
              "string comparison arguments");
        calls.emplace_back("compare");
        return different;
    }
    std::uint32_t EmptyStringAddress() override {
        calls.emplace_back("empty");
        return 0x4000u;
    }
    std::uint32_t ObjectManager() override {
        calls.emplace_back("object manager");
        return 0x1100u;
    }
    std::uint32_t FindObject(std::uint32_t manager,
                             std::uint32_t name) override {
        Check(manager == 0x1100u &&
                  name == (gob_name == 0u ? 0x4000u : gob_name),
              "object lookup operands");
        calls.emplace_back("find object");
        return object;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena");
        return 0x1200u;
    }
    void RemoveAnimatedObject(std::uint32_t arena,
                              std::uint32_t found_object) override {
        Check(arena == 0x1200u && found_object == object,
              "object removal operands");
        calls.emplace_back("remove");
    }
    std::uint32_t RenderUnit(std::uint32_t actor,
                             std::uint32_t type,
                             const std::array<float, 3>& scale,
                             std::uint32_t name,
                             bool left_hand) override {
        Check(actor == player && type == 0u &&
                  scale == std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
                  name == (render_name == 0u ? 0x4000u : render_name),
              "unit render operands");
        calls.emplace_back(left_hand ? "unit left" : "unit normal");
        return 0x81u;
    }
    std::uint32_t RenderObject(std::uint32_t actor,
                               std::uint32_t type,
                               std::uint32_t transform,
                               std::uint32_t name,
                               bool left_hand) override {
        Check(actor == player && type == 0x55u &&
                  transform == object + 0xb8u &&
                  name == (render_name == 0u ? 0x4000u : render_name),
              "object render operands");
        calls.emplace_back(left_hand ? "object left" : "object normal");
        return 0x82u;
    }
    std::uint32_t CleanupWeapons(std::uint32_t actor) override {
        Check(actor == player, "weapon cleanup player");
        calls.emplace_back("cleanup weapons");
        return 0x83u;
    }
    std::uint32_t SetupLeftHandEffect(std::uint32_t actor,
                                      std::uint32_t name) override {
        Check(actor == player &&
                  name == (render_name == 0u ? 0x4000u : render_name),
              "left effect operands");
        calls.emplace_back("left effect");
        return 0x84u;
    }
    std::uint32_t SetupRightHandEffect(std::uint32_t actor,
                                       std::uint32_t name) override {
        Check(actor == player &&
                  name == (render_name == 0u ? 0x4000u : render_name),
              "right effect operands");
        calls.emplace_back("right effect");
        return 0x85u;
    }
    std::uint32_t UpdateWeaponEffects(std::uint32_t actor) override {
        Check(actor == player, "weapon effect update player");
        calls.emplace_back("update effects");
        return 0x86u;
    }
};

std::uint32_t Attach(std::uint8_t left, Runtime& runtime) {
    return pal4::ScriptPlayerGobAttachment::Attach(
        0x2000u, 0x3000u, -2, left, runtime);
}

void EarlyReturns() {
    Runtime absent;
    absent.player = 0u;
    Check(Attach(0u, absent) == 0u &&
              absent.calls == std::vector<std::string>{
                  "player manager", "player"},
          "missing player exits before string comparison");
    Runtime empty;
    empty.different = false;
    empty.render_length = 0u;
    Check(Attach(1u, empty) == 0x3000u &&
              empty.calls == std::vector<std::string>{
                  "player manager", "player", "gob length", "compare",
                  "render length"},
          "empty GOB and render name return render string object");
    Runtime missing;
    missing.object = 0u;
    Check(Attach(0u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "player", "gob length", "compare",
                  "gob name", "object manager", "find object"},
          "missing GOB exits before arena work");
}

void RenderBranches() {
    for (auto left : {0u, 1u, 255u}) {
        Runtime unit;
        unit.different = false;
        unit.render_name = 0u;
        Check(Attach(static_cast<std::uint8_t>(left), unit) == 0x81u &&
                  unit.calls == std::vector<std::string>{
                      "player manager", "player", "gob length", "compare",
                      "render length", "render name", "empty",
                      left == 0u ? "unit normal" : "unit left"},
              "unit render uses left-hand flag and empty fallback");
        Runtime object;
        object.gob_name = 0u;
        object.render_name = 0u;
        Check(Attach(static_cast<std::uint8_t>(left), object) == 0x82u &&
                  object.calls == std::vector<std::string>{
                      "player manager", "player", "gob length", "compare",
                      "gob name", "empty", "object manager", "find object",
                      "arena", "remove", "render name", "empty",
                      "object type", left == 0u ? "object normal" :
                                                     "object left"},
              "object render follows removal and uses object transform");
    }
}

void CurrentPlayerBranches() {
    Runtime current;
    current.player = current.manager + 0xb14u;
    current.different = false;
    Check(pal4::ScriptPlayerGobAttachment::AttachToCurrent(
              0x2000u, 0x3000u, 1u, current) == 0x81u &&
              current.calls == std::vector<std::string>{
                  "player manager", "current index", "gob length",
                  "compare", "render length", "render name", "unit left"},
          "current player uses inline stride without indexed lookup");
    Runtime missing;
    missing.manager = 0u;
    missing.current_index = 0u;
    Check(pal4::ScriptPlayerGobAttachment::AttachToCurrent(
              0x2000u, 0x3000u, 0u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "current index"},
          "zero computed current player returns original manager");
    Runtime object;
    object.player = object.manager + 2u * 0xb14u;
    object.current_index = 2u;
    Check(pal4::ScriptPlayerGobAttachment::AttachToCurrent(
              0x2000u, 0x3000u, 0u, object) == 0x82u &&
              object.calls.front() == "player manager" &&
              object.calls[1] == "current index" &&
              object.calls.back() == "object normal",
          "current player object branch reuses attachment flow");
}

void DetachBranches() {
    Runtime present;
    Check(pal4::ScriptPlayerGobAttachment::Detach(-2, present) == 0x83u &&
              present.calls == std::vector<std::string>{
                  "player manager", "player", "cleanup weapons"},
          "detach forwards cleanup result for found player");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerGobAttachment::Detach(-2, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "player"},
          "detach skips cleanup for missing player");
    Runtime current;
    current.player = current.manager + current.current_index * 0xb14u;
    Check(pal4::ScriptPlayerGobAttachment::DetachCurrent(current) == 0x83u &&
              current.calls == std::vector<std::string>{
                  "player manager", "current index", "cleanup weapons"},
          "current detach uses inline player stride");
    Runtime absent_current;
    absent_current.manager = 0u;
    absent_current.current_index = 0u;
    Check(pal4::ScriptPlayerGobAttachment::DetachCurrent(absent_current) == 0u &&
              absent_current.calls == std::vector<std::string>{
                  "player manager", "current index"},
          "zero computed current player returns manager");
}

void EffectBranches() {
    for (auto left : {0u, 1u, 255u}) {
        Runtime indexed;
        indexed.render_name = 0u;
        Check(pal4::ScriptPlayerGobAttachment::AttachEffect(
                  -2, 0x3000u, static_cast<std::uint8_t>(left), indexed) ==
                  (left == 0u ? 0x85u : 0x84u) &&
                  indexed.calls == std::vector<std::string>{
                      "player manager", "player", "render name", "empty",
                      left == 0u ? "right effect" : "left effect"},
              "indexed hand effect selects renderer and empty fallback");
        Runtime current;
        current.player = current.manager + current.current_index * 0xb14u;
        Check(pal4::ScriptPlayerGobAttachment::AttachEffectToCurrent(
                  0x3000u, static_cast<std::uint8_t>(left), current) ==
                  (left == 0u ? 0x85u : 0x84u) &&
                  current.calls == std::vector<std::string>{
                      "player manager", "current index", "render name",
                      left == 0u ? "right effect" : "left effect"},
              "current hand effect uses inline player and original name");
    }
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerGobAttachment::AttachEffect(
              -2, 0x3000u, 1u, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "player"},
          "missing indexed player skips effect name");
    Runtime missing_current;
    missing_current.manager = 0u;
    missing_current.current_index = 0u;
    Check(pal4::ScriptPlayerGobAttachment::AttachEffectToCurrent(
              0x3000u, 0u, missing_current) == 0u &&
              missing_current.calls == std::vector<std::string>{
                  "player manager", "current index"},
          "zero current player skips effect name");
}

void DetachEffectBranches() {
    Runtime indexed;
    Check(pal4::ScriptPlayerGobAttachment::DetachEffect(-2, indexed) ==
                  0x86u && indexed.calls == std::vector<std::string>{
                      "player manager", "player", "update effects"},
          "indexed effect detach forwards update result");
    Runtime missing;
    missing.player = 0u;
    Check(pal4::ScriptPlayerGobAttachment::DetachEffect(-2, missing) == 0u &&
              missing.calls == std::vector<std::string>{
                  "player manager", "player"},
          "missing indexed player skips effect update");
    Runtime current;
    current.player = current.manager + current.current_index * 0xb14u;
    Check(pal4::ScriptPlayerGobAttachment::DetachEffectFromCurrent(current) ==
                  0x86u && current.calls == std::vector<std::string>{
                      "player manager", "current index", "update effects"},
          "current effect detach uses player stride");
    Runtime zero;
    zero.manager = 0u;
    zero.current_index = 0u;
    Check(pal4::ScriptPlayerGobAttachment::DetachEffectFromCurrent(zero) ==
                  0u && zero.calls == std::vector<std::string>{
                      "player manager", "current index"},
          "zero current player skips effect update");
}

}  // namespace

int main() {
    try {
        EarlyReturns();
        RenderBranches();
        CurrentPlayerBranches();
        DetachBranches();
        EffectBranches();
        DetachEffectBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
