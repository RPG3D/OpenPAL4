#include "pal4/gob_get_item.h"

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

struct Runtime final : pal4::IGobGetItemRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::pair<std::uint32_t, std::string>,
             std::uint32_t> xml_children;
    std::vector<std::string> calls;
    std::map<std::string, std::int32_t> config_values;
    std::uint32_t allocation = object;
    std::uint32_t model_result = 0xabcdu;
    std::uint32_t scene_package = 0u;
    bool fail_base = false;
    std::uint32_t hint_window = 0x9000u;
    std::uint32_t snapshot_allocation = 0x2000u;
    std::uint32_t snapshot_type = 7u;
    bool fail_snapshot_construct = false;
    std::string snapshot_name;
    bool property_exists = true;
    bool equipment_exists = true;
    std::string shown_hint;
    std::string error_message;
    std::int32_t granted_id = 0;
    std::int32_t granted_quantity = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobGetItem::kSize || size == 0x1cu,
              "get-item allocation size");
        calls.emplace_back("allocate");
        return size == 0x1cu ? snapshot_allocation : allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == object || memory == 0x2000u,
              "get-item free allocation");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "get-item base constructor");
        calls.emplace_back("construct base");
        if (fail_base) throw std::runtime_error("base failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "get-item base destructor");
        calls.emplace_back("destruct base");
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "get-item pre-load object");
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
              "get-item scene package");
        calls.emplace_back("load scene package");
    }
    std::uint32_t LoadBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && !path.empty() &&
                  fourth == 0x3000u && fifth == 0x4000u,
              "get-item base model arguments");
        calls.emplace_back("load base model");
        return model_result;
    }
    void HideHookUserDataAtomics(std::uint32_t clump) override {
        Check(clump == 0x5000u, "get-item hook clump");
        calls.emplace_back("hide hook atomics");
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
              "get-item old XML node");
        calls.emplace_back("remove XML child");
    }
    std::uint32_t CreateXmlChild(std::uint32_t parent,
                                  std::string_view name) override {
        Check(parent == 0x7000u &&
                  name == "PAL4_GameObject-getItem",
              "get-item new XML node");
        calls.emplace_back("create XML child");
        return 0x9000u;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup script packages");
    }
    std::uint32_t CreateHintWindow() override {
        calls.emplace_back("create hint window");
        return hint_window;
    }
    std::int32_t ReadXmlInt(std::uint32_t node,
                            std::string_view key,
                            std::int32_t fallback) override {
        Check(node == 0x8000u, "get-item XML attribute node");
        Check((key == "PAL4_GameObject-getItem-PropID" &&
                  fallback == 0) ||
                  (key == "PAL4_GameObject-getItem-EquipID" &&
                   fallback == 0) ||
                  (key == "PAL4_GameObject-getItem-num" &&
                   fallback == 1),
              "get-item XML attribute keys and defaults");
        calls.emplace_back(std::string(key));
        const auto found = config_values.find(std::string(key));
        return found == config_values.end() ? fallback : found->second;
    }
    bool PropertyExists(std::int32_t id) override {
        Check(id > 0, "get-item property lookup id");
        calls.emplace_back("property exists");
        return property_exists;
    }
    bool EquipmentExists(std::int32_t id) override {
        Check(id > 0, "get-item equipment lookup id");
        calls.emplace_back("equipment exists");
        return equipment_exists;
    }
    void GrantProperty(std::int32_t id,
                       std::int32_t quantity) override {
        granted_id = id;
        granted_quantity = quantity;
        calls.emplace_back("grant property");
    }
    void GrantEquipment(std::int32_t id) override {
        granted_id = id;
        calls.emplace_back("grant equipment");
    }
    std::string PropertyName(std::int32_t id) override {
        Check(id == granted_id, "get-item property name lookup");
        calls.emplace_back("property name");
        return "Potion";
    }
    std::string EquipmentName(std::int32_t id) override {
        Check(id == granted_id, "get-item equipment name lookup");
        calls.emplace_back("equipment name");
        return "Sword";
    }
    std::string GameString(std::uint32_t id) override {
        Check(id == 0xfbu, "get-item hint format string id");
        calls.emplace_back("game string");
        return "%s x%d";
    }
    std::string FormatHint(std::string_view format,
                           std::string_view name,
                           std::int32_t quantity) override {
        Check(format == "%s x%d", "get-item hint format");
        calls.emplace_back("format hint");
        return std::string(name) + " x" + std::to_string(quantity);
    }
    void ShowHint(std::uint32_t window,
                  std::string_view text, float x,
                  float y, float scale, std::uint32_t mode,
                  std::uint32_t visible) override {
        Check(window == 0x9000u && x == 0.5f && y == 0.15f &&
                  scale == 1.0f && mode == 0u && visible == 1u,
              "get-item hint window arguments");
        shown_hint = text;
        calls.emplace_back("show hint");
    }
    void PlayHintSound(std::string_view path,
                       std::uint32_t mode) override {
        Check(path == "gamedata\\PALSound\\WA06.wav" && mode == 1u,
              "get-item acquired sound path and mode");
        calls.emplace_back("play hint sound");
    }
    void ShowIdError(std::string_view message,
                     std::string_view title) override {
        Check(title == "IDError", "get-item ID error title");
        error_message = message;
        calls.emplace_back("show ID error");
    }
    std::uint32_t ConsumeResearchUse(
        std::uint32_t address) override {
        Check(address == object, "get-item use consumption object");
        calls.emplace_back("consume research use");
        return 0x7777u;
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x4000u, "get-item object name address");
        calls.emplace_back("read object name");
        return "Chest";
    }
    void ConstructSnapshot(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "get-item snapshot constructor");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot_construct)
            throw std::runtime_error("snapshot construction failed");
    }
    void AssignSnapshotName(std::uint32_t snapshot,
                            std::string_view name) override {
        Check(snapshot == 0x2000u, "get-item snapshot name target");
        snapshot_name = name;
        calls.emplace_back("assign snapshot name");
    }
    std::uint32_t SnapshotType(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "get-item snapshot type target");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
    std::uint32_t SetBaseVisible(
        std::uint32_t address, std::uint32_t visible) override {
        Check(address == object && visible <= 2u,
              "get-item snapshot visibility target");
        calls.emplace_back("set base visible");
        return 0x5555u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
};

void CheckLifecycle() {
    Check(pal4::GobGetItem::TypeKey() == 7u,
          "get-item factory type key");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobGetItem::Create(unavailable) == 0u &&
              unavailable.calls == (std::vector<std::string>{"allocate"}),
          "get-item allocation failure");
    Runtime created;
    Check(pal4::GobGetItem::Create(created) == Runtime::object &&
              created.words.at(Runtime::object) ==
                  pal4::GobGetItem::kVtable &&
              created.calls == (std::vector<std::string>{
                  "allocate", "construct base", "write word"}),
          "get-item constructor order and vtable");
    created.calls.clear();
    pal4::GobGetItem::Destruct(Runtime::object, created);
    Check(created.calls == (std::vector<std::string>{
              "write word", "destruct base"}),
          "get-item destructor order");
    Runtime failed;
    failed.fail_base = true;
    bool thrown = false;
    try {
        (void)pal4::GobGetItem::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct base", "free"}),
          "get-item failed base constructor frees allocation");
}

void CheckSceneInitialization() {
    Runtime object_package;
    object_package.words[Runtime::object + 0x14u] = 0x5000u;
    object_package.words[Runtime::object + 0x1c0u] = 0x6000u;
    object_package.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    object_package.xml_children[
        {0x7000u, "PAL4_GameObject-getItem"}] = 0x8000u;
    Check(pal4::GobGetItem::InitializeSceneAndConfig(
              Runtime::object, 2u, "palobject/item", 0x3000u,
              0x4000u, object_package) == 0xabcdu &&
              IndexOf(object_package.calls, "cleanup before scene load") <
                  IndexOf(object_package.calls, "load object package") &&
              IndexOf(object_package.calls, "load base model") <
                  IndexOf(object_package.calls, "hide hook atomics") &&
              IndexOf(object_package.calls, "remove XML child") <
                  IndexOf(object_package.calls, "create XML child") &&
              object_package.calls.back() == "cleanup script packages",
          "get-item object package and XML replacement");
    Runtime weapon;
    weapon.words[Runtime::object + 0x14u] = 0x5000u;
    weapon.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobGetItem::InitializeSceneAndConfig(
              Runtime::object, 2u, "palweapon/item", 0x3000u,
              0x4000u, weapon) == 0xabcdu &&
              IndexOf(weapon.calls, "load weapon package") <
                  IndexOf(weapon.calls, "load base model") &&
              std::find(weapon.calls.begin(), weapon.calls.end(),
                        "create XML child") == weapon.calls.end(),
          "get-item weapon package with missing parameters");
    Runtime scene;
    scene.scene_package = 0x4000u;
    scene.model_result = 0u;
    Check(pal4::GobGetItem::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/item", 0x3000u,
              0x4000u, scene) == 0u &&
              IndexOf(scene.calls, "load scene package") <
                  IndexOf(scene.calls, "load base model") &&
              std::find(scene.calls.begin(), scene.calls.end(),
                        "hide hook atomics") == scene.calls.end() &&
              scene.calls.back() == "cleanup script packages",
          "get-item failed model skips clump/XML and cleans scripts");
}

void ConfigureNode(Runtime& runtime) {
    runtime.words[Runtime::object + 0x198u] = 1u;
    runtime.words[Runtime::object + 0x1c0u] = 0x6000u;
    runtime.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    runtime.xml_children[
        {0x7000u, "PAL4_GameObject-getItem"}] = 0x8000u;
}

void CheckGrantAndHint() {
    Runtime no_uses;
    no_uses.words[Runtime::object + 0x198u] = 0u;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, no_uses) == 0u &&
              no_uses.calls.empty(), "get-item use-count gate");
    Runtime no_window;
    no_window.words[Runtime::object + 0x198u] = 1u;
    no_window.hint_window = 0u;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, no_window) == 0u &&
              no_window.calls == (std::vector<std::string>{
                  "create hint window"}),
          "get-item missing hint window preserves use");
    Runtime no_node;
    no_node.words[Runtime::object + 0x198u] = 1u;
    no_node.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, no_node) == 0u &&
              no_node.calls == (std::vector<std::string>{
                  "create hint window"}),
          "get-item missing config preserves use");

    Runtime property;
    ConfigureNode(property);
    property.config_values = {
        {"PAL4_GameObject-getItem-PropID", 21},
        {"PAL4_GameObject-getItem-EquipID", 5},
        {"PAL4_GameObject-getItem-num", 3}};
    Check(pal4::GobGetItem::GetConfigNode(
              Runtime::object, property) == 0x8000u,
          "get-item config node path");
    property.calls.clear();
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, property) == 0x7777u &&
              property.granted_id == 21 &&
              property.granted_quantity == 3 &&
              property.shown_hint == "Potion x3" &&
              IndexOf(property.calls, "grant property") <
                  IndexOf(property.calls, "show hint") &&
              IndexOf(property.calls, "show hint") <
                  IndexOf(property.calls, "play hint sound") &&
              property.calls.back() == "consume research use" &&
              std::find(property.calls.begin(), property.calls.end(),
                        "equipment exists") == property.calls.end(),
          "get-item property priority, quantity and hint/audio order");
    Runtime equipment;
    ConfigureNode(equipment);
    equipment.config_values[
        "PAL4_GameObject-getItem-EquipID"] = 5;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, equipment) == 0x7777u &&
              equipment.granted_id == 5 &&
              equipment.shown_hint == "Sword x1" &&
              IndexOf(equipment.calls, "grant equipment") <
                  IndexOf(equipment.calls, "show hint") &&
              equipment.calls.back() == "consume research use",
          "get-item equipment branch and default quantity");
    Runtime no_grant;
    ConfigureNode(no_grant);
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, no_grant) == 0x7777u &&
              no_grant.calls.back() == "consume research use" &&
              no_grant.shown_hint.empty(),
          "get-item empty IDs still consume use");
    Runtime bad_property;
    ConfigureNode(bad_property);
    bad_property.config_values[
        "PAL4_GameObject-getItem-PropID"] = 9;
    bad_property.property_exists = false;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, bad_property) == 0x7777u &&
              bad_property.error_message == "PropID not exist" &&
              bad_property.calls.back() == "consume research use",
          "get-item unknown property error and use consumption");
    Runtime bad_equipment;
    ConfigureNode(bad_equipment);
    bad_equipment.config_values[
        "PAL4_GameObject-getItem-EquipID"] = 7;
    bad_equipment.equipment_exists = false;
    Check(pal4::GobGetItem::TriggerGrantAndShowHint(
              Runtime::object, 0x3000u, bad_equipment) == 0x7777u &&
              bad_equipment.error_message == "EquipID not exist" &&
              bad_equipment.calls.back() == "consume research use",
          "get-item unknown equipment error and use consumption");
}

void CheckSnapshot() {
    Runtime disabled;
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobGetItem::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls.empty(), "get-item snapshot store gate");
    Runtime unavailable;
    unavailable.words[Runtime::object + 0x1a4u] = 1u;
    unavailable.snapshot_allocation = 0u;
    Check(pal4::GobGetItem::CreateStateSnapshot(
              Runtime::object, unavailable) == 0u &&
              unavailable.calls == (std::vector<std::string>{"allocate"}),
          "get-item snapshot allocation failure");
    Runtime created;
    created.words[Runtime::object + 0x1a4u] = 1u;
    created.words[Runtime::object + 4u] = 0x4000u;
    created.words[Runtime::object + 0x198u] = 2u;
    created.words[Runtime::object + 0x184u] = 1u;
    Check(pal4::GobGetItem::CreateStateSnapshot(
              Runtime::object, created) == 0x2000u &&
              created.snapshot_name == "Chest" &&
              created.words.at(0x2018u) == 2u &&
              created.words.at(0x2014u) == 1u &&
              created.calls == (std::vector<std::string>{
                  "allocate", "construct snapshot", "read object name",
                  "assign snapshot name", "write word", "write word"}),
          "get-item snapshot captures name, use count and visibility");
    Runtime exhausted;
    exhausted.words[Runtime::object + 0x1a4u] = 1u;
    exhausted.words[Runtime::object + 4u] = 0x4000u;
    exhausted.words[Runtime::object + 0x198u] = 0u;
    exhausted.words[Runtime::object + 0x184u] = 2u;
    exhausted.words[Runtime::object + 0x190u] = 1u;
    Check(pal4::GobGetItem::CreateStateSnapshot(
              Runtime::object, exhausted) == 0x2000u &&
              exhausted.words.at(0x2014u) == 0u,
          "exhausted get-item snapshot is hidden by base flag");
    Runtime no_hide;
    no_hide.words = exhausted.words;
    no_hide.words[Runtime::object + 0x190u] = 0u;
    Check(pal4::GobGetItem::CreateStateSnapshot(
              Runtime::object, no_hide) == 0x2000u &&
              no_hide.words.at(0x2014u) == 2u,
          "exhausted get-item without flag preserves visibility");
    Runtime failed;
    failed.words[Runtime::object + 0x1a4u] = 1u;
    failed.fail_snapshot_construct = true;
    bool thrown = false;
    try {
        (void)pal4::GobGetItem::CreateStateSnapshot(
            Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct snapshot", "free"}),
          "get-item snapshot constructor failure cleanup");
    Check(pal4::GobGetItem::ApplyStateSnapshot(
              Runtime::object, 0x2000u, disabled) == 0u &&
              disabled.calls.empty(), "get-item disabled snapshot restore");
    Runtime wrong;
    wrong.words[Runtime::object + 0x1a4u] = 1u;
    wrong.snapshot_type = 8u;
    Check(pal4::GobGetItem::ApplyStateSnapshot(
              Runtime::object, 0x2000u, wrong) == 8u &&
              wrong.calls == (std::vector<std::string>{"snapshot type"}),
          "get-item snapshot type gate");
    created.calls.clear();
    Check(pal4::GobGetItem::ApplyStateSnapshot(
              Runtime::object, 0x2000u, created) == 0x5555u &&
              created.words.at(Runtime::object + 0x198u) == 2u &&
              created.calls == (std::vector<std::string>{
                  "snapshot type", "write word", "set base visible"}),
          "get-item snapshot restores count before visibility");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckSceneInitialization();
        CheckGrantAndHint();
        CheckSnapshot();
        std::cout << "gob_get_item_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
