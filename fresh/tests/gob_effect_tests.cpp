#include "pal4/gob_effect.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
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

struct Runtime final : pal4::IGobEffectRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::pair<std::uint32_t, std::string>,
             std::uint32_t> xml_children;
    std::map<std::string, std::uint32_t> xml_attributes;
    std::map<std::uint32_t, std::int32_t> xml_values;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t snapshot_allocation = 0x2000u;
    std::uint32_t snapshot_type = 8u;
    bool fail_snapshot_construct = false;
    std::string snapshot_name;
    std::uint32_t model_result = 0xabcdu;
    std::uint32_t scene_package = 0u;
    bool fail_handle = false;
    bool fail_string = false;
    bool needs_update = false;
    bool created_needs_update = false;
    std::string effect_name;
    std::string created_name;
    std::uint32_t created_times = 0u;
    bool created_distance_clip = false;

    void StoreWord(std::uint32_t address, std::uint32_t value) override {
        WriteWord(address, value);
    }

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobEffect::kSize || size == 0x18u,
              "effect allocation size");
        calls.emplace_back("allocate");
        return size == 0x18u ? snapshot_allocation : allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == object || memory == 0x2000u,
              "effect freed allocation");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "effect base constructor");
        calls.emplace_back("construct base");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "effect base destructor");
        calls.emplace_back("destruct base");
    }
    void ConstructEffectHandle(std::uint32_t handle) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset,
              "effect handle constructor");
        calls.emplace_back("construct effect handle");
        if (fail_handle)
            throw std::runtime_error("handle construction failed");
    }
    void ResetEffectHandle(std::uint32_t handle) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset,
              "effect handle reset");
        calls.emplace_back("reset effect handle");
    }
    void InitializeEmptyString(std::uint32_t record) override {
        Check(record == object + pal4::GobEffect::kStringOffset,
              "effect string initialization");
        calls.emplace_back("initialize empty string");
        if (fail_string)
            throw std::runtime_error("string initialization failed");
    }
    void CleanupString(std::uint32_t record) override {
        Check(record == object + pal4::GobEffect::kStringOffset,
              "effect string cleanup");
        calls.emplace_back("cleanup string");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset,
              "effect update handle");
        calls.emplace_back("effect needs update");
        return std::find(calls.begin(), calls.end(), "create effect") ==
                   calls.end() ? needs_update : created_needs_update;
    }
    void HandleEffectEvent(std::uint32_t handle,
                           std::uint32_t context,
                           std::uint32_t mode) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset &&
                  context == 0x8000u && mode == 1u,
              "effect event arguments");
        calls.emplace_back("handle effect event");
    }
    std::uint32_t CleanupBase(std::uint32_t address,
                              std::uint32_t context) override {
        Check(address == object && context == 0x8000u,
              "effect base cleanup arguments");
        calls.emplace_back("cleanup base");
        return 0x1234u;
    }
    std::uint32_t RenderBase(std::uint32_t address,
                             std::uint32_t argument) override {
        Check(address == object && argument == 7u,
              "effect inherited render arguments");
        calls.emplace_back("render base");
        return 0x5678u;
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "effect pre-load object");
        calls.emplace_back("cleanup before load");
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
              "effect scene package");
        calls.emplace_back("load scene package");
    }
    std::uint32_t LoadBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u &&
                  !path.empty() && fourth == 0x3000u &&
                  fifth == 0x4000u,
              "effect base model arguments");
        calls.emplace_back("load base model");
        return model_result;
    }
    std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) override {
        calls.emplace_back("find XML child");
        const auto found = xml_children.find({parent, std::string(name)});
        return found == xml_children.end() ? 0u : found->second;
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x8000u,
              "effect old XML node removal");
        calls.emplace_back("remove XML child");
    }
    std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) override {
        Check(parent == 0x7000u &&
                  name == "PAL4_GameObject-effect",
              "effect XML child creation");
        calls.emplace_back("create XML child");
        return 0x9000u;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup script packages");
    }
    std::uint32_t InitializeGameState(
        std::uint32_t state, std::uint32_t mode) override {
        Check(state == 9u && mode == 0u,
              "effect saver state arguments");
        calls.emplace_back("initialize game state");
        return 0x9abcu;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    void ApplyBaseParameters(std::uint32_t address) override {
        Check(address == object, "effect base parameters object");
        calls.emplace_back("apply base parameters");
    }
    std::string ReadXmlString(std::uint32_t node,
                              std::string_view name,
                              std::string_view fallback) override {
        Check(node == 0x8000u &&
                  name == "PAL4_GameObject-effect-name" &&
                  fallback.empty(), "effect name XML key");
        calls.emplace_back("read XML string");
        return "flame";
    }
    void AssignEffectName(std::uint32_t record,
                          std::string_view name) override {
        Check(record == object + pal4::GobEffect::kStringOffset,
              "effect name record");
        effect_name = name;
        calls.emplace_back("assign effect name");
    }
    std::uint32_t FindXmlAttribute(std::uint32_t node,
                                    std::string_view name) override {
        Check(node == 0x8000u, "effect attribute node");
        calls.emplace_back("find XML attribute");
        const auto found = xml_attributes.find(std::string(name));
        return found == xml_attributes.end() ? 0u : found->second;
    }
    std::int32_t XmlAttributeToInt(std::uint32_t attribute,
                                    std::int32_t fallback) override {
        calls.emplace_back("parse XML integer");
        const auto found = xml_values.find(attribute);
        return found == xml_values.end() ? fallback : found->second;
    }
    std::uint32_t UpdateClumpTransform(std::uint32_t address,
                                        std::uint32_t transform,
                                        std::uint32_t mode) override {
        Check(address == object && transform == 0x2000u && mode == 3u,
              "effect clump transform arguments");
        calls.emplace_back("update clump transform");
        return 0x4321u;
    }
    std::string EffectName(std::uint32_t record) override {
        Check(record == object + pal4::GobEffect::kStringOffset,
              "effect name lookup");
        calls.emplace_back("effect name");
        return effect_name;
    }
    std::pair<std::uint32_t, std::uint32_t> CreateEffect(
        std::string_view name, std::uint32_t times,
        bool distance_clip) override {
        created_name = name;
        created_times = times;
        created_distance_clip = distance_clip;
        calls.emplace_back("create effect");
        return {0xaaaau, 0xbbbbu};
    }
    void ResetTemporaryEffect() override {
        calls.emplace_back("reset temporary effect");
    }
    void UpdateEffectProperties(std::uint32_t handle,
                                std::uint32_t atomic) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset &&
                  atomic == 0x7777u, "effect clump atomic");
        calls.emplace_back("update effect properties");
    }
    std::uint8_t UpdateEffectState(std::uint32_t handle) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset,
              "effect state handle");
        calls.emplace_back("update effect state");
        return 0x5au;
    }
    std::uint8_t UpdateDialogContent(std::uint32_t handle) override {
        Check(handle == object + pal4::GobEffect::kEffectHandleOffset,
              "effect dialog handle");
        calls.emplace_back("update dialog content");
        return 0x3cu;
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x4000u, "effect object name address");
        calls.emplace_back("read object name");
        return "Torch";
    }
    void ConstructSnapshot(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "effect snapshot constructor");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot_construct)
            throw std::runtime_error("snapshot construction failed");
    }
    void AssignSnapshotName(std::uint32_t snapshot,
                            std::string_view name) override {
        Check(snapshot == 0x2000u, "effect snapshot name target");
        snapshot_name = name;
        calls.emplace_back("assign snapshot name");
    }
    std::uint32_t SnapshotType(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "effect snapshot type target");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
    void SetBaseVisible(std::uint32_t address,
                        std::uint32_t visible) override {
        Check(address == object && visible == 2u,
              "effect restored visibility");
        calls.emplace_back("set base visible");
    }
};

void CheckLifecycle() {
    Check(pal4::GobEffect::TypeKey() == 8u,
          "effect factory type key");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobEffect::Create(unavailable) == 0u &&
              unavailable.calls ==
                  (std::vector<std::string>{"allocate"}),
          "effect allocation failure");
    Runtime created;
    Check(pal4::GobEffect::Create(created) == Runtime::object &&
              created.words.at(Runtime::object) ==
                  pal4::GobEffect::kVtable &&
              created.words.at(Runtime::object + 0x170u) == 0u &&
              created.words.at(Runtime::object + 0x1f0u) == 1u &&
              created.words.at(Runtime::object + 0x1f8u) == 0u &&
              created.calls[0] == "allocate" &&
              created.calls[1] == "construct base" &&
              created.calls[2] == "construct effect handle" &&
              created.calls[3] == "initialize empty string",
          "effect constructor field defaults and order");
    created.calls.clear();
    pal4::GobEffect::Destruct(Runtime::object, created);
    Check(created.calls ==
              (std::vector<std::string>{
                  "write word", "cleanup string",
                  "reset effect handle", "destruct base"}),
          "effect destructor order");

    Runtime handle_failed;
    handle_failed.fail_handle = true;
    bool thrown = false;
    try {
        (void)pal4::GobEffect::Create(handle_failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && handle_failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct base", "construct effect handle",
                  "destruct base", "free"}),
          "effect handle failure cleans base and allocation");
    Runtime string_failed;
    string_failed.fail_string = true;
    thrown = false;
    try {
        (void)pal4::GobEffect::Create(string_failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && string_failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct base", "construct effect handle",
                  "initialize empty string", "reset effect handle",
                  "destruct base", "free"}),
          "effect string failure cleans handle, base and allocation");
}

void CheckRuntimeGates() {
    Runtime cleanup;
    cleanup.needs_update = true;
    Check(pal4::GobEffect::CleanupResources(
              Runtime::object, 0x8000u, cleanup) == 0x1234u &&
              cleanup.calls ==
                  (std::vector<std::string>{
                      "effect needs update", "handle effect event",
                      "cleanup base"}),
          "effect update precedes base cleanup");
    Runtime no_update;
    Check(pal4::GobEffect::CleanupResources(
              Runtime::object, 0x8000u, no_update) == 0x1234u &&
              no_update.calls ==
                  (std::vector<std::string>{
                      "effect needs update", "cleanup base"}),
          "effect cleanup without pending event");
    Runtime hidden;
    hidden.words[Runtime::object + 0x170u] = 0u;
    Check(pal4::GobEffect::HandleEventIfEnabled(
              Runtime::object, 7u, hidden) == 0u &&
              hidden.calls.empty(),
          "effect disabled event gate");
    hidden.words[Runtime::object + 0x170u] = 1u;
    Check(pal4::GobEffect::HandleEventIfEnabled(
              Runtime::object, 7u, hidden) == 0x5678u &&
              hidden.calls ==
                  (std::vector<std::string>{"render base"}),
          "effect enabled inherited render");

    Runtime no_saver;
    no_saver.words[Runtime::object + 0x1f4u] = 0u;
    Check(pal4::GobEffect::TriggerSaverStateIfEnabled(
              Runtime::object, no_saver) == 0u &&
              no_saver.calls.empty(),
          "effect missing saver flag");
    no_saver.words[Runtime::object + 0x1f4u] = 1u;
    Check(pal4::GobEffect::TriggerSaverStateIfEnabled(
              Runtime::object, no_saver) == 0x9abcu &&
              no_saver.calls ==
                  (std::vector<std::string>{"initialize game state"}),
          "effect saver transitions to state nine");
}

void CheckSceneInitialization() {
    Runtime object_package;
    object_package.words[Runtime::object + 0x1c0u] = 0x6000u;
    object_package.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    object_package.xml_children[
        {0x7000u, "PAL4_GameObject-effect"}] = 0x8000u;
    Check(pal4::GobEffect::InitializeSceneAndConfig(
              Runtime::object, 2u, "palobject/FX", 0x3000u,
              0x4000u, object_package) == 0xabcdu &&
              IndexOf(object_package.calls, "cleanup before load") <
                  IndexOf(object_package.calls, "load object package") &&
              IndexOf(object_package.calls, "load object package") <
                  IndexOf(object_package.calls, "load base model") &&
              IndexOf(object_package.calls, "remove XML child") <
                  IndexOf(object_package.calls, "create XML child") &&
              object_package.calls.back() == "cleanup script packages",
          "effect object package and XML replacement");
    Runtime weapon;
    weapon.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobEffect::InitializeSceneAndConfig(
              Runtime::object, 2u, "palweapon/FX", 0x3000u,
              0x4000u, weapon) == 0xabcdu &&
              IndexOf(weapon.calls, "load weapon package") <
                  IndexOf(weapon.calls, "load base model") &&
              weapon.calls.back() == "cleanup script packages",
          "effect weapon package and absent XML");
    Runtime scene;
    scene.scene_package = 0x5000u;
    scene.model_result = 0u;
    Check(pal4::GobEffect::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/FX", 0x3000u,
              0x4000u, scene) == 0u &&
              IndexOf(scene.calls, "load scene package") <
                  IndexOf(scene.calls, "load base model") &&
              std::find(scene.calls.begin(), scene.calls.end(),
                        "find XML child") == scene.calls.end() &&
              scene.calls.back() == "cleanup script packages",
          "effect failed model skips XML and cleans scripts");
    Runtime no_scene;
    no_scene.model_result = 0u;
    (void)pal4::GobEffect::InitializeSceneAndConfig(
        Runtime::object, 2u, "scene/FX", 0x3000u,
        0x4000u, no_scene);
    Check(std::find(no_scene.calls.begin(), no_scene.calls.end(),
                    "load scene package") == no_scene.calls.end(),
          "effect missing scene package is skipped");
}

void CheckConfigAndTransform() {
    Runtime missing;
    missing.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobEffect::GetConfigNode(Runtime::object, missing) == 0u &&
              missing.calls.empty(), "effect null document");
    missing.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobEffect::GetConfigNode(Runtime::object, missing) == 0u,
          "effect missing parameter node");
    missing.calls.clear();
    pal4::GobEffect::ReadConfig(Runtime::object, missing);
    Check(missing.calls.front() == "apply base parameters" &&
              missing.calls.size() == 2u,
          "effect missing node only applies base parameters");

    Runtime config;
    config.words[Runtime::object + 0x1c0u] = 0x6000u;
    config.xml_children[{0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    config.xml_children[{0x7000u, "PAL4_GameObject-effect"}] = 0x8000u;
    Check(pal4::GobEffect::GetConfigNode(Runtime::object, config) == 0x8000u,
          "effect config node path");
    config.calls.clear();
    pal4::GobEffect::ReadConfig(Runtime::object, config);
    Check(config.effect_name == "flame" &&
              config.words.at(Runtime::object + 0x1f0u) == 0u &&
              config.words.at(Runtime::object + 0x1f4u) == 0u &&
              config.words.at(Runtime::object + 0x1f8u) == 1u &&
              config.calls.front() == "apply base parameters" &&
              IndexOf(config.calls, "read XML string") <
                  IndexOf(config.calls, "assign effect name"),
          "effect missing attribute defaults and name");
    config.xml_attributes["PAL4_GameObject-effect-times"] = 1u;
    config.xml_attributes["PAL4_GameObject-effect-saver"] = 2u;
    config.xml_attributes["PAL4_GameObject-effect-distance-clip"] = 3u;
    config.xml_values = {{1u, -2}, {2u, 7}, {3u, 0}};
    config.calls.clear();
    pal4::GobEffect::ReadConfig(Runtime::object, config);
    Check(config.words.at(Runtime::object + 0x1f0u) ==
              static_cast<std::uint32_t>(-2) &&
              config.words.at(Runtime::object + 0x1f4u) == 7u &&
              config.words.at(Runtime::object + 0x1f8u) == 0u &&
              std::count(config.calls.begin(), config.calls.end(),
                         "parse XML integer") == 3,
          "effect explicit signed config values");
    Check(pal4::GobEffect::UpdateClumpTransform(
              Runtime::object, 0x2000u, 3u, config) == 0x4321u &&
              config.calls.back() == "update clump transform",
          "effect transform thunk");
}

void CheckVisibilityAndEffect() {
    Runtime hidden;
    Check(pal4::GobEffect::SetVisibleAndCreateEffect(
              Runtime::object, 0x8000u, 0u, hidden) == 0u &&
              hidden.words.at(Runtime::object + 0x184u) == 0u &&
              hidden.calls == (std::vector<std::string>{
                  "write word", "effect needs update"}),
          "hidden effect without active handle");
    Runtime existing;
    existing.needs_update = true;
    Check(pal4::GobEffect::SetVisibleAndCreateEffect(
              Runtime::object, 0x8000u, 0u, existing) == 0x3cu &&
              existing.calls.back() == "update dialog content",
          "hidden effect updates active dialog");

    Runtime visible;
    visible.effect_name = "flame";
    visible.words[Runtime::object + 0x1f0u] = 3u;
    visible.words[Runtime::object + 0x1f8u] = 1u;
    visible.needs_update = true;
    Check(pal4::GobEffect::SetVisibleAndCreateEffect(
              Runtime::object, 0x8000u, 1u, visible) == 0u &&
              visible.words.at(Runtime::object + 0x184u) == 1u &&
              visible.words.at(Runtime::object +
                  pal4::GobEffect::kEffectHandleOffset) == 0xaaaau &&
              visible.words.at(Runtime::object +
                  pal4::GobEffect::kEffectHandleOffset + 4u) == 0xbbbbu &&
              visible.created_name == "flame" &&
              visible.created_times == 3u &&
              visible.created_distance_clip &&
              IndexOf(visible.calls, "handle effect event") <
                  IndexOf(visible.calls, "create effect") &&
              IndexOf(visible.calls, "create effect") <
                  IndexOf(visible.calls, "reset temporary effect"),
          "visible effect replaces active handle and releases temporary");
    Runtime created;
    created.effect_name = "spark";
    created.words[Runtime::object + 0x1f0u] = 2u;
    created.words[Runtime::object + 0x1f8u] = 0u;
    created.words[Runtime::object + 0x14u] = 0x3000u;
    created.words[0x3004u] = 0x7777u;
    created.created_needs_update = true;
    Check(pal4::GobEffect::SetVisibleAndCreateEffect(
              Runtime::object, 0x8000u, 2u, created) == 0x5au &&
              created.created_name == "spark" &&
              !created.created_distance_clip &&
              IndexOf(created.calls, "reset temporary effect") <
                  IndexOf(created.calls, "update effect properties") &&
              created.calls.back() == "update effect state",
          "visible effect updates created handle from clump atomic");
}

void CheckSnapshot() {
    Runtime disabled;
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobEffect::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls.empty(), "effect disabled snapshot store");
    disabled.words[Runtime::object + 0x1a4u] = 1u;
    disabled.snapshot_allocation = 0u;
    Check(pal4::GobEffect::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls == (std::vector<std::string>{"allocate"}),
          "effect snapshot allocation failure");
    Runtime created;
    created.words[Runtime::object + 0x1a4u] = 1u;
    created.words[Runtime::object + 4u] = 0x4000u;
    created.words[Runtime::object + 0x184u] = 2u;
    Check(pal4::GobEffect::CreateStateSnapshot(
              Runtime::object, created) == 0x2000u &&
              created.snapshot_name == "Torch" &&
              created.words.at(0x2014u) == 2u &&
              created.calls == (std::vector<std::string>{
                  "allocate", "construct snapshot", "read object name",
                  "assign snapshot name", "write word"}),
          "effect snapshot captures name and visibility");
    Runtime failed;
    failed.words[Runtime::object + 0x1a4u] = 1u;
    failed.fail_snapshot_construct = true;
    bool thrown = false;
    try {
        (void)pal4::GobEffect::CreateStateSnapshot(
            Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct snapshot", "free"}),
          "effect snapshot construction failure frees allocation");
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    pal4::GobEffect::ApplyStateSnapshot(
        Runtime::object, 0x2000u, disabled);
    Check(disabled.calls.size() == 1u,
          "effect disabled snapshot restore");
    Runtime wrong;
    wrong.words[Runtime::object + 0x1a4u] = 1u;
    wrong.snapshot_type = 4u;
    pal4::GobEffect::ApplyStateSnapshot(
        Runtime::object, 0x2000u, wrong);
    Check(wrong.calls == (std::vector<std::string>{"snapshot type"}),
          "effect snapshot type gate");
    created.calls.clear();
    pal4::GobEffect::ApplyStateSnapshot(
        Runtime::object, 0x2000u, created);
    Check(created.calls == (std::vector<std::string>{
              "snapshot type", "set base visible"}),
          "effect type-eight snapshot restores visibility");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckRuntimeGates();
        CheckSceneInitialization();
        CheckConfigAndTransform();
        CheckVisibilityAndEffect();
        CheckSnapshot();
        std::cout << "gob_effect_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
