#include "pal4/gob_marker.h"
#include "gob_marker_test_runtime.h"

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

struct Runtime final : pal4::IGobMarkerRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    MarkerSnapshotRuntime snapshot;
    std::string object_name = "Waypoint";
    std::uint32_t allocation = object;
    std::uint32_t model_result = 0x66u;
    std::uint32_t scene_package = 0u;
    bool throw_construct = false;

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x1dcu, "marker allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "marker free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "marker base constructor");
        calls.emplace_back("construct base");
        if (throw_construct) throw std::runtime_error("construct failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "marker base destructor");
        calls.emplace_back("destruct base");
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "marker preload cleanup");
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
        calls.emplace_back("scene package");
        return scene_package;
    }
    void LoadScenePackage(std::uint32_t package) override {
        Check(package == 0x7000u, "scene package address");
        calls.emplace_back("load scene package");
    }
    std::uint32_t InitializeBaseModel(
        std::uint32_t address, std::uint32_t first,
        std::string_view path, std::uint32_t fourth,
        std::uint32_t fifth) override {
        Check(address == object && first == 2u && fourth == 3u &&
              fifth == 4u && !path.empty(), "base model arguments");
        calls.emplace_back("initialize base model");
        return model_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup scripts");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address - object));
    }
    std::string_view ReadCString(std::uint32_t address) override {
        Check(address == 0x5000u, "marker name pointer");
        calls.emplace_back("read name");
        return object_name;
    }
    pal4::IGobSnapshotRuntime& SnapshotRuntime() override {
        calls.emplace_back("snapshot runtime");
        return snapshot;
    }
};

bool Contains(const std::vector<std::string>& calls,
              const std::string& value) {
    return std::find(calls.begin(), calls.end(), value) != calls.end();
}

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobMarker::Create(runtime) == Runtime::object &&
          runtime.words[Runtime::object] == pal4::GobMarker::kVtable &&
          runtime.words[Runtime::object + 0x170u] == 0u &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "construct base", "write 0", "write 368"}) &&
          pal4::GobMarker::TypeKey() == 9u,
          "marker constructor and type key");
    runtime.calls.clear();
    pal4::GobMarker::Destruct(Runtime::object, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "write 0", "destruct base"}),
          "marker destructor order");
    Runtime null;
    null.allocation = 0u;
    Check(pal4::GobMarker::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "marker null allocation");
    Runtime throwing;
    throwing.throw_construct = true;
    try {
        (void)pal4::GobMarker::Create(throwing);
        throw std::runtime_error("missing constructor exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "construct failed" &&
              throwing.calls == (std::vector<std::string>{
                  "allocate", "construct base", "free"}),
              "base exception frees marker allocation");
    }
}

void CheckSceneInitialization() {
    for (const auto path : {"palobject/marker", "palweapon/marker",
                            "arena/marker"}) {
        Runtime runtime;
        runtime.scene_package = 0x7000u;
        Check(pal4::GobMarker::InitializeSceneAndConfig(
                  Runtime::object, 2u, path, 3u, 4u, runtime) == 0x66u &&
              runtime.calls.front() == "preload cleanup" &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "initialize base model" &&
              runtime.calls.back() == "cleanup scripts",
              "scene initialization and cleanup order");
        const auto expected = path[3] == 'o' ? "load object package" :
            path[3] == 'w' ? "load weapon package" :
                             "load scene package";
        Check(Contains(runtime.calls, expected), "scene package route");
    }
    Runtime no_scene;
    Check(pal4::GobMarker::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/marker", 3u, 4u,
              no_scene) == 0x66u &&
          !Contains(no_scene.calls, "load scene package"),
          "missing scene package skips load");
    Runtime failed;
    failed.model_result = 0u;
    Check(pal4::GobMarker::InitializeSceneAndConfig(
              Runtime::object, 2u, "arena/marker", 3u, 4u,
              failed) == 0u &&
          failed.calls.back() == "cleanup scripts",
          "failed base model still cleans scripts");
}

void CheckSnapshot() {
    Runtime disabled;
    Check(pal4::GobMarker::CreateStateSnapshot(
              Runtime::object, disabled) == 0u &&
          disabled.calls.empty() && disabled.snapshot.calls.empty(),
          "snapshot gate at +0x1a4");
    for (const auto name : {"", "Waypoint", "LongMarkerNameLongerThanThirtyOneCharacters"}) {
        Runtime runtime;
        runtime.words[Runtime::object + 0x1a4u] = 1u;
        runtime.words[Runtime::object + 4u] = 0x5000u;
        runtime.object_name = name;
        Check(pal4::GobMarker::CreateStateSnapshot(
                  Runtime::object, runtime) ==
                  MarkerSnapshotRuntime::snapshot &&
              runtime.snapshot.name == name &&
              runtime.snapshot.calls.front() == "allocate" &&
              runtime.snapshot.calls.back() == "name assign",
              "snapshot copies object name");
    }
    Runtime null;
    null.words[Runtime::object + 0x1a4u] = 1u;
    null.snapshot.allocation = 0u;
    Check(pal4::GobMarker::CreateStateSnapshot(
              Runtime::object, null) == 0u &&
          null.snapshot.calls == (std::vector<std::string>{"allocate"}),
          "snapshot allocation failure");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckSceneInitialization();
        CheckSnapshot();
        std::cout << "gob_marker_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
