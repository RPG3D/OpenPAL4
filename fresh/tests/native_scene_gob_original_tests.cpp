#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_effect_script_asset.h"
#include "pal4/native_actor_animation_catalog.h"
#include "pal4/native_package_archive.h"

#include <array>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <variant>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main(int argc, char** argv) {
    Check(argc == 2, "original game root argument");
    pal4::NativeSceneGobCatalog catalog;
    if (!catalog.Load(std::filesystem::path(argv[1]), "Q01", "N01"))
        std::cerr << "Scene GOB: " << catalog.LastError() << '\n';
    Check(catalog.Records().size() == 4u,
          "Q01/N01 has four parsed scene objects");
    constexpr std::array<std::uint32_t, 4> types{0u, 0u, 0u, 8u};
    constexpr std::array<float, 4> scales{0.9f, 0.6f, 0.6f, 1.0f};
    constexpr std::array<std::uint32_t, 4> auto_disappear{1u, 0u, 0u, 1u};
    constexpr std::array<std::uint32_t, 4> store{0u, 0u, 0u, 1u};
    pal4::NativePackageArchive objects;
    Check(objects.Open((std::filesystem::path(argv[1]) / "gamedata" /
                        "palobject.cpk").string().c_str()),
          "open original object package");
    pal4::NativePackageArchive effects;
    Check(effects.Open((std::filesystem::path(argv[1]) / "gamedata" /
                        "Effect.cpk").string().c_str()),
          "open original effect package");
    for (std::size_t index = 0u; index < types.size(); ++index) {
        const auto& record = catalog.Records()[index];
        Check(record.type == types[index] && !record.name.empty() &&
                  !record.model.empty() &&
                  record.resource_dir ==
                      "gamedata\\PALObject\\" + record.model + "\\",
              "scene object type, name and model are present");
        Check(objects.ReadMember((record.model + "\\" + record.model +
                                  ".dff").c_str()).has_value() &&
                  objects.ReadMember((record.model + "\\" + record.variant +
                                      ".png").c_str()).has_value(),
              "scene object model and variant texture exist");
        const auto& xml = record.config.Root();
        Check(xml.name == "PAL4-GameObject" && xml.children.size() == 1u &&
                  xml.children[0].name == "PAL4-GameObject-Parameters",
              "scene object XML root and parameter node");
        const auto* scale = xml.FindAttribute(
            "PAL4-GameObject-object-scale");
        const auto* disappear = xml.FindAttribute(
            "PAL4-GameObject-object-auto-disappear");
        const auto* stored = xml.FindAttribute(
            "PAL4-GameObject-object-store");
        Check(scale && disappear && stored &&
                  std::get_if<float>(&scale->value) &&
                  *std::get_if<float>(&scale->value) == scales[index] &&
                  disappear->ToInt(99u) == auto_disappear[index] &&
                  stored->ToInt(99u) == store[index],
              "scene object XML configuration values");
        if (record.type == 8u) {
            const auto& children = xml.children[0].children;
            const auto* repeat = children.size() == 1u ?
                children[0].FindAttribute("PAL4_GameObject-effect-times") :
                nullptr;
            const auto* effect_name = children.size() == 1u ?
                children[0].FindAttribute("PAL4_GameObject-effect-name") :
                nullptr;
            Check(children.size() == 1u &&
                      children[0].name == "PAL4_GameObject-effect" &&
                      repeat && repeat->ToInt(0u) == 0xffffffffu &&
                      effect_name &&
                      std::get_if<std::string>(&effect_name->value) &&
                      *std::get_if<std::string>(&effect_name->value) ==
                          "H_076",
                  "scene effect XML child and repeat count");
            pal4::NativeEffectScriptAsset script;
            if (!script.Load(effects, "H_076"))
                std::cerr << "H_076: " << script.LastError() << '\n';
            const auto& effect = script.Document().Root();
            const auto find_section = [&](std::string_view name)
                -> const pal4::BinaryXmlNode* {
                const auto found = std::find_if(
                    effect.children.begin(), effect.children.end(),
                    [name](const auto& node) { return node.name == name; });
                return found == effect.children.end() ? nullptr : &*found;
            };
            const auto* object_list = find_section("ObjectList");
            const auto* task_list = find_section("TaskList");
            const auto* oxn_list = find_section("OxnList");
            Check(script.Name() == "H_076" && effect.name == "Effect" &&
                      object_list && object_list->children.size() == 4u &&
                      task_list && task_list->children.size() == 4u &&
                      oxn_list && oxn_list->children.size() == 4u &&
                      std::count_if(oxn_list->children.begin(),
                          oxn_list->children.end(), [](const auto& node) {
                              return node.name == "ClumpObject";
                          }) == 1 &&
                      objects.Find("OJ02\\OJ02.dff") >= 0,
                  "H_076 loads original objects, tasks and OJ02 clump");
            const auto* smoke = script.FindTexture("HY\\smoke 1.png");
            Check(script.TextureCount() == 1u && smoke &&
                      smoke->member == "tex\\HY\\smoke 1.dds" &&
                      !smoke->bytes.empty(),
                  "H_076 caches its original smoke DDS once");
            Check(script.Load(argv[1], "H_076") &&
                      script.Document().Root().name == "Effect" &&
                      script.TextureCount() == 1u,
                  "standalone effect script loader opens the original package");
            Check(!script.Load(effects, "../H_076") &&
                      script.Document().Root().name.empty() &&
                      script.TextureCount() == 0u,
                  "invalid effect script name releases the previous document");
        }
    }
    Check(!catalog.Load(std::filesystem::path(argv[1]), "../Q01", "N01") &&
              catalog.Records().empty(),
          "invalid scene path clears the previous catalog");
    if (!catalog.Load(std::filesystem::path(argv[1]), "Q01", "Q01"))
        std::cerr << "Q01/Q01 GOB: " << catalog.LastError() << '\n';
    Check(!catalog.Records().empty(),
          "second Q01/Q01 scene has parsed objects");
    std::set<std::string> checked_effects;
    for (const auto& record : catalog.Records()) {
        if (record.type != 8u) continue;
        const auto name = record.EffectName();
        Check(name.has_value(), "Q01/Q01 effect has a script name");
        if (!checked_effects.insert(std::string(*name)).second) continue;
        pal4::NativeEffectScriptAsset script;
        Check(script.Load(effects, *name),
              "Q01/Q01 effect and referenced particle textures load");
        const auto expected = *name == "H_224" ? 2u :
            *name == "M_011" ? 0u : 1u;
        Check(script.TextureCount() == expected,
              "Q01/Q01 effect caches each distinct particle texture once");
        if (*name == "H_224") {
            const auto* flame = script.FindTexture("HY\\t050.1_1.3.png");
            const auto* smoke = script.FindTexture("HY\\smoke 1.1.png");
            Check(flame && smoke && !flame->bytes.empty() &&
                      !smoke->bytes.empty(),
                  "H_224 retains both original particle textures");
        }
    }
    Check(checked_effects == std::set<std::string>{
              "H_077", "H_224", "M_011", "Y_006"},
          "Q01/Q01 effect scripts have the inspected visual families");
    pal4::NativeEffectScriptAsset save_exit_effect;
    Check(save_exit_effect.Load(effects, "H_XS027") &&
              save_exit_effect.Name() == "H_XS027" &&
              save_exit_effect.Document().Root().name == "Effect",
          "Save exit H_XS027 parses from original Effect.cpk");
    const auto entrance = std::find_if(
        catalog.Records().begin(), catalog.Records().end(),
        [](const pal4::NativeSceneGobRecord& record) {
            return record.name == "entrance1";
        });
    if (entrance == catalog.Records().end()) {
        for (const auto& record : catalog.Records())
            std::cerr << "Q01/Q01 GOB name: " << record.name << '\n';
    }
    Check(entrance != catalog.Records().end(),
          "second scene contains the first scripted visibility target");
    if (entrance->type != 6u)
        std::cerr << "Q01/Q01 entrance1 type: " << entrance->type << '\n';
    Check(entrance->type == 6u,
          "first scripted visibility target uses the action GOB vtable");
    pal4::NativePackageArchive world_objects;
    Check(world_objects.Open((std::filesystem::path(argv[1]) / "gamedata" /
                              "PALWorld" / "q01.cpk").string().c_str()),
          "open original Q01 world package");
    std::size_t local_models = 0u;
    std::size_t action_animation_tables = 0u;
    std::size_t action_event_sidecars = 0u;
    std::set<std::string> checked_animated_models;
    std::array<std::size_t, 11> type_counts{};
    for (const auto& record : catalog.Records()) {
        Check(record.type < type_counts.size(),
              "Q01/Q01 object type has a registered factory");
        ++type_counts[record.type];
        if (record.resource_dir == "gamedata\\PALWorld\\Q01\\Q01\\") {
            ++local_models;
            Check(world_objects.Find(("Q01\\" + record.model +
                                      ".dff").c_str()) >= 0,
                  "scene-local GOB model exists in the Q01 world package");
        } else {
            Check(record.resource_dir ==
                      "gamedata\\PALObject\\" + record.model + "\\" &&
                      objects.Find((record.model + "\\" + record.model +
                                    ".dff").c_str()) >= 0,
                  "shared GOB model exists in the object package");
        }
        if (record.type == 6u) {
            const auto member = record.resource_dir ==
                "gamedata\\PALWorld\\Q01\\Q01\\"
                ? "Q01\\" + record.model + ".tab"
                : record.model + "\\" + record.model + ".tab";
            const auto present = record.resource_dir ==
                "gamedata\\PALWorld\\Q01\\Q01\\"
                ? world_objects.Find(member.c_str()) >= 0
                : objects.Find(member.c_str()) >= 0;
            if (present) {
                ++action_animation_tables;
            }
        }
        if ((record.type == 5u || record.type == 6u) &&
            checked_animated_models.insert(record.model).second) {
            const bool local = record.resource_dir ==
                "gamedata\\PALWorld\\Q01\\Q01\\";
            auto& archive = local ? world_objects : objects;
            const auto directory = local ? "Q01" : record.model;
            pal4::NativeActorAnimationCatalog animations;
            if (animations.Load(archive, directory, record.model)) {
                for (const auto& [action, filename] : animations.Entries()) {
                    const auto sidecar = std::string(directory) + "\\" +
                        filename.substr(0u, filename.size() - 4u) + ".amf";
                    if (archive.Find(sidecar.c_str()) >= 0) {
                        ++action_event_sidecars;
                    }
                }
            }
        }
    }
    Check(local_models == 2u,
          "Q01/Q01 includes both scene-local object models");
    Check(action_animation_tables == 9u,
          "Q01/Q01 has nine original Action animation tables");
    Check(action_event_sidecars == 0u,
          "Q01/Q01 animated GOB actions have no AMF event sidecars");
    const auto machine = std::find_if(
        catalog.Records().begin(), catalog.Records().end(),
        [](const pal4::NativeSceneGobRecord& record) {
            return record.name == "item101";
        });
    Check(machine != catalog.Records().end() && machine->type == 5u,
          "Q01/Q01 item101 is a Machine object");
    const auto machine_tab = machine->model + "\\" + machine->model + ".tab";
    const auto machine_animation =
        machine->model + "\\" + machine->model + ".anm";
    const auto machine_tab_bytes = objects.ReadMember(machine_tab.c_str());
    Check(machine->model == "OM07" && machine_tab_bytes &&
              std::string_view(reinterpret_cast<const char*>(
                  machine_tab_bytes->data()), machine_tab_bytes->size())
                  .starts_with("{OM07$OM07.anm}") &&
              objects.Find(machine_animation.c_str()) >= 0,
          "Q01/Q01 item101 has its original Machine action table and clip");
    Check(catalog.Records().size() == 48u && type_counts[0] == 2u &&
              type_counts[3] == 13u && type_counts[5] == 1u &&
              type_counts[6] == 9u && type_counts[7] == 8u &&
              type_counts[8] == 15u,
          "Q01/Q01 object type inventory matches the original scene");
    std::set<std::string> effect_names;
    for (const auto& record : catalog.Records()) {
        if (record.type != 8u) continue;
        const auto name = record.EffectName();
        Check(name.has_value(),
              "Q01/Q01 effect object names a script");
        if (!effect_names.emplace(*name).second) continue;
        pal4::NativeEffectScriptAsset script;
        if (!script.Load(effects, *name))
            std::cerr << "Q01/Q01 effect " << *name << ": "
                      << script.LastError() << '\n';
        Check(script.Document().Root().name == "Effect",
              "Q01/Q01 effect script parses from Effect.cpk");
    }
    Check(!effect_names.empty(),
          "Q01/Q01 effect objects reference original scripts");
    Check(catalog.Load(argv[1], "M15", "1"), "original M15 GOB catalog parses");
    for(const auto name : {"effect01", "effect02"}) {
        const auto record=std::find_if(catalog.Records().begin(),catalog.Records().end(),
            [name](const auto& value){return value.name==name;});
        Check(record!=catalog.Records().end() && record->type==8u &&
                  record->flags[0]==0u && record->EffectName()=="066",
              "M15 dormant effects preserve their actual missing 066 reference");
    }
    pal4::NativeEffectScriptAsset dormant;
    Check(!dormant.Load(effects,"066") && dormant.Name().empty() &&
              effects.Find("script\\H_066.eft")>=0,
          "absent 066 is distinct from H_066; hidden GOBs need no substitute");
    if (!catalog.Load(std::filesystem::path(argv[1]), "Q03", "Q03S"))
        std::cerr << "Q03/Q03S GOB: " << catalog.LastError() << '\n';
    Check(!catalog.Records().empty(), "original Shouyang gate GOB parses");
    const auto gate_sign = std::find_if(catalog.Records().begin(),
        catalog.Records().end(), [](const auto& record) {
            return record.name == "ZP001";
        });
    Check(gate_sign != catalog.Records().end() && gate_sign->type == 6u &&
              gate_sign->model == "C" && gate_sign->variant.empty(),
          "Shouyang ZP001 retains the original zero-length variant");
    Check(!catalog.Load(argv[1], "Q04", "CN05C") && catalog.SceneMemberMissing() &&
              catalog.Records().empty(),
          "original CN05C has no GOB member, independently of physical CN05");
    Check(catalog.Load(argv[1], "Q04", "CN05") && !catalog.SceneMemberMissing() &&
              catalog.Records().empty(),
          "original CN05 has a present zero-object GOB, distinct from a missing member");
    Check(!catalog.Load(argv[1], "../Q04", "CN05C") && !catalog.SceneMemberMissing(),
          "invalid paths are not classified as optional missing members");
    Check(!catalog.Load(std::filesystem::path(argv[1]) / "missing-gob-test-root", "Q04", "CN05C") &&
              !catalog.SceneMemberMissing(),
          "unavailable package is not an optional missing member");
    Check(catalog.Load(argv[1], "Q01", "N01") && !catalog.SceneMemberMissing() &&
              catalog.Records().size()==4u,
          "later populated load resets the missing status and restores actual records");
}
