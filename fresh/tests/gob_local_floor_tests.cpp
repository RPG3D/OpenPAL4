#include "pal4/gob_local_floor.h"

#include <algorithm>
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

struct Runtime final : pal4::IGobLocalFloorRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    static constexpr std::uint32_t snapshot = 0x2000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t object_allocation = object;
    std::uint32_t snapshot_allocation = snapshot;
    std::uint32_t base_result = 0x55u;
    std::uint32_t scene_package = 0u;
    std::uint32_t snapshot_type = 1u;
    std::uint32_t adapter_helper = 0u;
    std::string snapshot_name;
    bool fail_base = false;
    bool fail_snapshot = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate " + std::to_string(size));
        Check(size == 0x1e0u || size == 0x18u, "allocation size");
        return size == 0x1e0u ? object_allocation : snapshot_allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object || address == snapshot, "freed address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "base constructor address");
        calls.emplace_back("construct base");
        if (fail_base) throw std::runtime_error("base failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "base destructor address");
        calls.emplace_back("destruct base");
    }
    std::uint32_t CleanupBase(std::uint32_t address,
                              std::uint32_t context) override {
        Check(address == object && context == 9u, "cleanup arguments");
        calls.emplace_back("cleanup base");
        return base_result;
    }
    std::uint32_t RenderBaseIfVisible(std::uint32_t address,
                                     std::uint32_t context) override {
        Check(address == object && context == 9u, "render arguments");
        calls.emplace_back("render base");
        return base_result;
    }
    std::uint32_t UpdateBaseClumpIfVisible(float elapsed) override {
        Check(elapsed == 0.25f, "clump update elapsed");
        calls.emplace_back("update clump");
        return base_result;
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "scene cleanup object");
        calls.emplace_back("preload cleanup");
    }
    bool UsesObjectPackage(std::string_view path) override {
        calls.emplace_back("check object");
        return path.find("palobject") != std::string_view::npos;
    }
    bool UsesWeaponPackage(std::string_view path) override {
        calls.emplace_back("check weapon");
        return path.find("palweapon") != std::string_view::npos;
    }
    void LoadObjectPackage() override { calls.emplace_back("object package"); }
    void LoadWeaponPackage() override { calls.emplace_back("weapon package"); }
    std::uint32_t CurrentScenePackage() override {
        calls.emplace_back("current scene");
        return scene_package;
    }
    void LoadScenePackage(std::uint32_t package) override {
        Check(package == scene_package, "scene package address");
        calls.emplace_back("scene package");
    }
    std::uint32_t InitializeBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && !path.empty() &&
              fourth == 3u && fifth == 4u, "base model arguments");
        calls.emplace_back("initialize model");
        return base_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("script cleanup");
    }
    std::uint32_t AttachMovementHelper(std::uint32_t helper) override {
        adapter_helper = helper;
        calls.emplace_back("attach helper");
        return base_result;
    }
    std::uint32_t DetachMovementHelper(std::uint32_t helper) override {
        adapter_helper = helper;
        calls.emplace_back("detach helper");
        return base_result;
    }
    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x4000u, "name address");
        calls.emplace_back("read name");
        return "Floor";
    }
    void ConstructSnapshot(std::uint32_t address) override {
        Check(address == snapshot, "snapshot address");
        calls.emplace_back("construct snapshot");
        if (fail_snapshot) throw std::runtime_error("snapshot failed");
    }
    void AssignSnapshotName(std::uint32_t address,
                            std::string_view name) override {
        Check(address == snapshot, "snapshot name address");
        snapshot_name = name;
        calls.emplace_back("assign name");
    }
    std::uint32_t SnapshotType(std::uint32_t address) override {
        Check(address == snapshot, "snapshot type address");
        calls.emplace_back("snapshot type");
        return snapshot_type;
    }
    std::uint32_t SetBaseVisible(std::uint32_t address,
                                std::uint32_t visible) override {
        Check(address == object && visible == 2u, "restore visibility");
        calls.emplace_back("set visible");
        return base_result;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write " + std::to_string(address));
    }
};

void CheckLifecycle() {
    Check(pal4::GobLocalFloor::TypeKey() == 1u &&
          pal4::GobLocalFloor::InteractionThreshold() == 100.0,
          "factory key and threshold");
    Runtime null_memory;
    null_memory.object_allocation = 0u;
    Check(pal4::GobLocalFloor::Create(null_memory) == 0u &&
          null_memory.calls.size() == 1u, "null allocation");
    Runtime runtime;
    Check(pal4::GobLocalFloor::Create(runtime) == Runtime::object &&
          runtime.calls == (std::vector<std::string>{
              "allocate 480", "construct base", "write 4572",
              "write 4096", "write 4572"}) &&
          runtime.words.at(Runtime::object) == pal4::GobLocalFloor::kVtable &&
          runtime.words.at(Runtime::object + 0x1dcu) ==
              pal4::GobLocalFloor::kHelperVtable,
          "constructor order and vtables");
    runtime.calls.clear();
    pal4::GobLocalFloor::Destruct(Runtime::object, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "write 4096", "write 4572", "write 4572", "destruct base"}) &&
          runtime.words.at(Runtime::object + 0x1dcu) ==
              pal4::GobLocalFloor::kHelperBaseVtable,
          "destructor order");
    Runtime failed;
    failed.fail_base = true;
    bool thrown = false;
    try { (void)pal4::GobLocalFloor::Create(failed); }
    catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate 480", "construct base", "free"}),
          "failed base construction frees allocation");
}

void CheckForwardersAndScene() {
    Runtime runtime;
    Check(pal4::GobLocalFloor::CleanupResources(
              Runtime::object, 9u, runtime) == 0x55u &&
          pal4::GobLocalFloor::RenderIfVisible(
              Runtime::object, 9u, runtime) == 0x55u &&
          pal4::GobLocalFloor::UpdateClumpIfVisible(
              0.25f, runtime) == 0x55u &&
          runtime.calls == (std::vector<std::string>{
              "cleanup base", "render base", "update clump"}),
          "base forwarders");
    for (const auto path : {"palobject/floor", "palweapon/floor",
                            "arena/floor"}) {
        Runtime scene;
        scene.scene_package = 0x4000u;
        Check(pal4::GobLocalFloor::InitializeSceneAndConfig(
                  Runtime::object, 2u, path, 3u, 4u, scene) == 0x55u &&
              scene.calls.front() == "preload cleanup" &&
              scene.calls[scene.calls.size() - 2u] == "initialize model" &&
              scene.calls.back() == "script cleanup", "scene call order");
        const auto expected = path[3] == 'o' ? "object package" :
            path[3] == 'w' ? "weapon package" : "scene package";
        Check(std::find(scene.calls.begin(), scene.calls.end(), expected) !=
                  scene.calls.end(), "selected scene package");
    }
    Runtime no_package;
    no_package.base_result = 0u;
    Check(pal4::GobLocalFloor::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/floor", 3u, 4u,
              no_package) == 0u &&
          std::find(no_package.calls.begin(), no_package.calls.end(),
                    "scene package") == no_package.calls.end() &&
          no_package.calls.back() == "script cleanup",
          "absent scene package and failed model");
}

void CheckAdapterAndSnapshot() {
    Runtime runtime;
    Check(pal4::GobLocalFloor::AttachMovementAdapter(
              Runtime::object, runtime) == 0x55u &&
          runtime.adapter_helper == Runtime::object + 0x1dcu,
          "adapter attach uses embedded helper");
    Check(pal4::GobLocalFloor::DetachMovementAdapter(
              Runtime::object, runtime) == 0x55u &&
          runtime.adapter_helper == Runtime::object + 0x1dcu,
          "adapter detach uses embedded helper");
    Check(pal4::GobLocalFloor::AttachMovementAdapter(
              0u, runtime) == 0x55u && runtime.adapter_helper == 0u &&
          pal4::GobLocalFloor::DetachMovementAdapter(
              0u, runtime) == 0x55u && runtime.adapter_helper == 0u,
          "adapter null forwarding");
    runtime.words[Runtime::object + 0x14u] = 0x9000u;
    Check(pal4::GobLocalFloor::HelperClump(
              Runtime::object + 0x1dcu, runtime) == 0x9000u,
          "helper retrieves outer clump");
    runtime.words[Runtime::object + 0x1a4u] = 0u;
    Check(pal4::GobLocalFloor::CreateStateSnapshot(
              Runtime::object, runtime) == 0u, "store gate");
    runtime.words[Runtime::object + 0x1a4u] = 1u;
    runtime.snapshot_allocation = 0u;
    Check(pal4::GobLocalFloor::CreateStateSnapshot(
              Runtime::object, runtime) == 0u, "snapshot allocation gate");
    runtime.snapshot_allocation = Runtime::snapshot;
    runtime.words[Runtime::object + 4u] = 0x4000u;
    runtime.words[Runtime::object + 0x184u] = 2u;
    runtime.calls.clear();
    Check(pal4::GobLocalFloor::CreateStateSnapshot(
              Runtime::object, runtime) == Runtime::snapshot &&
          runtime.snapshot_name == "Floor" &&
          runtime.words.at(Runtime::snapshot + 0x14u) == 2u &&
          runtime.calls == (std::vector<std::string>{
              "allocate 24", "construct snapshot", "read name",
              "assign name", "write 8212"}), "snapshot capture");
    runtime.calls.clear();
    Check(pal4::GobLocalFloor::ApplyStateSnapshot(
              Runtime::object, Runtime::snapshot, runtime) == 0x55u &&
          runtime.calls == (std::vector<std::string>{
              "snapshot type", "set visible"}), "type-1 restore");
    runtime.snapshot_type = 7u;
    runtime.calls.clear();
    Check(pal4::GobLocalFloor::ApplyStateSnapshot(
              Runtime::object, Runtime::snapshot, runtime) == 7u &&
          runtime.calls == (std::vector<std::string>{"snapshot type"}),
          "wrong snapshot type");
    runtime.words[Runtime::object + 0x1a4u] = 0u;
    runtime.calls.clear();
    Check(pal4::GobLocalFloor::ApplyStateSnapshot(
              Runtime::object, Runtime::snapshot, runtime) == 0u &&
          runtime.calls.empty(), "disabled snapshot restore");
    Runtime failed;
    failed.words[Runtime::object + 0x1a4u] = 1u;
    failed.fail_snapshot = true;
    bool thrown = false;
    try { (void)pal4::GobLocalFloor::CreateStateSnapshot(
              Runtime::object, failed); }
    catch (const std::runtime_error&) { thrown = true; }
    Check(thrown && failed.calls == (std::vector<std::string>{
              "allocate 24", "construct snapshot", "free"}),
          "snapshot constructor exception cleanup");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckForwardersAndScene();
        CheckAdapterAndSnapshot();
        std::cout << "gob_local_floor_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
