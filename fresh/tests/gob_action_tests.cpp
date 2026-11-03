#include "pal4/gob_action.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& name) {
    const auto found = std::find(calls.begin(), calls.end(), name);
    if (found == calls.end()) throw std::runtime_error("missing " + name);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime final : pal4::IGobActionRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t snapshot_allocation = 0x2000u;
    std::uint32_t snapshot_type = 6u;
    bool fail_snapshot_construct = false;
    std::uint32_t animation_flag = 1u;
    bool fail_animation = false;
    float last_delta = -1.0f;
    std::uint32_t scene_package = 0u;
    std::uint32_t model_result = 1u;
    std::map<std::pair<std::uint32_t, std::string>, std::uint32_t> xml_children;
    std::map<std::string, std::uint32_t> xml_attributes;
    std::map<std::uint32_t, std::int32_t> attribute_values;
    std::vector<std::pair<std::string, std::int32_t>> written_attributes;
    std::uint32_t last_visible = 0xffffffffu;
    std::uint32_t last_cleared_flag = 0xffffffffu;
    float last_animation_time = -1.0f;
    std::string snapshot_name;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobAction::kObjectSize || size == 0x1cu,
              "action allocation size");
        calls.emplace_back("allocate");
        return size == 0x1cu ? snapshot_allocation : allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == object || memory == 0x2000u,
              "action freed allocation");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "action base constructor");
        calls.emplace_back("construct base");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "action base destructor");
        calls.emplace_back("destruct base");
    }
    void InitializeAnimation(std::uint32_t state) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation state initializer");
        calls.emplace_back("initialize animation");
        if (fail_animation) throw std::runtime_error("animation init failed");
    }
    void DestructAnimation(std::uint32_t state) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation state destructor");
        calls.emplace_back("destruct animation");
    }
    void CleanupAnimation(std::uint32_t state) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation cleanup");
        calls.emplace_back("cleanup animation");
    }
    std::uint32_t CleanupBase(std::uint32_t address,
                              std::uint32_t context) override {
        Check(address == object && context == 0x8000u,
              "action base cleanup arguments");
        calls.emplace_back("cleanup base");
        return 0x1234u;
    }
    void RenderBase(std::uint32_t address,
                    std::uint32_t argument) override {
        Check(address == object && argument == 7u,
              "action base render arguments");
        calls.emplace_back("render base");
    }
    std::uint32_t ProcessAnimation(std::uint32_t state,
                                    std::uint32_t argument) override {
        Check(state == object + pal4::GobAction::kAnimationOffset &&
                  argument == 7u,
              "action process animation arguments");
        calls.emplace_back("process animation");
        return 0x5678u;
    }
    void UpdateBase(std::uint32_t address, float delta) override {
        Check(address == object, "action base update object");
        last_delta = delta;
        calls.emplace_back("update base");
    }
    void UpdateInterpolator(std::uint32_t state,
                            float delta) override {
        Check(state == object + pal4::GobAction::kAnimationOffset &&
                  delta == last_delta,
              "action animation update arguments");
        calls.emplace_back("update interpolator");
    }
    std::uint32_t AnimationFlag(std::uint32_t state) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation flag handle");
        calls.emplace_back("animation flag");
        return animation_flag;
    }
    void SetAnimationTime(std::uint32_t state, float time) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation time state");
        last_animation_time = time;
        calls.emplace_back("set animation time");
    }
    std::uint32_t ClearAnimationFlag(
        std::uint32_t state, std::uint32_t flag) override {
        Check(state == object + pal4::GobAction::kAnimationOffset &&
                  flag <= 1u,
              "action clear pause flag");
        last_cleared_flag = flag;
        calls.emplace_back("clear animation flag");
        return 0x89abu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "action pre-load object");
        calls.emplace_back("cleanup before scene load");
    }
    bool UsesObjectPackage(std::string_view path) override {
        calls.emplace_back("check object package");
        return path.find("palobject") != std::string_view::npos;
    }
    bool UsesWeaponPackage(std::string_view path) override {
        calls.emplace_back("check weapon package");
        return path.find("palweapon") != std::string_view::npos;
    }
    void LoadObjectPackage() override {
        calls.emplace_back("load object package");
    }
    void LoadWeaponPackage() override {
        calls.emplace_back("load weapon package");
    }
    std::uint32_t CurrentScenePackage() override {
        calls.emplace_back("current scene package");
        return scene_package;
    }
    void LoadScenePackage(std::uint32_t package) override {
        Check(package == scene_package && package != 0u,
              "action scene package");
        calls.emplace_back("load scene package");
    }
    std::uint32_t LoadBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t animation_data,
        std::uint32_t final_argument) override {
        Check(address == object && first == 2u &&
                  !path.empty() && animation_data == 0x3000u &&
                  final_argument == 0x4000u,
              "action base model arguments");
        calls.emplace_back("load base model");
        return model_result;
    }
    void HideHookUserDataAtomics(std::uint32_t clump) override {
        Check(clump == 0x5000u, "action clump hook atomics");
        calls.emplace_back("hide hook atomics");
    }
    void LoadAnimationData(
        std::uint32_t state, std::uint32_t clump,
        std::string_view path,
        std::uint32_t animation_data) override {
        Check(state == object + pal4::GobAction::kAnimationOffset &&
                  clump == 0x5000u && !path.empty() &&
                  animation_data == 0x3000u,
              "action animation load arguments");
        calls.emplace_back("load animation data");
    }
    void SetAnimationFlag(std::uint32_t state,
                          std::uint32_t flag) override {
        Check(state == object + pal4::GobAction::kAnimationOffset &&
                  flag == 1u,
              "action animation load flag");
        calls.emplace_back("set animation flag");
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup script packages");
    }
    std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) override {
        calls.emplace_back("find XML child");
        const auto found = xml_children.find(
            {parent, std::string(name)});
        return found == xml_children.end() ? 0u : found->second;
    }
    std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) override {
        Check(parent == 0x7000u &&
                  name == "PAL4_GameObject-action",
              "action XML child creation");
        calls.emplace_back("create XML child");
        return 0x9000u;
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x8000u,
              "action XML child removal");
        calls.emplace_back("remove XML child");
    }
    void WriteXmlInt(std::uint32_t node,
                     std::string_view name,
                     std::int32_t value) override {
        Check(node == 0x9000u, "action XML attribute node");
        written_attributes.emplace_back(name, value);
        calls.emplace_back("write XML int");
    }
    std::uint32_t FindXmlAttribute(
        std::uint32_t node, std::string_view name) override {
        Check(node == 0x8000u || node == 0x9000u,
              "action XML lookup node");
        calls.emplace_back("find XML attribute");
        const auto found = xml_attributes.find(std::string(name));
        return found == xml_attributes.end() ? 0u : found->second;
    }
    std::int32_t XmlAttributeToInt(
        std::uint32_t attribute, std::int32_t fallback) override {
        Check(fallback == 0 || fallback == 1,
              "action XML int default");
        calls.emplace_back("parse XML int");
        return attribute_values.at(attribute);
    }
    void ResetAnimationSlot(std::uint32_t address) override {
        Check(address == object, "action reset slot object");
        calls.emplace_back("reset animation slot");
    }
    float AnimationLength(std::uint32_t state) override {
        Check(state == object + pal4::GobAction::kAnimationOffset,
              "action animation length state");
        calls.emplace_back("animation length");
        return 4.0f;
    }
    void SetBaseVisible(std::uint32_t address,
                        std::uint32_t visible) override {
        Check(address == object, "action base visibility object");
        last_visible = visible;
        calls.emplace_back("set base visible");
    }
    void ApplyBaseParameters(std::uint32_t address) override {
        Check(address == object, "action base config object");
        calls.emplace_back("apply base parameters");
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x3000u, "action object name pointer");
        calls.emplace_back("read object name");
        return "JQB";
    }
    void ConstructSnapshot(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "action snapshot constructor");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot_construct)
            throw std::runtime_error("snapshot construction failed");
    }
    void AssignSnapshotName(std::uint32_t snapshot,
                            std::string_view name) override {
        Check(snapshot == 0x2000u, "action snapshot name target");
        snapshot_name = name;
        calls.emplace_back("assign snapshot name");
    }
    std::uint32_t SnapshotType(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "action snapshot type target");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
};

void CheckLifecycle() {
    Check(pal4::GobAction::TypeKey() == 6u, "action factory key");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobAction::Create(unavailable) == 0u &&
              unavailable.calls ==
                  (std::vector<std::string>{"allocate"}),
          "action allocation failure");
    Runtime created;
    Check(pal4::GobAction::Create(created) == Runtime::object &&
              created.words.at(Runtime::object) ==
                  pal4::GobAction::kVtable &&
              created.words.at(Runtime::object +
                               pal4::GobAction::kAnimationOffset) ==
                  pal4::GobAction::kAnimationVtable &&
              created.words.at(Runtime::object + 0x258u) == 1u &&
              created.words.at(Runtime::object + 0x25cu) == 0u &&
              created.words.at(Runtime::object + 0x260u) == 0u &&
              created.calls[0] == "allocate" &&
              created.calls[1] == "construct base" &&
              created.calls[2] == "initialize animation",
          "action factory and constructor layout");
    created.calls.clear();
    pal4::GobAction::Destruct(Runtime::object, created);
    Check(created.calls ==
              (std::vector<std::string>{
                  "write word", "write word",
                  "destruct animation", "destruct base"}),
          "action destructor order");

    Runtime failed;
    failed.fail_animation = true;
    bool thrown = false;
    try {
        (void)pal4::GobAction::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct base", "initialize animation",
                  "destruct base", "free"}),
          "action exceptional construction cleans base and allocation");
}

void CheckAnimation() {
    Runtime runtime;
    Check(pal4::GobAction::CleanupResources(
              Runtime::object, 0x8000u, runtime) == 0x1234u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "cleanup animation", "cleanup base"}),
          "action animation cleanup before base");
    runtime.calls.clear();
    Check(pal4::GobAction::HandleEvent(
              Runtime::object, 7u, runtime) == 0x5678u &&
              runtime.calls ==
                  (std::vector<std::string>{
                      "render base", "process animation"}),
          "action render and animation event order");

    Runtime inactive;
    inactive.animation_flag = 0u;
    Check(pal4::GobAction::Update(
              Runtime::object, 0.25f, inactive) == 0u &&
              inactive.calls ==
                  (std::vector<std::string>{
                      "update base", "update interpolator",
                      "animation flag"}),
          "action inactive animation update");
    Runtime continuing;
    continuing.words[Runtime::object + 0x25cu] = 2u;
    Check(pal4::GobAction::Update(
              Runtime::object, 0.5f, continuing) == 1u &&
              continuing.words.at(Runtime::object + 0x25cu) == 1u &&
              continuing.calls.back() == "write word",
          "action decrements positive play count");
    Runtime ending;
    ending.words[Runtime::object + 0x25cu] = 1u;
    ending.words[Runtime::object + 0x260u] = 0u;
    Check(pal4::GobAction::Update(
              Runtime::object, 0.5f, ending) == 0x89abu &&
              ending.words.at(Runtime::object + 0x25cu) == 0u &&
              ending.last_animation_time == 0.0f &&
              ending.calls[ending.calls.size() - 2u] ==
                  "set animation time" &&
              ending.calls.back() == "clear animation flag",
          "action final play resets time and pauses");
    Runtime holding;
    holding.words[Runtime::object + 0x25cu] = 1u;
    holding.words[Runtime::object + 0x260u] = 1u;
    Check(pal4::GobAction::Update(
              Runtime::object, 0.5f, holding) == 0x89abu &&
              std::find(holding.calls.begin(), holding.calls.end(),
                        "set animation time") == holding.calls.end(),
          "action hold-end keeps animation time");
    Runtime unlimited;
    unlimited.words[Runtime::object + 0x25cu] = 0xffffffffu;
    Check(pal4::GobAction::Update(
              Runtime::object, 0.5f, unlimited) == 0xffffffffu &&
              unlimited.words.at(Runtime::object + 0x25cu) ==
                  0xffffffffu,
          "action negative play count does not decrement");
}

void CheckSceneInitialization() {
    Runtime object_package;
    object_package.words[Runtime::object + 0x14u] = 0x5000u;
    object_package.words[Runtime::object + 0x1c0u] = 0x6000u;
    object_package.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    object_package.xml_children[
        {0x7000u, "PAL4_GameObject-action"}] = 0x8000u;
    Check(pal4::GobAction::InitializeSceneAndAnimation(
              Runtime::object, 2u, "palobject/JQB", 0x3000u,
              0x4000u, object_package) == 1u &&
              IndexOf(object_package.calls, "cleanup before scene load") <
                  IndexOf(object_package.calls, "load object package") &&
              IndexOf(object_package.calls, "load object package") <
                  IndexOf(object_package.calls, "load base model") &&
              IndexOf(object_package.calls, "load base model") <
                  IndexOf(object_package.calls, "hide hook atomics") &&
              IndexOf(object_package.calls, "hide hook atomics") <
                  IndexOf(object_package.calls, "load animation data") &&
              IndexOf(object_package.calls, "load animation data") <
                  IndexOf(object_package.calls, "set animation flag") &&
              IndexOf(object_package.calls, "remove XML child") <
                  IndexOf(object_package.calls, "create XML child") &&
              object_package.calls.back() == "cleanup script packages",
          "action object package load and animation/XML order");

    Runtime weapon;
    weapon.words[Runtime::object + 0x14u] = 0x5000u;
    weapon.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobAction::InitializeSceneAndAnimation(
              Runtime::object, 2u, "palweapon/JQB", 0x3000u,
              0x4000u, weapon) == 1u &&
              IndexOf(weapon.calls, "load weapon package") <
                  IndexOf(weapon.calls, "load base model") &&
              std::find(weapon.calls.begin(), weapon.calls.end(),
                        "load scene package") == weapon.calls.end(),
          "action weapon package selection");

    Runtime scene;
    scene.scene_package = 0xabcdu;
    scene.model_result = 0u;
    Check(pal4::GobAction::InitializeSceneAndAnimation(
              Runtime::object, 2u, "scene/JQB", 0x3000u,
              0x4000u, scene) == 0u &&
              IndexOf(scene.calls, "load scene package") <
                  IndexOf(scene.calls, "load base model") &&
              std::find(scene.calls.begin(), scene.calls.end(),
                        "load animation data") == scene.calls.end() &&
              scene.calls.back() == "cleanup script packages",
          "action failed base model still cleans scripts");
    Runtime no_scene;
    no_scene.model_result = 0u;
    (void)pal4::GobAction::InitializeSceneAndAnimation(
        Runtime::object, 2u, "scene/JQB", 0x3000u,
        0x4000u, no_scene);
    Check(std::find(no_scene.calls.begin(), no_scene.calls.end(),
                    "load scene package") == no_scene.calls.end(),
          "action missing scene package is skipped");
}

void CheckActionConfigNodeAndVisibility() {
    Runtime no_document;
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobAction::GetOrCreateActionConfigNode(
              Runtime::object, no_document) == 0u &&
              no_document.calls.empty(),
          "action missing XML document");
    Runtime no_parameters;
    no_parameters.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobAction::GetOrCreateActionConfigNode(
              Runtime::object, no_parameters) == 0u &&
              no_parameters.calls ==
                  (std::vector<std::string>{"find XML child"}),
          "action missing parameters node");
    Runtime existing;
    existing.words[Runtime::object + 0x1c0u] = 0x6000u;
    existing.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    existing.xml_children[
        {0x7000u, "PAL4_GameObject-action"}] = 0x8000u;
    Check(pal4::GobAction::GetOrCreateActionConfigNode(
              Runtime::object, existing) == 0x8000u &&
              existing.written_attributes.empty(),
          "action existing config retained");
    Runtime created;
    created.words[Runtime::object + 0x1c0u] = 0x6000u;
    created.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    Check(pal4::GobAction::GetOrCreateActionConfigNode(
              Runtime::object, created) == 0x9000u &&
              created.written_attributes ==
                  (std::vector<std::pair<std::string, std::int32_t>>{
                      {"PAL4_GameObject-action-default-play", 1},
                      {"PAL4_GameObject-action-play-times", 0},
                      {"PAL4_GameObject-action-holding-end", 0}}),
          "action new node receives three exact default attributes");

    Runtime reset;
    Check(pal4::GobAction::ResetAnimation(
              Runtime::object, reset) == 0x89abu &&
              reset.calls ==
                  (std::vector<std::string>{
                      "reset animation slot", "set animation time",
                      "clear animation flag"}) &&
              reset.last_animation_time == 0.0f &&
              reset.last_cleared_flag == 0u,
          "action reset slot, time and flag");

    Runtime hidden;
    pal4::GobAction::SetVisibleAndAnimationState(
        Runtime::object, 0u, hidden);
    Check(hidden.calls ==
              (std::vector<std::string>{"set base visible"}),
          "action hidden skips animation state");
    Runtime hold_end;
    hold_end.words[Runtime::object + 0x25cu] = 0u;
    hold_end.words[Runtime::object + 0x260u] = 1u;
    pal4::GobAction::SetVisibleAndAnimationState(
        Runtime::object, 1u, hold_end);
    Check(hold_end.last_cleared_flag == 1u &&
              hold_end.last_animation_time == 4.0f &&
              IndexOf(hold_end.calls, "animation length") <
                  IndexOf(hold_end.calls, "set animation time"),
          "action visible hold-end seeks to animation end");
    Runtime restart;
    restart.words[Runtime::object + 0x25cu] = 0u;
    restart.words[Runtime::object + 0x260u] = 0u;
    pal4::GobAction::SetVisibleAndAnimationState(
        Runtime::object, 1u, restart);
    Check(restart.last_animation_time == 0.0f &&
              std::find(restart.calls.begin(), restart.calls.end(),
                        "animation length") == restart.calls.end(),
          "action visible without hold-end seeks to start");
    for (const auto default_play : {0u, 1u}) {
        Runtime counted;
        counted.words[Runtime::object + 0x25cu] = 2u;
        counted.words[Runtime::object + 0x258u] = default_play;
        pal4::GobAction::SetVisibleAndAnimationState(
            Runtime::object, 1u, counted);
        Check(counted.last_cleared_flag ==
                  (default_play == 0u ? 1u : 0u) &&
                  std::find(counted.calls.begin(), counted.calls.end(),
                            "set animation time") == counted.calls.end(),
              "action counted visibility follows default-play flag");
    }
}

void CheckReadConfig() {
    Runtime no_document;
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    pal4::GobAction::ReadConfig(Runtime::object, no_document);
    Check(no_document.calls ==
              (std::vector<std::string>{"apply base parameters"}),
          "action config always applies base first");

    Runtime defaults;
    defaults.words[Runtime::object + 0x1c0u] = 0x6000u;
    defaults.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    defaults.xml_children[
        {0x7000u, "PAL4_GameObject-action"}] = 0x8000u;
    pal4::GobAction::ReadConfig(Runtime::object, defaults);
    Check(defaults.words.at(Runtime::object + 0x258u) == 1u &&
              defaults.words.at(Runtime::object + 0x25cu) == 0u &&
              defaults.words.at(Runtime::object + 0x260u) == 0u &&
              defaults.last_cleared_flag == 0u &&
              defaults.calls.front() == "apply base parameters" &&
              defaults.calls.back() == "clear animation flag",
          "action missing attributes use defaults");

    Runtime configured;
    configured.words[Runtime::object + 0x1c0u] = 0x6000u;
    configured.xml_children = defaults.xml_children;
    configured.xml_attributes = {
        {"PAL4_GameObject-action-default-play", 1u},
        {"PAL4_GameObject-action-play-times", 2u},
        {"PAL4_GameObject-action-holding-end", 3u}};
    configured.attribute_values = {{1u, 0}, {2u, -1}, {3u, 1}};
    pal4::GobAction::ReadConfig(Runtime::object, configured);
    Check(configured.words.at(Runtime::object + 0x258u) == 0u &&
              configured.words.at(Runtime::object + 0x25cu) ==
                  0xffffffffu &&
              configured.words.at(Runtime::object + 0x260u) == 1u &&
              configured.last_cleared_flag == 1u &&
              std::count(configured.calls.begin(), configured.calls.end(),
                         "parse XML int") == 3,
          "action explicit attributes and pause flag");
}

void CheckStateSnapshot() {
    Runtime disabled;
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobAction::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls.empty(),
          "action disabled store skips snapshot allocation");
    Runtime allocation_failed;
    allocation_failed.words[Runtime::object + 0x1a4u] = 1u;
    allocation_failed.snapshot_allocation = 0u;
    Check(pal4::GobAction::CreateStateSnapshot(
              Runtime::object, allocation_failed) == 0u &&
              allocation_failed.calls ==
                  (std::vector<std::string>{"allocate"}),
          "action snapshot allocation failure");
    Runtime created;
    created.words[Runtime::object + 0x1a4u] = 1u;
    created.words[Runtime::object + 4u] = 0x3000u;
    created.words[Runtime::object + 0x184u] = 7u;
    created.words[Runtime::object + 0x25cu] = 3u;
    Check(pal4::GobAction::CreateStateSnapshot(
              Runtime::object, created) == 0x2000u &&
              created.snapshot_name == "JQB" &&
              created.words.at(0x2014u) == 7u &&
              created.words.at(0x2018u) == 3u &&
              created.calls ==
                  (std::vector<std::string>{
                      "allocate", "construct snapshot", "read object name",
                      "assign snapshot name", "write word", "write word"}),
          "action snapshot captures name, visibility and play count");
    Runtime construction_failed;
    construction_failed.words[Runtime::object + 0x1a4u] = 1u;
    construction_failed.fail_snapshot_construct = true;
    bool thrown = false;
    try {
        (void)pal4::GobAction::CreateStateSnapshot(
            Runtime::object, construction_failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && construction_failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct snapshot", "free"}),
          "action snapshot construction failure frees allocation");

    pal4::GobAction::ApplyStateSnapshot(
        Runtime::object, 0x2000u, disabled);
    Check(disabled.calls.empty(), "action disabled store skips restore");
    Runtime wrong_type;
    wrong_type.words[Runtime::object + 0x1a4u] = 1u;
    wrong_type.snapshot_type = 4u;
    pal4::GobAction::ApplyStateSnapshot(
        Runtime::object, 0x2000u, wrong_type);
    Check(wrong_type.calls ==
              (std::vector<std::string>{"snapshot type"}),
          "action snapshot type gate");
    Runtime zero;
    zero.words[Runtime::object + 0x1a4u] = 1u;
    zero.words[Runtime::object + 0x25cu] = 2u;
    zero.words[Runtime::object + 0x258u] = 1u;
    zero.words[Runtime::object + 0x260u] = 1u;
    zero.words[0x2014u] = 0u;
    zero.words[0x2018u] = 0u;
    pal4::GobAction::ApplyStateSnapshot(
        Runtime::object, 0x2000u, zero);
    Check(zero.last_visible == 0u &&
              zero.words.at(Runtime::object + 0x25cu) == 0u &&
              zero.last_cleared_flag == 1u &&
              zero.last_animation_time == 4.0f &&
              IndexOf(zero.calls, "set base visible") <
                  IndexOf(zero.calls, "write word"),
          "action zero-play snapshot seeks to held end");
    Runtime counted;
    counted.words[Runtime::object + 0x1a4u] = 1u;
    counted.words[Runtime::object + 0x25cu] = 1u;
    counted.words[Runtime::object + 0x258u] = 0u;
    counted.words[0x2014u] = 1u;
    counted.words[0x2018u] = 5u;
    pal4::GobAction::ApplyStateSnapshot(
        Runtime::object, 0x2000u, counted);
    Check(counted.last_visible == 1u &&
              counted.words.at(Runtime::object + 0x25cu) == 5u &&
              counted.last_cleared_flag == 1u &&
              std::count(counted.calls.begin(), counted.calls.end(),
                         "clear animation flag") == 2,
          "action counted snapshot reapplies default-play state");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckAnimation();
        CheckSceneInitialization();
        CheckActionConfigNodeAndVisibility();
        CheckReadConfig();
        CheckStateSnapshot();
        std::cout << "gob_action_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
