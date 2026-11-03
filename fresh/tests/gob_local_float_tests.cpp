#include "pal4/gob_local_float.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <numbers>
#include <span>
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

struct Runtime final : pal4::IGobLocalFloatRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, float>> float_writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> render_states;
    std::vector<std::vector<DebugVertex>> transformed;
    std::vector<std::uint32_t> transform_modes;
    std::vector<float> circle_angles;
    std::vector<std::uint32_t> xml_type_reads;
    std::vector<std::string> xml_float_keys;
    std::vector<std::uint32_t> xml_children;
    std::map<std::uint32_t, std::int32_t> xml_types;
    std::map<std::pair<std::uint32_t, std::string>, float> xml_floats;
    std::uint32_t config_document = 0x8000u;
    std::uint32_t machine_init_result = 0xabcdu;
    std::uint32_t scene_package = 0u;
    bool tasks_present = true;
    bool parameters_present = true;
    bool old_float_present = true;
    bool task_containers_ready = false;
    std::int32_t config_auto_touch = 0;
    std::int32_t config_touch_type = 1;
    std::uint32_t path_allocation = 0x3000u;
    std::uint32_t snapshot_allocation = 0x4000u;
    std::uint32_t snapshot_type = 2u;
    bool fail_snapshot_construct = false;
    std::string snapshot_name;
    bool ring_transform_succeeds = true;
    bool segment_transform_succeeds = true;
    std::uint32_t allocation = object;
    bool fail_machine = false;
    pal4::IGobLocalFloatRuntime::Vector3 after_position{0.0f, 0.0f, 0.0f};
    pal4::IGobLocalFloatRuntime::Vector3 stored_delta{0.0f, 0.0f, 0.0f};
    float after_angle = 0.0f;
    float stored_rotation = 0.0f;
    float query_axis_y = 1.0f;
    float query_angle = 4.0f;
    pal4::IGobLocalFloatRuntime::Vector3 actor_position{0.0f, 0.0f, 0.0f};
    pal4::IGobLocalFloatRuntime::Vector3 actor_transform{0.0f, 0.0f, 0.0f};

    static void CheckVector(
        const pal4::IGobLocalFloatRuntime::Vector3& value,
        float x, float y, float z, const char* message) {
        Check(value.x == x && value.y == y && value.z == z, message);
    }

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobLocalFloat::kSize || size == 0x34u ||
                  (size != 0u && size % 12u == 0u),
              "local float allocation size");
        calls.emplace_back("allocate");
        return size == pal4::GobLocalFloat::kSize ? allocation :
            size == 0x34u ? snapshot_allocation : path_allocation;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == object || memory == 0x4000u || memory == 0x2000u,
              "local float allocation cleanup");
        calls.emplace_back("free");
    }
    void ConstructMachine(std::uint32_t address) override {
        Check(address == object, "local float machine constructor");
        calls.emplace_back("construct machine");
        if (fail_machine) throw std::runtime_error("machine failed");
    }
    void DestructMachine(std::uint32_t address) override {
        Check(address == object, "local float machine destructor");
        calls.emplace_back("destruct machine");
    }
    void CleanupMachineRuntime(std::uint32_t address,
                               std::uint32_t context) override {
        Check(address == object && context == 0x8000u,
              "local float base cleanup arguments");
        calls.emplace_back("cleanup machine");
    }
    std::uint32_t FreePathBuffer(std::uint32_t memory) override {
        Check(memory == 0x3000u, "local float path buffer");
        calls.emplace_back("free path buffer");
        return 0x7777u;
    }
    std::uint32_t ForwardHandleEvent(std::uint32_t address,
                                      std::uint32_t argument) override {
        Check(address == object && argument == 9u,
              "local float event forwarding");
        calls.emplace_back("forward event");
        return 0x5555u;
    }
    void UpdateMachineTasks(std::uint32_t address,
                            float elapsed) override {
        Check(address == object && elapsed == 0.5f,
              "local float machine update arguments");
        calls.emplace_back("update machine tasks");
        floats[object + 0xacu] = after_position.x;
        floats[object + 0xb0u] = after_position.y;
        floats[object + 0xb4u] = after_position.z;
        floats[object + 0xa4u] = after_angle;
    }
    void StorePlatformDelta(const Vector3& translation,
                            float rotation) override {
        stored_delta = translation;
        stored_rotation = rotation;
        calls.emplace_back("store platform delta");
    }
    std::uint32_t ActorControlAccess() override {
        calls.emplace_back("actor control access");
        return 0x4000u;
    }
    void ApplyActorTranslation(std::uint32_t actor,
                               const Vector3& translation,
                               std::uint32_t mode) override {
        Check(actor == 0x5000u && mode == 1u,
              "local float actor translation arguments");
        CheckVector(translation, 2.0f, -3.0f, 4.0f,
                    "local float actor translation delta");
        calls.emplace_back("apply actor translation");
    }
    void MatrixTranslate(std::uint32_t matrix,
                         const Vector3& translation,
                         std::uint32_t mode) override {
        Check(matrix == 0x7010u && mode == 2u,
              "local float matrix translation arguments");
        if (calls.empty() || calls.back() == "apply actor translation")
            CheckVector(translation, -12.0f, -17.0f, -34.0f,
                        "local float negative pivot");
        else
            CheckVector(translation, 12.0f, 17.0f, 34.0f,
                        "local float positive pivot");
        calls.emplace_back("matrix translate");
    }
    void MatrixRotateY(std::uint32_t matrix, float angle,
                       std::uint32_t mode) override {
        Check(matrix == 0x7010u && angle == 6.0f && mode == 2u,
              "local float matrix rotation");
        calls.emplace_back("matrix rotate");
    }
    void FrameUpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x7000u, "local float frame update");
        calls.emplace_back("frame update");
    }
    void MatrixQueryRotation(std::uint32_t matrix,
                             float& axis_y,
                             float& angle) override {
        Check(matrix == 0x7010u, "local float matrix query");
        axis_y = query_axis_y;
        angle = query_angle;
        calls.emplace_back("matrix query");
    }
    void SetActorPosition(std::uint32_t actor,
                          const Vector3& position) override {
        Check(actor == 0x5000u, "local float actor position target");
        actor_position = position;
        calls.emplace_back("set actor position");
    }
    void SetActorTransform(std::uint32_t actor,
                           const Vector3& transform,
                           std::uint32_t mode) override {
        Check(actor == 0x5000u && mode == 1u,
              "local float actor transform target");
        actor_transform = transform;
        calls.emplace_back("set actor transform");
    }
    void UpdateMinimapGrid(std::uint32_t actor_position_address,
                           std::uint32_t actor_transform_address) override {
        Check(actor_position_address == 0x50acu &&
                  actor_transform_address == 0x50a0u,
              "local float minimap input offsets");
        calls.emplace_back("update minimap");
    }
    std::uint32_t UpdateFloorAttributes() override {
        calls.emplace_back("update floor attributes");
        return 0x8888u;
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        render_states.emplace_back(state, value);
        calls.emplace_back("render state");
    }
    void SampleCircle(float angle, float& x,
                      float& z) override {
        circle_angles.push_back(angle);
        x = std::sin(angle);
        z = std::cos(angle);
        calls.emplace_back("sample circle");
    }
    bool TransformDebugVertices(
        std::span<const DebugVertex> vertices,
        std::uint32_t mode) override {
        Check((vertices.size() == 21u && mode == 2u) ||
                  (vertices.size() == 2u && mode == 0u),
              "local float debug transform mode and size");
        transformed.emplace_back(vertices.begin(), vertices.end());
        transform_modes.push_back(mode);
        calls.emplace_back("transform vertices");
        return mode == 2u ? ring_transform_succeeds :
                            segment_transform_succeeds;
    }
    void RenderDebugPrimitive(std::uint32_t type) override {
        Check(type == 2u, "local float debug primitive type");
        calls.emplace_back("render primitive");
    }
    void EndDebugPrimitive() override {
        calls.emplace_back("end primitive");
    }
    std::uint32_t GetMachineConfigNode(
        std::uint32_t address) override {
        Check(address == object, "local float config object");
        calls.emplace_back("get machine config");
        return config_document;
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "local float pre-load object");
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
              "local float scene package");
        calls.emplace_back("load scene package");
    }
    std::uint32_t InitializeMachineAndConfig(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && !path.empty() &&
                  fourth == 0x3000u && fifth == 0x4000u,
              "local float machine initialization arguments");
        calls.emplace_back("initialize machine and config");
        return machine_init_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup script packages");
    }
    std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) override {
        if (name == "PAL4_GameObject-machine-tasks") {
            Check(parent == 0x8000u, "local float machine tasks XML key");
            calls.emplace_back("find machine tasks");
            return tasks_present ? 0x8100u : 0u;
        }
        if (name == "PAL4-GameObject-Parameters") {
            Check(parent == 0x6000u, "local float parameter XML parent");
            calls.emplace_back("find parameters");
            return parameters_present ? 0x7000u : 0u;
        }
        Check(parent == 0x7000u &&
                  name == "PAL4_GAMEOBJECT_float",
              "local float config XML key");
        calls.emplace_back("find float config");
        return old_float_present ? 0x9000u : 0u;
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == 0x7000u && child == 0x9000u,
              "local float old XML node");
        calls.emplace_back("remove float config");
    }
    std::uint32_t CreateXmlChild(std::uint32_t parent,
                                  std::string_view name) override {
        Check(parent == 0x7000u &&
                  name == "PAL4_GAMEOBJECT_float",
              "local float new XML node");
        calls.emplace_back("create float config");
        return 0x9100u;
    }
    std::uint32_t MovementAdapterRoot() override {
        calls.emplace_back("movement adapter root");
        return 0x5000u;
    }
    std::uint32_t MovementProperty(std::uint32_t address) override {
        Check(address == object, "local float movement property object");
        calls.emplace_back("movement property");
        return 0x9000u;
    }
    void SetClumpProperties(
        std::uint32_t clump_manager, std::uint32_t clump,
        std::uint32_t property, std::uint32_t first,
        std::uint32_t second, std::uint32_t value) override {
        Check(clump_manager == 0x5100u && clump == 0x6000u &&
                  property == 7u && first == 1u && second == 0u &&
                  value == 0x9000u,
              "local float clump property arguments");
        calls.emplace_back("set clump properties");
    }
    void SetClumpRenderCallback(
        std::uint32_t clump_manager,
        std::uint32_t clump) override {
        Check(clump_manager == 0x5100u && clump == 0x6000u,
              "local float clump callback arguments");
        calls.emplace_back("set clump render callback");
    }
    std::uint32_t AttachMovementHelper(
        std::uint32_t adapter, std::uint32_t helper) override {
        Check(adapter == 0x5200u && helper == object + 0x2ccu,
              "local float movement helper attach");
        calls.emplace_back("attach movement helper");
        return 0xaaaau;
    }
    std::uint32_t DetachMovementHelper(
        std::uint32_t adapter, std::uint32_t helper) override {
        Check(adapter == 0x5200u && helper == object + 0x2ccu,
              "local float movement helper detach");
        calls.emplace_back("detach movement helper");
        return 0xbbbbu;
    }
    std::uint32_t TryTriggerMachineTask(
        std::uint32_t address,
        std::uint32_t argument) override {
        Check(address == object && argument == 0x8000u,
              "local float machine trigger arguments");
        calls.emplace_back("trigger machine task");
        return 0xccccu;
    }
    std::uint32_t ResetTriggeredMachine(
        std::uint32_t address) override {
        Check(address == object, "local float machine reset object");
        calls.emplace_back("reset triggered machine");
        return 0xddddu;
    }
    std::uint8_t StopTaskSounds(std::uint32_t address) override {
        Check(address == object, "local float stop sounds object");
        calls.emplace_back("stop task sounds");
        return 0x5au;
    }
    std::uint8_t ResumeTaskSounds(std::uint32_t address) override {
        Check(address == object, "local float resume sounds object");
        calls.emplace_back("resume task sounds");
        return 0x3cu;
    }
    bool AreTaskContainersReady(std::uint32_t address) override {
        Check(address == object, "local float task readiness object");
        calls.emplace_back("task containers ready");
        return task_containers_ready;
    }
    void LoadMachineMainConfig(std::uint32_t address) override {
        Check(address == object, "local float machine config target");
        calls.emplace_back("load machine main config");
    }
    void ApplyMachineVisibleFlag(std::uint32_t address,
                                 std::uint32_t visible) override {
        Check(address == object && visible <= 2u,
              "local float machine visibility arguments");
        calls.emplace_back("apply machine visible");
    }
    std::uint32_t InvokeMachineTrigger(
        std::uint32_t address, const Vector3& value) override {
        Check(address == object, "local float machine trigger object");
        CheckVector(value, 0.0f, 0.0f, 0.0f,
                    "local float zero trigger vector");
        calls.emplace_back("invoke machine trigger");
        return 0xeeeeu;
    }
    std::string_view ReadObjectName(std::uint32_t address) override {
        Check(address == 0x9100u, "local float object name address");
        calls.emplace_back("read object name");
        return "Raft";
    }
    void ConstructSnapshot(std::uint32_t snapshot) override {
        Check(snapshot == 0x4000u, "local float snapshot constructor");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot_construct)
            throw std::runtime_error("snapshot construction failed");
    }
    void AssignSnapshotName(std::uint32_t snapshot,
                            std::string_view name) override {
        Check(snapshot == 0x4000u, "local float snapshot name target");
        snapshot_name = name;
        calls.emplace_back("assign snapshot name");
    }
    std::uint32_t SnapshotType(std::uint32_t snapshot) override {
        Check(snapshot == 0x4000u, "local float snapshot type target");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
    void SetBaseVisible(std::uint32_t address,
                        std::uint32_t visible) override {
        Check(address == object && visible == 3u,
              "local float snapshot visibility");
        calls.emplace_back("set base visible");
    }
    void SetBasePosition(std::uint32_t address,
                         const Vector3& position,
                         std::uint32_t mode) override {
        Check(address == object && mode == 0u,
              "local float snapshot position arguments");
        CheckVector(position, 1.0f, 2.0f, 3.0f,
                    "local float snapshot position");
        calls.emplace_back("set base position");
    }
    std::uint32_t SetBaseRotation(
        std::uint32_t address, const Vector3& rotation,
        std::uint32_t mode) override {
        Check(address == object && mode == 0u,
              "local float snapshot rotation arguments");
        CheckVector(rotation, 4.0f, 5.0f, 6.0f,
                    "local float snapshot rotation");
        calls.emplace_back("set base rotation");
        return 0x1234u;
    }
    std::int32_t XmlChildCount(std::uint32_t node) override {
        Check(node == 0x8100u, "local float machine task node");
        calls.emplace_back("XML child count");
        return static_cast<std::int32_t>(xml_children.size());
    }
    std::uint32_t XmlChildAt(std::uint32_t node,
                             std::int32_t index) override {
        Check(node == 0x8100u && index >= 0 &&
                  static_cast<std::size_t>(index) < xml_children.size(),
              "local float XML child index");
        calls.emplace_back("XML child at");
        return xml_children[static_cast<std::size_t>(index)];
    }
    std::int32_t ReadXmlInt(std::uint32_t node,
                            std::string_view key,
                            std::int32_t fallback) override {
        if (key == "PAL4_GOMTask_type") {
            Check(fallback == -1, "local float task type default");
            xml_type_reads.push_back(node);
            calls.emplace_back("read XML type");
            const auto found = xml_types.find(node);
            return found == xml_types.end() ? fallback : found->second;
        }
        Check(node == 0x9000u, "local float config attribute node");
        if (key == "PAL4_GAMEOBJECT_float_auto_touch") {
            Check(fallback == 0, "local float auto-touch default");
            calls.emplace_back("read auto touch");
            return config_auto_touch;
        }
        Check(key == "PAL4_GAMEOBJECT_float_touch_type" &&
                  fallback == 1, "local float touch type key/default");
        calls.emplace_back("read touch type");
        return config_touch_type;
    }
    float ReadXmlFloat(std::uint32_t node,
                       std::string_view key,
                       float fallback) override {
        Check(fallback == 0.0f, "local float XML coordinate default");
        xml_float_keys.emplace_back(key);
        calls.emplace_back("read XML float");
        const auto found = xml_floats.find({node, std::string(key)});
        return found == xml_floats.end() ? fallback : found->second;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        return floats.at(address);
    }
    void WriteFloat(std::uint32_t address, float value) override {
        floats[address] = value;
        float_writes.emplace_back(address, value);
        calls.emplace_back("write float");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
        calls.emplace_back("write word");
    }
};

void SetInitialMachine(Runtime& runtime) {
    runtime.floats[Runtime::object + 0xacu] = 10.0f;
    runtime.floats[Runtime::object + 0xb0u] = 20.0f;
    runtime.floats[Runtime::object + 0xb4u] = 30.0f;
    runtime.floats[Runtime::object + 0xa4u] = 5.0f;
    runtime.after_position = {12.0f, 17.0f, 34.0f};
    runtime.after_angle = 11.0f;
    runtime.words[Runtime::object + 0x2d0u] = 1u;
}

void CheckLifecycle() {
    Runtime missing;
    missing.allocation = 0u;
    Check(pal4::GobLocalFloat::Create(missing) == 0u &&
              missing.calls == (std::vector<std::string>{"allocate"}),
          "local float allocation failure");

    Runtime created;
    Check(pal4::GobLocalFloat::Create(created) == Runtime::object &&
              created.calls[0] == "allocate" &&
              created.calls[1] == "construct machine" &&
              created.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x12ccu, pal4::GobLocalFloat::kHelperBaseVtable},
                      {0x12d0u, 0u}, {0x12d4u, 0u},
                      {0x12dcu, 0u}, {0x12e0u, 0u},
                      {0x1000u, pal4::GobLocalFloat::kVtable},
                      {0x11dcu, pal4::GobLocalFloat::kMachineMemberVtable},
                      {0x12ccu, pal4::GobLocalFloat::kHelperVtable},
                      {0x12d8u, 2u}}),
          "local float constructor field order and vtables");
    Check(pal4::GobLocalFloat::PrimaryTaskContainer(
              Runtime::object) == 0x1258u &&
              pal4::GobLocalFloat::InteractionThreshold() == 15.0,
          "local float task container and threshold");
    created.calls.clear();
    created.writes.clear();
    pal4::GobLocalFloat::Destruct(Runtime::object, created);
    Check(created.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x1000u, pal4::GobLocalFloat::kVtable},
                  {0x11dcu, pal4::GobLocalFloat::kMachineMemberVtable},
                  {0x12ccu, pal4::GobLocalFloat::kHelperVtable},
                  {0x12ccu, pal4::GobLocalFloat::kHelperBaseVtable}}) &&
              created.calls.back() == "destruct machine",
          "local float destructor restores base helper before machine");
    Runtime failed;
    failed.fail_machine = true;
    bool thrown = false;
    try {
        (void)pal4::GobLocalFloat::Create(failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct machine", "free"}),
          "local float construction failure releases allocation");
}

void CheckCleanupAndForwarding() {
    Runtime no_buffer;
    no_buffer.words[Runtime::object + 0x2dcu] = 0u;
    Check(pal4::GobLocalFloat::CleanupAndReset(
              Runtime::object, 0x8000u, no_buffer) == 0u &&
              no_buffer.words.at(Runtime::object + 0x2d0u) == 0u &&
              no_buffer.words.at(Runtime::object + 0x2e0u) == 0u &&
              no_buffer.calls == (std::vector<std::string>{
                  "cleanup machine", "write word", "write word"}),
          "local float cleanup without path buffer");
    Runtime with_buffer;
    with_buffer.words[Runtime::object + 0x2dcu] = 0x3000u;
    Check(pal4::GobLocalFloat::CleanupAndReset(
              Runtime::object, 0x8000u, with_buffer) == 0x7777u &&
              with_buffer.words.at(Runtime::object + 0x2dcu) == 0u &&
              with_buffer.calls == (std::vector<std::string>{
                  "cleanup machine", "write word", "free path buffer",
                  "write word", "write word"}),
          "local float cleanup frees path buffer after base cleanup");
    Check(pal4::GobLocalFloat::HandleEvent(
              Runtime::object, 9u, with_buffer) == 0x5555u &&
              with_buffer.calls.back() == "forward event",
          "local float event thunk result");
}

void CheckMachineUpdate() {
    Runtime stopped;
    SetInitialMachine(stopped);
    stopped.after_position = {10.0f, 20.0f, 30.0f};
    Check(pal4::GobLocalFloat::UpdateMachineAndActor(
              Runtime::object, 0.5f, stopped) == 0u &&
              stopped.stored_rotation == 6.0f &&
              stopped.calls == (std::vector<std::string>{
                  "update machine tasks", "store platform delta"}),
          "rotation alone does not move actor platform");
    Runtime disabled;
    SetInitialMachine(disabled);
    disabled.words[Runtime::object + 0x2d0u] = 0u;
    Check(pal4::GobLocalFloat::UpdateMachineAndActor(
              Runtime::object, 0.5f, disabled) == 1u &&
              disabled.calls == (std::vector<std::string>{
                  "update machine tasks", "store platform delta"}),
          "moving machine without platform actor flag");
    Runtime absent;
    SetInitialMachine(absent);
    absent.words[0x4004u] = 0u;
    Check(pal4::GobLocalFloat::UpdateMachineAndActor(
              Runtime::object, 0.5f, absent) == 0x4000u &&
              absent.calls.back() == "actor control access",
          "missing actor returns access object result");

    Runtime active;
    SetInitialMachine(active);
    active.words[0x4004u] = 0x5000u;
    active.words[0x5014u] = 0x6000u;
    active.words[0x6004u] = 0x7000u;
    active.floats[0x50a0u] = 40.0f;
    active.floats[0x50a4u] = 50.0f;
    active.floats[0x50a8u] = 60.0f;
    active.floats[0x7040u] = 4.0f;
    active.floats[0x7044u] = 5.0f;
    active.floats[0x7048u] = 6.0f;
    Check(pal4::GobLocalFloat::UpdateMachineAndActor(
              Runtime::object, 0.5f, active) == 0x8888u &&
              active.stored_rotation == 6.0f,
          "local float active actor result and angle delta");
    Runtime::CheckVector(active.stored_delta, 2.0f, -3.0f, 4.0f,
                         "local float platform translation");
    Runtime::CheckVector(active.actor_position, 4.0f, 5.0f, 6.0f,
                         "local float actor frame position");
    Runtime::CheckVector(active.actor_transform, 40.0f, -46.0f, 60.0f,
                         "local float positive axis heading");
    Check(active.calls == (std::vector<std::string>{
              "update machine tasks", "store platform delta",
              "actor control access", "apply actor translation",
              "matrix translate", "matrix rotate", "matrix translate",
              "frame update", "matrix query", "set actor position",
              "set actor transform", "update minimap",
              "update floor attributes"}),
          "local float actor and RenderWare call order");
    active.query_axis_y = -1.0f;
    active.calls.clear();
    SetInitialMachine(active);
    (void)pal4::GobLocalFloat::UpdateMachineAndActor(
        Runtime::object, 0.5f, active);
    Check(active.actor_transform.y == -54.0f,
          "local float negative axis heading");
    active.query_axis_y = 0.0f;
    active.calls.clear();
    SetInitialMachine(active);
    (void)pal4::GobLocalFloat::UpdateMachineAndActor(
        Runtime::object, 0.5f, active);
    Check(active.actor_transform.y == 50.0f,
          "local float zero axis preserves heading");

    Runtime nan_delta;
    SetInitialMachine(nan_delta);
    nan_delta.after_position = {
        std::nanf(""), 20.0f, 30.0f};
    Check(pal4::GobLocalFloat::UpdateMachineAndActor(
              Runtime::object, 0.5f, nan_delta) == 0u &&
              nan_delta.calls.size() == 2u,
          "unordered displacement is not treated as movement");
}

void CheckDebugPath() {
    Runtime absent;
    absent.words[Runtime::object + 0x2dcu] = 0u;
    absent.words[Runtime::object + 0x2e0u] = 2u;
    pal4::GobLocalFloat::RenderDebugPath(Runtime::object, absent);
    Check(absent.calls.empty(), "missing local float debug points");
    Runtime empty;
    empty.words[Runtime::object + 0x2dcu] = 0x3000u;
    empty.words[Runtime::object + 0x2e0u] = 0u;
    pal4::GobLocalFloat::RenderDebugPath(Runtime::object, empty);
    Check(empty.calls.empty(), "nonpositive local float path count");
    Runtime one;
    one.words[Runtime::object + 0x2dcu] = 0x3000u;
    one.words[Runtime::object + 0x2e0u] = 1u;
    pal4::GobLocalFloat::RenderDebugPath(Runtime::object, one);
    Check(one.render_states ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {6u, 1u}, {7u, 1u}, {1u, 0u},
                  {6u, 1u}, {7u, 2u}}) &&
              one.transformed.empty(),
          "single local float path point changes states without segments");

    Runtime path;
    path.words[Runtime::object + 0x2dcu] = 0x3000u;
    path.words[Runtime::object + 0x2e0u] = 2u;
    path.floats[0x3000u] = 1.0f;
    path.floats[0x3004u] = 2.0f;
    path.floats[0x3008u] = 3.0f;
    path.floats[0x300cu] = 4.0f;
    path.floats[0x3010u] = 5.0f;
    path.floats[0x3014u] = 6.0f;
    pal4::GobLocalFloat::RenderDebugPath(Runtime::object, path);
    Check(path.circle_angles.size() == 21u &&
              path.circle_angles.front() == 0.0f &&
              path.circle_angles.back() ==
                  static_cast<float>(2.0 * std::numbers::pi) &&
              path.transform_modes ==
                  (std::vector<std::uint32_t>{2u, 0u}) &&
              path.transformed.size() == 2u &&
              path.transformed[0].size() == 21u &&
              path.transformed[1].size() == 2u,
          "local float circle sampling and transform modes");
    Runtime::CheckVector(path.transformed[0][0].position,
                         1.0f, 2.0f, 7.0f,
                         "local float ring first vertex");
    Runtime::CheckVector(path.transformed[1][0].position,
                         1.0f, 2.0f, 3.0f,
                         "local float segment start");
    Runtime::CheckVector(path.transformed[1][1].position,
                         4.0f, 5.0f, 6.0f,
                         "local float segment end");
    Check(path.transformed[0][0].color == 0xefdcdc11u &&
              path.transformed[1][0].color == 0xef11dc18u &&
              path.transformed[1][1].color == 0xef0010dcu &&
              path.render_states == one.render_states &&
              std::count(path.calls.begin(), path.calls.end(),
                         "render primitive") == 2 &&
              std::count(path.calls.begin(), path.calls.end(),
                         "end primitive") == 2,
          "local float debug colors, draw count and state restoration");
    Runtime skipped_draw;
    skipped_draw.words = path.words;
    skipped_draw.floats = path.floats;
    skipped_draw.ring_transform_succeeds = false;
    skipped_draw.segment_transform_succeeds = false;
    pal4::GobLocalFloat::RenderDebugPath(
        Runtime::object, skipped_draw);
    Check(skipped_draw.transformed.size() == 2u &&
              std::count(skipped_draw.calls.begin(),
                         skipped_draw.calls.end(),
                         "render primitive") == 0 &&
              skipped_draw.render_states == one.render_states,
          "failed RenderWare transforms skip draw and restore states");
}

void CheckPathRebuild() {
    Runtime no_document;
    no_document.words[Runtime::object + 0x2dcu] = 0x3000u;
    no_document.words[Runtime::object + 0x2e0u] = 5u;
    no_document.config_document = 0u;
    Check(pal4::GobLocalFloat::RebuildMachineTaskPathPoints(
              Runtime::object, no_document) == 0u &&
              no_document.words.at(Runtime::object + 0x2dcu) == 0u &&
              no_document.words.at(Runtime::object + 0x2e0u) == 0u &&
              no_document.calls == (std::vector<std::string>{
                  "free path buffer", "write word", "write word",
                  "get machine config"}),
          "local float rebuild frees old path before missing config");
    Runtime no_tasks;
    no_tasks.words[Runtime::object + 0x2dcu] = 0u;
    no_tasks.tasks_present = false;
    Check(pal4::GobLocalFloat::RebuildMachineTaskPathPoints(
              Runtime::object, no_tasks) == 0u &&
              no_tasks.calls.back() == "find machine tasks",
          "missing machine-task XML child");
    Runtime no_children;
    no_children.words[Runtime::object + 0x2dcu] = 0u;
    Check(pal4::GobLocalFloat::RebuildMachineTaskPathPoints(
              Runtime::object, no_children) == 0u &&
              no_children.calls.back() == "XML child count",
          "empty machine task list");

    Runtime path;
    path.words[Runtime::object + 0x2dcu] = 0u;
    path.xml_children = {0x8200u, 0x8201u, 0x8202u,
                         0x8203u, 0x8204u};
    path.xml_types = {{0x8200u, 16}, {0x8201u, 3},
                      {0x8202u, 10}, {0x8203u, 0}};
    path.xml_floats = {
        {{0x8200u, "PAL4_GOMTask_start_posX"}, 1.0f},
        {{0x8200u, "PAL4_GOMTask_start_posY"}, 2.0f},
        {{0x8200u, "PAL4_GOMTask_start_posZ"}, 3.0f},
        {{0x8202u, "PAL4_GOMTask_movment_tPosX"}, 4.0f},
        {{0x8202u, "PAL4_GOMTask_movment_tPosZ"}, 6.0f},
        {{0x8203u, "PAL4_GOMTask_move_target_X"}, 7.0f},
        {{0x8203u, "PAL4_GOMTask_move_target_Y"}, 8.0f},
        {{0x8203u, "PAL4_GOMTask_move_target_Z"}, 9.0f}};
    Check(pal4::GobLocalFloat::RebuildMachineTaskPathPoints(
              Runtime::object, path) == 5u &&
              path.words.at(Runtime::object + 0x2e0u) == 3u &&
              path.words.at(Runtime::object + 0x2dcu) == 0x3000u,
          "local float path count and allocation");
    Check(path.xml_type_reads ==
              (std::vector<std::uint32_t>{
                  0x8200u, 0x8201u, 0x8202u, 0x8203u, 0x8204u,
                  0x8200u, 0x8201u, 0x8202u, 0x8203u, 0x8204u}) &&
              path.xml_float_keys == (std::vector<std::string>{
                  "PAL4_GOMTask_start_posX",
                  "PAL4_GOMTask_start_posY",
                  "PAL4_GOMTask_start_posZ",
                  "PAL4_GOMTask_movment_tPosX",
                  "PAL4_GOMTask_movment_tPosY",
                  "PAL4_GOMTask_movment_tPosZ",
                  "PAL4_GOMTask_move_target_X",
                  "PAL4_GOMTask_move_target_Y",
                  "PAL4_GOMTask_move_target_Z"}),
          "local float two XML passes and exact coordinate keys");
    Check(path.floats.at(0x3000u) == 1.0f &&
              path.floats.at(0x3004u) == 2.0f &&
              path.floats.at(0x3008u) == 3.0f &&
              path.floats.at(0x300cu) == 4.0f &&
              path.floats.at(0x3010u) == 0.0f &&
              path.floats.at(0x3014u) == 6.0f &&
              path.floats.at(0x3018u) == 7.0f &&
              path.floats.at(0x301cu) == 8.0f &&
              path.floats.at(0x3020u) == 9.0f,
          "local float compact path writes and missing coordinate default");
    Runtime allocation_failed;
    allocation_failed.words[Runtime::object + 0x2dcu] = 0u;
    allocation_failed.xml_children = {0x8200u};
    allocation_failed.xml_types[0x8200u] = 0;
    allocation_failed.path_allocation = 0u;
    Check(pal4::GobLocalFloat::RebuildMachineTaskPathPoints(
              Runtime::object, allocation_failed) == 0u &&
              allocation_failed.words.at(Runtime::object + 0x2e0u) == 1u &&
              allocation_failed.words.at(Runtime::object + 0x2dcu) == 0u,
          "path allocation failure is contained at runtime boundary");
}

void CheckSceneInitialization() {
    Runtime object_package;
    object_package.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobLocalFloat::InitializeSceneAndConfig(
              Runtime::object, 2u, "palobject/float", 0x3000u,
              0x4000u, object_package) == 0xabcdu &&
              object_package.words.at(Runtime::object + 0x2d0u) == 0u &&
              object_package.calls == (std::vector<std::string>{
                  "cleanup before scene load", "check object package",
                  "load object package", "initialize machine and config",
                  "cleanup script packages", "find parameters",
                  "find float config", "remove float config",
                  "create float config", "write word"}),
          "local float object package and XML node replacement");
    Runtime weapon;
    weapon.words[Runtime::object + 0x1c0u] = 0x6000u;
    weapon.old_float_present = false;
    Check(pal4::GobLocalFloat::InitializeSceneAndConfig(
              Runtime::object, 2u, "palweapon/float", 0x3000u,
              0x4000u, weapon) == 0xabcdu &&
              std::find(weapon.calls.begin(), weapon.calls.end(),
                        "load weapon package") != weapon.calls.end() &&
              std::find(weapon.calls.begin(), weapon.calls.end(),
                        "remove float config") == weapon.calls.end() &&
              weapon.calls.back() == "write word",
          "local float weapon package and absent old XML node");
    Runtime scene;
    scene.words[Runtime::object + 0x1c0u] = 0x6000u;
    scene.scene_package = 0x5000u;
    scene.parameters_present = false;
    Check(pal4::GobLocalFloat::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/float", 0x3000u,
              0x4000u, scene) == 0xabcdu &&
              std::find(scene.calls.begin(), scene.calls.end(),
                        "load scene package") != scene.calls.end() &&
              std::find(scene.calls.begin(), scene.calls.end(),
                        "create float config") == scene.calls.end() &&
              scene.calls.back() == "write word",
          "local float scene package and absent parameters");
    Runtime failed;
    failed.machine_init_result = 0u;
    Check(pal4::GobLocalFloat::InitializeSceneAndConfig(
              Runtime::object, 2u, "scene/float", 0x3000u,
              0x4000u, failed) == 0u &&
              failed.calls.back() == "cleanup script packages" &&
              failed.words.find(Runtime::object + 0x2d0u) ==
                  failed.words.end(),
          "failed machine initialization cleans scripts without XML work");
}

void CheckMovementAndTaskGates() {
    Runtime movement;
    movement.words[Runtime::object + 0x14u] = 0x6000u;
    movement.words[Runtime::object + 0x188u] = 7u;
    movement.words[0x5000u] = 0x5100u;
    movement.words[0x5004u] = 0x5200u;
    Check(pal4::GobLocalFloat::AttachMovementAdapter(
              Runtime::object, movement) == 0xaaaau &&
              movement.calls == (std::vector<std::string>{
                  "movement adapter root", "movement property",
                  "set clump properties", "movement adapter root",
                  "attach movement helper"}),
          "local float adapter registration arguments and order");
    movement.calls.clear();
    Check(pal4::GobLocalFloat::DetachMovementAdapter(
              Runtime::object, movement) == 0xbbbbu &&
              movement.calls == (std::vector<std::string>{
                  "movement adapter root", "set clump render callback",
                  "movement adapter root", "detach movement helper"}),
          "local float adapter cleanup arguments and order");
    Runtime gate;
    gate.words[Runtime::object + 0x2d8u] = 0u;
    gate.words[Runtime::object + 0x198u] = 0u;
    Check(pal4::GobLocalFloat::TryTriggerByTouchTypeOrUseCount(
              Runtime::object, 0x8000u, gate) == 0u &&
              gate.calls.empty(), "local float disabled trigger");
    gate.words[Runtime::object + 0x2d8u] = 2u;
    Check(pal4::GobLocalFloat::TryTriggerByTouchTypeOrUseCount(
              Runtime::object, 0x8000u, gate) == 0xccccu &&
              gate.calls == (std::vector<std::string>{
                  "trigger machine task"}),
          "local float touch type trigger");
    gate.words[Runtime::object + 0x2d8u] = 0u;
    gate.words[Runtime::object + 0x198u] = 1u;
    gate.calls.clear();
    Check(pal4::GobLocalFloat::TryTriggerByTouchTypeOrUseCount(
              Runtime::object, 0x8000u, gate) == 0xccccu &&
              gate.calls == (std::vector<std::string>{
                  "trigger machine task"}),
          "local float inherited use-count trigger");
    gate.calls.clear();
    Check(pal4::GobLocalFloat::ResetMachineAndTouchLatch(
              Runtime::object, gate) == 0xddddu &&
              gate.words.at(Runtime::object + 0x2d0u) == 0u &&
              gate.calls == (std::vector<std::string>{
                  "reset triggered machine", "write word"}),
          "local float reset machine before clearing touch latch");
    gate.calls.clear();
    gate.task_containers_ready = true;
    Check(pal4::GobLocalFloat::StopTaskSounds(
              Runtime::object, gate) == 0x5au &&
              pal4::GobLocalFloat::ResumeTaskSounds(
                  Runtime::object, gate) == 0x3cu &&
              pal4::GobLocalFloat::AreTaskContainersReady(
                  Runtime::object, gate) &&
              gate.calls == (std::vector<std::string>{
                  "stop task sounds", "resume task sounds",
                  "task containers ready"}),
          "local float task sound and readiness thunks");
    Runtime config;
    config.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobLocalFloat::GetConfigNode(
              Runtime::object, config) == 0u && config.calls.empty(),
          "local float missing config document");
    config.words[Runtime::object + 0x1c0u] = 0x6000u;
    config.parameters_present = false;
    Check(pal4::GobLocalFloat::GetConfigNode(
              Runtime::object, config) == 0u &&
              config.calls == (std::vector<std::string>{
                  "find parameters"}),
          "local float missing parameter node");
    config.parameters_present = true;
    config.calls.clear();
    Check(pal4::GobLocalFloat::GetConfigNode(
              Runtime::object, config) == 0x9000u &&
              config.calls == (std::vector<std::string>{
                  "find parameters", "find float config"}),
          "local float config node path");
}

void CheckConfigVisibilityAndHelper() {
    Runtime no_node;
    no_node.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobLocalFloat::ReadConfigAndMachineFlags(
              Runtime::object, no_node) == 0u &&
              no_node.calls == (std::vector<std::string>{
                  "load machine main config"}) &&
              no_node.words.find(Runtime::object + 0x2d4u) ==
                  no_node.words.end(),
          "local float missing config still reads machine flags");
    Runtime defaults;
    defaults.words[Runtime::object + 0x1c0u] = 0x6000u;
    Check(pal4::GobLocalFloat::ReadConfigAndMachineFlags(
              Runtime::object, defaults) == 1u &&
              defaults.words.at(Runtime::object + 0x190u) == 0u &&
              defaults.words.at(Runtime::object + 0x2d4u) == 0u &&
              defaults.words.at(Runtime::object + 0x2d8u) == 1u &&
              defaults.words.at(Runtime::object + 0x198u) == 0xffffffffu &&
              defaults.calls == (std::vector<std::string>{
                  "load machine main config", "find parameters",
                  "find float config", "write word", "read auto touch",
                  "write word", "read touch type", "write word",
                  "write word"}),
          "local float config defaults and use-count override");
    Runtime explicit_values;
    explicit_values.words[Runtime::object + 0x1c0u] = 0x6000u;
    explicit_values.config_auto_touch = -2;
    explicit_values.config_touch_type = 2;
    Check(pal4::GobLocalFloat::ReadConfigAndMachineFlags(
              Runtime::object, explicit_values) == 2u &&
              explicit_values.words.at(Runtime::object + 0x2d4u) ==
                  static_cast<std::uint32_t>(-2) &&
              explicit_values.words.at(Runtime::object + 0x2d8u) == 2u &&
              explicit_values.words.find(Runtime::object + 0x198u) ==
                  explicit_values.words.end(),
          "local float explicit config values");
    Runtime hidden;
    hidden.words[Runtime::object + 0x2d4u] = 1u;
    Check(pal4::GobLocalFloat::ApplyVisibleAndAutoTouch(
              Runtime::object, 0u, hidden) == 1u &&
              hidden.calls == (std::vector<std::string>{
                  "apply machine visible"}),
          "local float hidden auto touch gate");
    hidden.calls.clear();
    Check(pal4::GobLocalFloat::ApplyVisibleAndAutoTouch(
              Runtime::object, 2u, hidden) == 0xeeeeu &&
              hidden.calls == (std::vector<std::string>{
                  "apply machine visible", "invoke machine trigger"}),
          "local float visible auto trigger with zero vector");
    hidden.words[Runtime::object + 0x2d4u] = 0u;
    hidden.calls.clear();
    Check(pal4::GobLocalFloat::ApplyVisibleAndAutoTouch(
              Runtime::object, 2u, hidden) == 0u &&
              hidden.calls == (std::vector<std::string>{
                  "apply machine visible"}),
          "local float disabled auto touch gate");
    hidden.words[Runtime::object + 0x14u] = 0x6000u;
    Check(pal4::GobLocalFloat::HelperClump(
              Runtime::object, hidden) == 0x6000u,
          "local float helper clump negative offset");

    Runtime latched;
    latched.words[Runtime::object + 0x2d0u] = 2u;
    Check(pal4::GobLocalFloat::ProcessHelperTouchState(
              Runtime::object, latched) == 2u &&
              latched.calls.empty(), "local float helper already latched");
    Runtime typed;
    typed.words[Runtime::object + 0x2d0u] = 0u;
    typed.words[Runtime::object + 0x2d8u] = 3u;
    Check(pal4::GobLocalFloat::ProcessHelperTouchState(
              Runtime::object, typed) == 3u &&
              typed.words.at(Runtime::object + 0x2d0u) == 1u &&
              typed.calls == (std::vector<std::string>{"write word"}),
          "local float helper touch type latches without trigger");
    Runtime trigger;
    trigger.words[Runtime::object + 0x2d0u] = 0u;
    trigger.words[Runtime::object + 0x2d8u] = 0u;
    trigger.words[Runtime::object + 0x27cu] = 0u;
    Check(pal4::GobLocalFloat::ProcessHelperTouchState(
              Runtime::object, trigger) == 0xeeeeu &&
              trigger.calls == (std::vector<std::string>{
                  "write word", "invoke machine trigger"}),
          "local float helper zero-vector trigger");
    Runtime pending;
    pending.words[Runtime::object + 0x2d0u] = 0u;
    pending.words[Runtime::object + 0x2d8u] = 0u;
    pending.words[Runtime::object + 0x27cu] = 1u;
    pending.words[Runtime::object + 0x278u] = 9u;
    Check(pal4::GobLocalFloat::ProcessHelperTouchState(
              Runtime::object, pending) == 9u &&
              pending.words.at(Runtime::object + 0x278u) == 0u &&
              pending.calls == (std::vector<std::string>{
                  "write word", "write word"}),
          "local float helper consumes pending flag");
    pending.calls.clear();
    Check(pal4::GobLocalFloat::ResetHelperTouchState(
              Runtime::object, pending) == 1u &&
              pending.words.at(Runtime::object + 0x2d0u) == 0u &&
              pending.calls == (std::vector<std::string>{"write word"}) &&
              pal4::GobLocalFloat::ResetHelperTouchState(
                  Runtime::object, pending) == 0u,
          "local float helper reset returns previous latch");
}

void CheckStateSnapshot() {
    Runtime disabled;
    disabled.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobLocalFloat::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
              disabled.calls.empty(), "local float snapshot store gate");
    Runtime unavailable;
    unavailable.words[Runtime::object + 0x1a4u] = 1u;
    unavailable.snapshot_allocation = 0u;
    Check(pal4::GobLocalFloat::CreateStateSnapshot(
              Runtime::object, unavailable) == 0u &&
              unavailable.calls == (std::vector<std::string>{"allocate"}),
          "local float snapshot allocation failure");
    Runtime created;
    created.words[Runtime::object + 0x1a4u] = 1u;
    created.words[Runtime::object + 4u] = 0x9100u;
    created.words[Runtime::object + 0x184u] = 3u;
    created.words[Runtime::object + 0x198u] = 7u;
    created.floats[Runtime::object + 0xacu] = 1.0f;
    created.floats[Runtime::object + 0xb0u] = 2.0f;
    created.floats[Runtime::object + 0xb4u] = 3.0f;
    created.floats[Runtime::object + 0xa0u] = 4.0f;
    created.floats[Runtime::object + 0xa4u] = 5.0f;
    created.floats[Runtime::object + 0xa8u] = 6.0f;
    Check(pal4::GobLocalFloat::CreateStateSnapshot(
              Runtime::object, created) == 0x4000u &&
              created.snapshot_name == "Raft" &&
              created.words.at(0x4014u) == 3u &&
              created.words.at(0x4030u) == 7u &&
              created.float_writes ==
                  (std::vector<std::pair<std::uint32_t, float>>{
                      {0x4018u, 1.0f}, {0x401cu, 2.0f},
                      {0x4020u, 3.0f}, {0x4024u, 4.0f},
                      {0x4028u, 5.0f}, {0x402cu, 6.0f}}),
          "local float snapshot fields and order");
    Runtime failed;
    failed.words[Runtime::object + 0x1a4u] = 1u;
    failed.fail_snapshot_construct = true;
    bool thrown = false;
    try {
        (void)pal4::GobLocalFloat::CreateStateSnapshot(
            Runtime::object, failed);
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct snapshot", "free"}),
          "local float snapshot constructor failure cleanup");
    Check(pal4::GobLocalFloat::ApplyStateSnapshot(
              Runtime::object, 0x4000u, disabled) == 0u &&
              disabled.calls.empty(),
          "local float disabled snapshot restore");
    Runtime wrong;
    wrong.words[Runtime::object + 0x1a4u] = 1u;
    wrong.snapshot_type = 8u;
    Check(pal4::GobLocalFloat::ApplyStateSnapshot(
              Runtime::object, 0x4000u, wrong) == 8u &&
              wrong.calls == (std::vector<std::string>{"snapshot type"}),
          "local float snapshot type gate");
    created.calls.clear();
    Check(pal4::GobLocalFloat::ApplyStateSnapshot(
              Runtime::object, 0x4000u, created) == 0x1234u &&
              created.words.at(Runtime::object + 0x198u) == 7u &&
              created.calls == (std::vector<std::string>{
                  "snapshot type", "set base visible",
                  "set base position", "set base rotation",
                  "write word"}),
          "local float snapshot restores visibility, transform and research");
}

void CheckHelperDeletion() {
    Runtime runtime;
    Check(pal4::GobLocalFloat::DeleteHelper(
              0x2000u, 2u, runtime) == 0x2000u &&
              runtime.words.at(0x2000u) ==
                  pal4::GobLocalFloat::kHelperBaseVtable &&
              runtime.calls == (std::vector<std::string>{"write word"}),
          "local float helper destructor restores base vtable");
    runtime.calls.clear();
    Check(pal4::GobLocalFloat::DeleteHelper(
              0x2000u, 3u, runtime) == 0x2000u &&
              runtime.calls == (std::vector<std::string>{
                  "write word", "free"}),
          "local float helper deleting flag frees allocation");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckCleanupAndForwarding();
        CheckMachineUpdate();
        CheckDebugPath();
        CheckPathRebuild();
        CheckSceneInitialization();
        CheckMovementAndTaskGates();
        CheckConfigVisibilityAndHelper();
        CheckStateSnapshot();
        CheckHelperDeletion();
        std::cout << "gob_local_float_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
