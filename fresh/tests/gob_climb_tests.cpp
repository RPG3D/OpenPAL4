#include "pal4/gob_climb.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

struct Runtime final : pal4::IGobClimbRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::pair<std::uint32_t, std::string>,
             std::uint32_t> xml_children;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    std::uint32_t snapshot_allocation = 0x2000u;
    std::uint32_t snapshot_type = 4u;
    bool fail_snapshot_construct = false;
    std::string snapshot_name;
    std::uint32_t applied_visible = 0xffffffffu;
    std::uint32_t model_result = 0xabcdu;
    std::uint32_t scene_package = 0u;
    bool fail_base_construction = false;
    std::map<std::string, std::uint32_t> attributes;
    std::map<std::uint32_t, float> attribute_values;
    std::vector<std::string> requested_attributes;
    Position last_near{};
    Position last_far{};
    float last_heading = 0.0f;
    std::uint32_t last_side = 0xffffffffu;
    float normalized_x = 0.0f;
    float normalized_z = 0.0f;
    float computed_heading = 30.0f;
    float interaction_threshold = 20.0f;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobClimb::kSize || size == 0x18u,
              "climb allocation size");
        calls.emplace_back("allocate");
        return size == 0x18u ? snapshot_allocation : allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == object || memory == 0x2000u,
              "climb freed allocation");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "climb base constructor object");
        calls.emplace_back("construct base");
        if (fail_base_construction)
            throw std::runtime_error("base constructor failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "climb base destructor object");
        calls.emplace_back("destruct base");
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "climb pre-load object");
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
              "climb scene package");
        calls.emplace_back("load scene package");
    }
    std::uint32_t LoadBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u &&
                  !path.empty() && fourth == 0x3000u &&
                  fifth == 0x4000u,
              "climb base model arguments");
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
              "climb old XML node removal");
        calls.emplace_back("remove XML child");
    }
    std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) override {
        Check(parent == 0x7000u &&
                  name == "PAL4_GameObject-climb",
              "climb XML child creation");
        calls.emplace_back("create XML child");
        return 0x9000u;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup script packages");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    std::uint32_t FindXmlAttribute(
        std::uint32_t node, std::string_view name) override {
        Check(node == 0x8000u, "climb XML attribute node");
        requested_attributes.emplace_back(name);
        calls.emplace_back("find XML attribute");
        const auto found = attributes.find(std::string(name));
        return found == attributes.end() ? 0u : found->second;
    }
    float XmlAttributeToFloat(std::uint32_t attribute,
                              float fallback) override {
        Check(fallback == 0.0f, "climb float default");
        calls.emplace_back("parse XML float");
        return attribute_values.at(attribute);
    }
    void NormalizeHorizontal(float& x, float& z) override {
        const auto length = std::sqrt(x * x + z * z);
        if (length != 0.0f) {
            x /= length;
            z /= length;
        }
        normalized_x = x;
        normalized_z = z;
        calls.emplace_back("normalize heading");
    }
    float HeadingDegrees(float x, float z) override {
        Check(x == normalized_x && z == normalized_z,
              "climb normalized heading input");
        calls.emplace_back("compute heading");
        return computed_heading;
    }
    float ClumpFacingDegrees(std::uint32_t address) override {
        Check(address == object, "climb clump facing object");
        calls.emplace_back("clump facing");
        return 77.0f;
    }
    std::uint32_t BeginActorMotion(
        const Position& near_end, const Position& far_end,
        float heading_degrees, std::uint32_t side) override {
        last_near = near_end;
        last_far = far_end;
        last_heading = heading_degrees;
        last_side = side;
        calls.emplace_back("begin actor motion");
        return 0x5555u;
    }
    float InteractionThreshold(std::uint32_t address) override {
        Check(address == object, "climb range threshold object");
        calls.emplace_back("interaction threshold");
        return interaction_threshold;
    }
    std::string ReadCString(std::uint32_t address) override {
        Check(address == 0x3000u, "climb object name pointer");
        calls.emplace_back("read object name");
        return "Ladder";
    }
    void ConstructSnapshot(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "climb snapshot constructor");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot_construct)
            throw std::runtime_error("snapshot construction failed");
    }
    void AssignSnapshotName(std::uint32_t snapshot,
                            std::string_view name) override {
        Check(snapshot == 0x2000u, "climb snapshot name target");
        snapshot_name = name;
        calls.emplace_back("assign snapshot name");
    }
    std::uint32_t SnapshotType(std::uint32_t snapshot) override {
        Check(snapshot == 0x2000u, "climb snapshot type target");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
    void SetBaseVisible(std::uint32_t address,
                        std::uint32_t visible) override {
        Check(address == object, "climb base visibility object");
        applied_visible = visible;
        calls.emplace_back("set base visible");
    }
};

void CheckLifecycle() {
    Check(pal4::GobClimb::TypeKey() == 4u,
          "climb factory type key");
    Runtime unavailable;
    unavailable.allocation = 0u;
    Check(pal4::GobClimb::Create(unavailable) == 0u &&
              unavailable.calls ==
                  (std::vector<std::string>{"allocate"}),
          "climb allocation failure");
    Runtime created;
    Check(pal4::GobClimb::Create(created) == Runtime::object &&
              created.words.at(Runtime::object) ==
                  pal4::GobClimb::kVtable &&
              created.calls ==
                  (std::vector<std::string>{
                      "allocate", "construct base", "write word"}),
          "climb constructor and vtable");
    created.calls.clear();
    pal4::GobClimb::Destruct(Runtime::object, created);
    Check(created.calls ==
              (std::vector<std::string>{
                  "write word", "destruct base"}),
          "climb destructor order");
    Runtime failed;
    failed.fail_base_construction = true;
    bool thrown = false;
    try {
        (void)pal4::GobClimb::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct base", "free"}),
          "climb exceptional construction frees allocation");
}

void CheckSceneInitialization() {
    Runtime object_package;
    object_package.words[Runtime::object + 0x1c0u] = 0x6000u;
    object_package.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    object_package.xml_children[
        {0x7000u, "PAL4_GameObject-climb"}] = 0x8000u;
    Check(pal4::GobClimb::InitializeSceneAndConfig(
              Runtime::object, 2u, "palobject/climb", 0x3000u,
              0x4000u, object_package) == 0xabcdu &&
              IndexOf(object_package.calls, "cleanup before load") <
                  IndexOf(object_package.calls, "load object package") &&
              IndexOf(object_package.calls, "load object package") <
                  IndexOf(object_package.calls, "load base model") &&
              IndexOf(object_package.calls, "remove XML child") <
                  IndexOf(object_package.calls, "create XML child") &&
              object_package.calls.back() == "cleanup script packages",
          "climb object package and XML replacement order");
    Runtime weapon;
    weapon.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobClimb::InitializeSceneAndConfig(
              Runtime::object, 2u, "palweapon/climb", 0x3000u,
              0x4000u, weapon) == 0xabcdu &&
              IndexOf(weapon.calls, "load weapon package") <
                  IndexOf(weapon.calls, "load base model") &&
              std::find(weapon.calls.begin(), weapon.calls.end(),
                        "create XML child") == weapon.calls.end(),
          "climb weapon package and missing XML parent");
    Runtime scene;
    scene.scene_package = 0x5000u;
    scene.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobClimb::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/climb", 0x3000u,
              0x4000u, scene) == 0xabcdu &&
              IndexOf(scene.calls, "load scene package") <
                  IndexOf(scene.calls, "load base model"),
          "climb current scene package selection");
    Runtime failed_model;
    failed_model.model_result = 0u;
    failed_model.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobClimb::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/climb", 0x3000u,
              0x4000u, failed_model) == 0u &&
              failed_model.calls.back() == "cleanup script packages" &&
              std::find(failed_model.calls.begin(), failed_model.calls.end(),
                        "find XML child") == failed_model.calls.end(),
          "climb failed model skips XML and cleans scripts");
    Runtime no_document;
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobClimb::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/climb", 0x3000u,
              0x4000u, no_document) == 0u &&
              no_document.calls.back() == "cleanup script packages",
          "climb successful model without document returns zero");
}

void CheckActorMotion() {
    using Position = pal4::GobClimb::Position;
    Runtime absent;
    absent.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobClimb::GetConfigNode(
              Runtime::object, absent) == 0u &&
              pal4::GobClimb::BeginMotionFromConfig(
                  Runtime::object, Position{}, absent) == 0u &&
              absent.calls.empty(),
          "climb missing XML has no motion");

    const std::vector<std::string> keys = {
        "PAL4_GameObject-climb-posXA",
        "PAL4_GameObject-climb-posYA",
        "PAL4_GameObject-climb-posZA",
        "PAL4_GameObject-climb-posXB",
        "PAL4_GameObject-climb-posYB",
        "PAL4_GameObject-climb-posZB"};
    Runtime from_a;
    from_a.words[Runtime::object + 0x1c0u] = 0x6000u;
    from_a.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    from_a.xml_children[
        {0x7000u, "PAL4_GameObject-climb"}] = 0x8000u;
    for (std::size_t i = 0; i < keys.size(); ++i)
        from_a.attributes[keys[i]] =
            static_cast<std::uint32_t>(i + 1u);
    from_a.attribute_values = {
        {1u, 0.0f}, {2u, 0.0f}, {3u, 0.0f},
        {4u, 0.0f}, {5u, 4.0f}, {6u, 3.0f}};
    Check(pal4::GobClimb::GetConfigNode(
              Runtime::object, from_a) == 0x8000u &&
              pal4::GobClimb::BeginMotionFromConfig(
                  Runtime::object, Position{0.0f, 0.0f, 0.0f},
                  from_a) == 0x5555u &&
              from_a.requested_attributes == keys &&
              from_a.normalized_x == 0.0f &&
              from_a.normalized_z == 1.0f &&
              from_a.last_near.y == 0.0f &&
              from_a.last_far.y == 4.0f &&
              from_a.last_side == 1u &&
              from_a.last_heading == 30.0f &&
              IndexOf(from_a.calls, "normalize heading") <
                  IndexOf(from_a.calls, "begin actor motion"),
          "climb ascending heading and nearer A endpoint");

    Runtime from_b;
    from_b.words = from_a.words;
    from_b.xml_children = from_a.xml_children;
    from_b.attributes = from_a.attributes;
    from_b.attribute_values = from_a.attribute_values;
    Check(pal4::GobClimb::BeginMotionFromConfig(
              Runtime::object, Position{0.0f, 4.0f, 3.0f},
              from_b) == 0x5555u &&
              from_b.last_near.y == 4.0f &&
              from_b.last_far.y == 0.0f &&
              from_b.last_side == 0u &&
              from_b.normalized_z == 1.0f,
          "climb nearer B reverses endpoint order");

    Runtime downhill;
    downhill.words = from_a.words;
    downhill.xml_children = from_a.xml_children;
    downhill.attributes = from_a.attributes;
    downhill.attribute_values = {
        {1u, 0.0f}, {2u, 5.0f}, {3u, 0.0f},
        {4u, 4.0f}, {5u, 0.0f}, {6u, 0.0f}};
    Check(pal4::GobClimb::BeginMotionFromConfig(
              Runtime::object, Position{4.0f, 0.0f, 0.0f},
              downhill) == 0x5555u &&
              downhill.normalized_x == -1.0f &&
              downhill.last_side == 0u,
          "climb heading points toward higher endpoint");

    Runtime defaults;
    defaults.words = from_a.words;
    defaults.xml_children = from_a.xml_children;
    defaults.computed_heading = 0.001f;
    Check(pal4::GobClimb::BeginMotionFromConfig(
              Runtime::object, Position{}, defaults) == 0x5555u &&
              defaults.requested_attributes == keys &&
              defaults.last_heading == 77.0f &&
              defaults.last_side == 1u &&
              IndexOf(defaults.calls, "clump facing") <
                  IndexOf(defaults.calls, "begin actor motion"),
          "climb zero coordinates and near-zero heading fallback");
}

void CheckEndpointRange() {
    using Position = pal4::GobClimb::Position;
    Runtime absent;
    absent.words[Runtime::object + 0x1c0u] = 0u;
    float distance = -1.0f;
    Check(!pal4::GobClimb::IsWithinEndpointRange(
              Runtime::object, Position{}, &distance, absent) &&
              distance == -1.0f && absent.calls.empty(),
          "climb absent endpoint config leaves output untouched");

    Runtime runtime;
    runtime.words[Runtime::object + 0x1c0u] = 0x6000u;
    runtime.xml_children[
        {0x6000u, "PAL4-GameObject-Parameters"}] = 0x7000u;
    runtime.xml_children[
        {0x7000u, "PAL4_GameObject-climb"}] = 0x8000u;
    const std::vector<std::string> keys = {
        "PAL4_GameObject-climb-posXA",
        "PAL4_GameObject-climb-posYA",
        "PAL4_GameObject-climb-posZA",
        "PAL4_GameObject-climb-posXB",
        "PAL4_GameObject-climb-posYB",
        "PAL4_GameObject-climb-posZB"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        runtime.attributes[keys[i]] =
            static_cast<std::uint32_t>(i + 1u);
    runtime.attribute_values = {
        {1u, 0.0f}, {2u, 0.0f}, {3u, 0.0f},
        {4u, 10.0f}, {5u, 100.0f}, {6u, 0.0f}};
    runtime.interaction_threshold = 100.0f;
    Check(pal4::GobClimb::IsWithinEndpointRange(
              Runtime::object, Position{0.0f, 80.0f, 0.0f},
              &distance, runtime) &&
              distance == 100.0f &&
              runtime.requested_attributes == keys &&
              runtime.calls.back() == "interaction threshold",
          "climb picks nearest endpoint in 3D then compares horizontal squared range");
    runtime.interaction_threshold = 99.0f;
    Check(!pal4::GobClimb::IsWithinEndpointRange(
              Runtime::object, Position{0.0f, 80.0f, 0.0f},
              &distance, runtime) && distance == 100.0f,
          "climb range threshold rejects squared distance above limit");
    runtime.interaction_threshold =
        std::numeric_limits<float>::quiet_NaN();
    Check(!pal4::GobClimb::IsWithinEndpointRange(
              Runtime::object, Position{0.0f, 80.0f, 0.0f},
              nullptr, runtime),
          "climb unordered threshold rejects");

    Runtime tie;
    tie.words = runtime.words;
    tie.xml_children = runtime.xml_children;
    tie.attributes = runtime.attributes;
    tie.attribute_values = {
        {1u, 0.0f}, {2u, 0.0f}, {3u, 0.0f},
        {4u, 10.0f}, {5u, 10.0f}, {6u, 0.0f}};
    tie.interaction_threshold = 0.0f;
    Check(pal4::GobClimb::IsWithinEndpointRange(
              Runtime::object, Position{0.0f, 10.0f, 0.0f},
              &distance, tie) && distance == 0.0f,
          "climb equal 3D distances prefer endpoint A");
}

void CheckStateSnapshot() {
    Runtime disabled;
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobClimb::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls.empty(),
          "climb disabled store skips snapshot allocation");
    Runtime allocation_failed;
    allocation_failed.words[Runtime::object + 0x1a4u] = 1u;
    allocation_failed.snapshot_allocation = 0u;
    Check(pal4::GobClimb::CreateStateSnapshot(
              Runtime::object, allocation_failed) == 0u &&
              allocation_failed.calls ==
                  (std::vector<std::string>{"allocate"}),
          "climb snapshot allocation failure");
    Runtime created;
    created.words[Runtime::object + 0x1a4u] = 1u;
    created.words[Runtime::object + 4u] = 0x3000u;
    created.words[Runtime::object + 0x184u] = 7u;
    Check(pal4::GobClimb::CreateStateSnapshot(
              Runtime::object, created) == 0x2000u &&
              created.snapshot_name == "Ladder" &&
              created.words.at(0x2014u) == 7u &&
              created.calls ==
                  (std::vector<std::string>{
                      "allocate", "construct snapshot", "read object name",
                      "assign snapshot name", "write word"}),
          "climb snapshot captures name and visibility");
    Runtime construction_failed;
    construction_failed.words[Runtime::object + 0x1a4u] = 1u;
    construction_failed.fail_snapshot_construct = true;
    bool thrown = false;
    try {
        (void)pal4::GobClimb::CreateStateSnapshot(
            Runtime::object, construction_failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && construction_failed.calls ==
              (std::vector<std::string>{
                  "allocate", "construct snapshot", "free"}),
          "climb snapshot construction failure frees allocation");

    pal4::GobClimb::ApplyStateSnapshot(
        Runtime::object, 0x2000u, disabled);
    Check(disabled.calls.empty(), "climb disabled store skips restore");
    Runtime wrong_type;
    wrong_type.words[Runtime::object + 0x1a4u] = 1u;
    wrong_type.snapshot_type = 6u;
    pal4::GobClimb::ApplyStateSnapshot(
        Runtime::object, 0x2000u, wrong_type);
    Check(wrong_type.calls ==
              (std::vector<std::string>{"snapshot type"}),
          "climb snapshot type gate");
    Runtime restored;
    restored.words[Runtime::object + 0x1a4u] = 1u;
    restored.words[0x2014u] = 0u;
    pal4::GobClimb::ApplyStateSnapshot(
        Runtime::object, 0x2000u, restored);
    Check(restored.applied_visible == 0u &&
              restored.calls ==
                  (std::vector<std::string>{
                      "snapshot type", "set base visible"}),
          "climb type-four snapshot restores visibility");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckSceneInitialization();
        CheckActorMotion();
        CheckEndpointRange();
        CheckStateSnapshot();
        std::cout << "gob_climb_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
