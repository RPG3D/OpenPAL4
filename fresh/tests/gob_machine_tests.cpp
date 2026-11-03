#include "pal4/gob_machine.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::size_t IndexOf(const std::vector<std::string>& calls,
                    const std::string& value) {
    const auto found = std::find(calls.begin(), calls.end(), value);
    if (found == calls.end()) throw std::runtime_error("missing " + value);
    return static_cast<std::size_t>(found - calls.begin());
}

struct Runtime final : pal4::IGobMachineRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocation = object;
    bool fail_base = false;
    std::uint32_t model_result = 0x99u;
    std::uint32_t scene_package = 0u;
    std::uint32_t old_machine_node = 0u;
    std::uint32_t parameters_node = 0x8000u;
    std::uint8_t primary_ready = 0u;
    std::uint8_t context_ready = 0u;
    int created_conditions = 0;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::GobMachine::kSize, "machine allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "machine free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "machine base constructor");
        calls.emplace_back("construct base");
        if (fail_base) throw std::runtime_error("base failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "machine base destructor");
        calls.emplace_back("destruct base");
    }
    void InitializeAnimationState(std::uint32_t address) override {
        Check(address == object + 0x1dcu, "animation state init");
        calls.emplace_back("initialize animation state");
    }
    void CleanupAnimationState(std::uint32_t address) override {
        Check(address == object + 0x1dcu, "animation state destructor");
        calls.emplace_back("destroy animation state");
    }
    void CleanupAnimationData(std::uint32_t address) override {
        Check(address == object + 0x1dcu, "animation data cleanup");
        calls.emplace_back("cleanup animation data");
    }
    void InitializeTaskContainer(std::uint32_t address) override {
        Check(address == object + 0x258u ||
              address == object + 0x268u, "task container init");
        calls.emplace_back(address == object + 0x258u ?
                           "initialize primary" : "initialize context");
    }
    void DestroyTaskContainer(std::uint32_t address) override {
        calls.emplace_back(address == object + 0x268u ?
                           "destroy context" : "destroy primary");
    }
    void ClearTaskContainer(std::uint32_t address) override {
        calls.emplace_back(address == object + 0x258u ?
                           "clear primary" : "clear context");
    }
    void InitializeTaskArrayRecord(std::uint32_t address) override {
        Check(address == object + 0x2a0u, "task array init");
        calls.emplace_back("initialize task array");
    }
    void InitializeStringRecord(std::uint32_t address) override {
        Check(address == object + 0x2bcu, "string init");
        calls.emplace_back("initialize string");
    }
    void ReleaseStringRecord(std::uint32_t address) override {
        Check(address == object + 0x2bcu, "string release");
        calls.emplace_back("release string");
    }
    void FreeTaskArray(std::uint32_t address) override {
        Check(address == 0x5000u, "task array free");
        calls.emplace_back("free task array");
    }
    void CleanupBaseResources(std::uint32_t address) override {
        Check(address == object, "base cleanup");
        calls.emplace_back("cleanup base resources");
    }
    void UpdateBaseClump(float elapsed) override {
        Check(elapsed == 0.25f, "base clump elapsed");
        calls.emplace_back("update base clump");
    }
    void UpdateAnimation(std::uint32_t state,
                         float elapsed) override {
        Check(state == object + 0x1dcu && elapsed == 0.25f,
              "animation update arguments");
        calls.emplace_back("update animation");
    }
    std::uint32_t UpdateTaskContainer(std::uint32_t container,
                                       float elapsed) override {
        Check(elapsed == 0.25f, "task elapsed");
        calls.emplace_back(container == object + 0x258u ?
                           "update primary" : "update context");
        return 0x33u;
    }
    std::uint32_t TriggerTaskRun(
        std::uint32_t address,
        const std::array<float, 3>& position) override {
        Check(address == object && position ==
              (std::array<float, 3>{0.0f, 0.0f, 0.0f}),
              "auto-touch retrigger arguments");
        calls.emplace_back("retrigger task run");
        return 0x44u;
    }
    void ReaddActiveSceneObject(std::uint32_t address,
                                float weight) override {
        Check(address == object && weight == 1.0f,
              "active scene object arguments");
        calls.emplace_back("readd active object");
    }
    std::uint32_t RefreshMinimap() override {
        calls.emplace_back("refresh minimap");
        return 0x66u;
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "preload cleanup object");
        calls.emplace_back("preload cleanup");
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
              "scene package address");
        calls.emplace_back("load scene package");
    }
    std::uint32_t InitializeBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && !path.empty() &&
              fourth == 3u && fifth == 4u,
              "base model arguments");
        calls.emplace_back("initialize base model");
        return model_result;
    }
    void HideHookUserDataAtomics(std::uint32_t clump) override {
        Check(clump == 0x5000u, "hook atomic clump");
        calls.emplace_back("hide hook atomics");
    }
    void LoadAnimationData(std::uint32_t state,
                           std::uint32_t clump,
                           std::string_view path,
                           std::uint32_t argument) override {
        Check(state == object + 0x1dcu && clump == 0x5000u &&
              !path.empty() && argument == 3u,
              "animation load arguments");
        calls.emplace_back("load animation data");
    }
    std::uint32_t FindXmlChild(std::uint32_t parent,
                                std::string_view name) override {
        calls.emplace_back("find XML child");
        if (parent == 0x6000u && name ==
            "PAL4-GameObject-Parameters") return parameters_node;
        if (parent == 0x8000u && name ==
            "PAL4_GameObject-machine") return old_machine_node;
        throw std::runtime_error("unexpected XML lookup");
    }
    void RemoveXmlChild(std::uint32_t parent,
                        std::uint32_t child) override {
        Check(parent == 0x8000u && child == old_machine_node,
              "remove old machine node");
        calls.emplace_back("remove XML child");
    }
    std::uint32_t CreateXmlChild(std::uint32_t parent,
                                  std::string_view name) override {
        if (parent == 0x8000u && name ==
            "PAL4_GameObject-machine") {
            calls.emplace_back("create machine node");
            return 0x9000u;
        }
        Check(parent == 0x9000u && name ==
              "PAL4_GameObject-machine-condition",
              "machine condition node");
        ++created_conditions;
        calls.emplace_back("create condition");
        return 0xa000u;
    }
    void ClearAnimationFlag(std::uint32_t state,
                            std::uint32_t flag) override {
        Check(state == object + 0x1dcu && flag == 1u,
              "clear animation flag");
        calls.emplace_back("clear animation flag");
    }
    void SetAnimationFlag(std::uint32_t state,
                          std::uint32_t flag) override {
        Check(state == object + 0x1dcu && flag == 0u,
              "set animation flag");
        calls.emplace_back("set animation flag");
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup scripts");
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == object + 0x264u ||
              address == object + 0x274u, "task ready byte");
        return address == object + 0x264u ?
            primary_ready : context_ready;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address - object));
    }
};

void CheckLifecycle() {
    Check(pal4::GobMachine::TypeKey() == 5u, "machine factory type key");
    Runtime null_memory;
    null_memory.allocation = 0u;
    Check(pal4::GobMachine::Create(null_memory) == 0u &&
          null_memory.calls == (std::vector<std::string>{"allocate"}),
          "null machine allocation");
    Runtime failed;
    failed.fail_base = true;
    bool thrown = false;
    try { (void)pal4::GobMachine::Create(failed); }
    catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate", "construct base", "free"}),
          "base construction failure frees allocation");
    Runtime runtime;
    Check(pal4::GobMachine::Create(runtime) == Runtime::object &&
          runtime.calls.front() == "allocate" &&
          IndexOf(runtime.calls, "construct base") <
              IndexOf(runtime.calls, "initialize animation state") &&
          IndexOf(runtime.calls, "initialize primary") <
              IndexOf(runtime.calls, "initialize context") &&
          IndexOf(runtime.calls, "initialize context") <
              IndexOf(runtime.calls, "initialize task array") &&
          runtime.words.at(Runtime::object) ==
              pal4::GobMachine::kVtable &&
          runtime.words.at(Runtime::object + 0x1dcu) ==
              pal4::GobMachine::kStateVtable,
          "machine constructor order and vtables");
    for (const auto offset : {0x278u, 0x27cu, 0x280u, 0x284u,
                              0x2a4u, 0x2a8u, 0x2acu,
                              0x2b0u, 0x2b4u, 0x2b8u,
                              0x2c0u, 0x2c4u, 0x2c8u})
        Check(runtime.words.at(Runtime::object + offset) == 0u,
              "machine constructor zero field");
    runtime.calls.clear();
    runtime.words[Runtime::object + 0x2a4u] = 0x5000u;
    pal4::GobMachine::Destruct(Runtime::object, runtime);
    Check(IndexOf(runtime.calls, "release string") <
              IndexOf(runtime.calls, "free task array") &&
          IndexOf(runtime.calls, "free task array") <
              IndexOf(runtime.calls, "destroy context") &&
          IndexOf(runtime.calls, "destroy context") <
              IndexOf(runtime.calls, "destroy primary") &&
          IndexOf(runtime.calls, "destroy primary") <
              IndexOf(runtime.calls, "destroy animation state") &&
          runtime.calls.back() == "destruct base" &&
          runtime.words.at(Runtime::object + 0x2a4u) == 0u,
          "machine destructor releases owned resources in order");
}

void CheckResourceCleanup() {
    Runtime runtime;
    runtime.words[Runtime::object + 0x2a4u] = 0x5000u;
    runtime.words[Runtime::object + 0x2a8u] = 0x5008u;
    for (const auto offset : {0x278u, 0x27cu, 0x280u, 0x284u})
        runtime.words[Runtime::object + offset] = 1u;
    Check(pal4::GobMachine::CleanupResources(
              Runtime::object, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "cleanup animation data", "cleanup base resources",
              "clear primary", "clear context", "write 680",
              "write 632", "write 636", "write 640", "write 644"}) &&
          runtime.words.at(Runtime::object + 0x2a8u) == 0x5000u,
          "machine cleanup order and pointer reset");
    for (const auto offset : {0x278u, 0x27cu, 0x280u, 0x284u})
        Check(runtime.words.at(Runtime::object + offset) == 0u,
              "machine cleanup clears task state");
}

void PrepareUpdate(Runtime& runtime) {
    runtime.words[Runtime::object + 0x278u] = 0u;
    runtime.words[Runtime::object + 0x27cu] = 1u;
    runtime.words[Runtime::object + 0x2a4u] = 0x5000u;
    runtime.words[Runtime::object + 0x2a8u] = 0x5008u;
    runtime.words[Runtime::object + 0x280u] = 0u;
    runtime.words[Runtime::object + 0x198u] = 0u;
    runtime.words[Runtime::object + 0x190u] = 0u;
}

void CheckUpdateBranches() {
    Runtime pending;
    PrepareUpdate(pending);
    pending.words[Runtime::object + 0x278u] = 9u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, pending) == 9u &&
          pending.calls == (std::vector<std::string>{
              "update base clump", "update animation"}),
          "pending gate after base and animation updates");
    Runtime idle;
    PrepareUpdate(idle);
    idle.words[Runtime::object + 0x27cu] = 0u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, idle) == 0u,
          "idle gate");
    Runtime primary;
    PrepareUpdate(primary);
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, primary) == 0x33u &&
          primary.calls.back() == "update primary" &&
          primary.words.at(Runtime::object + 0x27cu) == 1u,
          "primary container update");
    Runtime context;
    PrepareUpdate(context);
    context.primary_ready = 1u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, context) == 0x33u &&
          context.calls.back() == "update context",
          "context container update");
    Runtime auto_touch;
    PrepareUpdate(auto_touch);
    auto_touch.primary_ready = auto_touch.context_ready = 1u;
    auto_touch.words[Runtime::object + 0x280u] = 1u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, auto_touch) == 0x44u &&
          auto_touch.words.at(Runtime::object + 0x27cu) == 0u &&
          auto_touch.words.at(Runtime::object + 0x2a8u) == 0x5000u &&
          auto_touch.calls.back() == "retrigger task run",
          "auto-touch completion retriggers after pointer reset");
    Runtime remaining_uses;
    PrepareUpdate(remaining_uses);
    remaining_uses.primary_ready = remaining_uses.context_ready = 1u;
    remaining_uses.words[Runtime::object + 0x198u] = 3u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, remaining_uses) == 3u &&
          std::find(remaining_uses.calls.begin(),
                    remaining_uses.calls.end(),
                    "refresh minimap") == remaining_uses.calls.end(),
          "remaining uses skip minimap refresh");
    Runtime exhausted;
    PrepareUpdate(exhausted);
    exhausted.primary_ready = exhausted.context_ready = 1u;
    exhausted.words[Runtime::object + 0x190u] = 1u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, exhausted) == 0x66u &&
          exhausted.calls[exhausted.calls.size() - 2u] ==
              "readd active object" &&
          exhausted.calls.back() == "refresh minimap",
          "exhausted auto-disappear readds then refreshes minimap");
    Runtime visible;
    PrepareUpdate(visible);
    visible.primary_ready = visible.context_ready = 1u;
    Check(pal4::GobMachine::UpdateRuntimeAndTasks(
              Runtime::object, 0.25f, visible) == 0x66u &&
          visible.calls.back() == "refresh minimap" &&
          std::find(visible.calls.begin(), visible.calls.end(),
                    "readd active object") == visible.calls.end(),
          "exhausted without auto-disappear refreshes only");
}

void PrepareScene(Runtime& runtime) {
    runtime.words[Runtime::object + 0x14u] = 0x5000u;
    runtime.words[Runtime::object + 0x1c0u] = 0x6000u;
}

void CheckSceneInitialization() {
    for (const auto path : {"palobject/machine", "palweapon/machine",
                            "arena/machine"}) {
        Runtime runtime;
        PrepareScene(runtime);
        runtime.scene_package = 0x7000u;
        runtime.old_machine_node = 0xb000u;
        Check(pal4::GobMachine::InitializeSceneAndConfig(
                  Runtime::object, 2u, path, 3u, 4u,
                  runtime) == 0x99u &&
              runtime.calls.front() == "preload cleanup" &&
              IndexOf(runtime.calls, "initialize base model") <
                  IndexOf(runtime.calls, "hide hook atomics") &&
              IndexOf(runtime.calls, "hide hook atomics") <
                  IndexOf(runtime.calls, "load animation data") &&
              IndexOf(runtime.calls, "remove XML child") <
                  IndexOf(runtime.calls, "create machine node") &&
              runtime.created_conditions == 5 &&
              IndexOf(runtime.calls, "clear animation flag") <
                  IndexOf(runtime.calls, "set animation flag") &&
              runtime.calls.back() == "cleanup scripts",
              "machine initialization order and XML replacement");
        const auto expected = path[3] == 'o' ? "load object package" :
            path[3] == 'w' ? "load weapon package" :
                             "load scene package";
        Check(std::find(runtime.calls.begin(), runtime.calls.end(),
                        expected) != runtime.calls.end(),
              "machine scene package routing");
    }
    Runtime no_document;
    PrepareScene(no_document);
    no_document.words[Runtime::object + 0x1c0u] = 0u;
    Check(pal4::GobMachine::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/machine", 3u, 4u,
              no_document) == 0u &&
          IndexOf(no_document.calls, "load animation data") <
              IndexOf(no_document.calls, "cleanup scripts") &&
          no_document.created_conditions == 0,
          "missing document returns zero after animation load");
    Runtime no_parameters;
    PrepareScene(no_parameters);
    no_parameters.parameters_node = 0u;
    Check(pal4::GobMachine::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/machine", 3u, 4u,
              no_parameters) == 0x99u &&
          no_parameters.created_conditions == 0 &&
          IndexOf(no_parameters.calls, "clear animation flag") <
              IndexOf(no_parameters.calls, "cleanup scripts"),
          "missing parameters still finishes animation flags");
    Runtime failed;
    failed.model_result = 0u;
    Check(pal4::GobMachine::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/machine", 3u, 4u,
              failed) == 0u &&
          failed.calls.back() == "cleanup scripts" &&
          std::find(failed.calls.begin(), failed.calls.end(),
                    "hide hook atomics") == failed.calls.end(),
          "failed model skips clump, animation and XML");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckResourceCleanup();
        CheckUpdateBranches();
        CheckSceneInitialization();
        std::cout << "gob_machine_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
