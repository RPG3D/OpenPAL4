#include "pal4/gob_normal.h"
#include "gob_normal_test_runtime.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGobNormalRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    NormalSnapshotRuntime snapshot;
    std::uint32_t allocation = object;
    std::uint32_t model_result = 7u;
    std::uint32_t scene_package = 0u;
    std::uint32_t interpreter = 0x3000u;
    std::uint32_t validation = 0u;
    bool fail_construct = false;
    bool cached = false;
    bool registered = true;
    std::string entry = "OnTouch";
    std::string name = "Normal";

    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 0x1dcu, "normal allocation size");
        calls.emplace_back("allocate");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == object, "normal free address");
        calls.emplace_back("free");
    }
    void ConstructBase(std::uint32_t address) override {
        Check(address == object, "construct base address");
        calls.emplace_back("construct base");
        if (fail_construct) throw std::runtime_error("base failed");
    }
    void DestructBase(std::uint32_t address) override {
        Check(address == object, "destruct base address");
        calls.emplace_back("destruct base");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write word");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read word");
        return words[address];
    }
    std::string_view ReadCString(std::uint32_t address) override {
        calls.emplace_back("read string");
        if (address == 0x5000u) return entry;
        if (address == 0x6000u) return name;
        throw std::runtime_error("unknown string address");
    }
    void CleanupBeforeSceneLoad(std::uint32_t address) override {
        Check(address == object, "scene cleanup address");
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
    void LoadObjectPackage() override { calls.emplace_back("load object"); }
    void LoadWeaponPackage() override { calls.emplace_back("load weapon"); }
    std::uint32_t CurrentScenePackage() override {
        calls.emplace_back("scene package");
        return scene_package;
    }
    void LoadScenePackage(std::uint32_t package) override {
        Check(package == 0x7000u, "scene package address");
        calls.emplace_back("load scene");
    }
    std::uint32_t InitializeBaseModel(std::uint32_t address,
        std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth) override {
        Check(address == object && first == 2u && fourth == 3u &&
              fifth == 4u && !path.empty(), "model args");
        calls.emplace_back("model");
        return model_result;
    }
    void CleanupScriptPackages() override {
        calls.emplace_back("cleanup packages");
    }
    void InitializeScript() override { calls.emplace_back("script init"); }
    std::string CurrentSceneScriptPath() override {
        calls.emplace_back("script path");
        return "scene.cs";
    }
    bool ScriptCacheContains(std::string_view key) override {
        Check(key == "scene.cs[OnTouch]", "script cache key");
        calls.emplace_back("cache check");
        return cached;
    }
    std::uint32_t CreateScriptInterpreter(std::string_view path,
        std::string_view key, std::string_view script_entry) override {
        Check(path == "scene.cs" && key == "scene.cs[OnTouch]" &&
              script_entry == "OnTouch", "interpreter arguments");
        calls.emplace_back("create interpreter");
        return interpreter;
    }
    bool AddScriptModule(std::uint32_t address) override {
        Check(address == interpreter, "execute interpreter address");
        calls.emplace_back("add module");
        return registered;
    }
    void PushStackModule(std::uint32_t address) override {
        Check(address == interpreter, "stack interpreter address");
        calls.emplace_back("push stack");
    }
    void ReleaseInterpreter(std::uint32_t address) override {
        Check(address == interpreter, "release address");
        calls.emplace_back("release interpreter");
    }
    void ShowScriptNotFound() override { calls.emplace_back("not found"); }
    std::uint32_t CleanupScriptPath() override {
        calls.emplace_back("cleanup path");
        return 0x88u;
    }
    std::uint32_t ConsumeResearchUse(std::uint32_t address) override {
        Check(address == object, "consume object address");
        calls.emplace_back("consume use");
        return 0x99u;
    }
    pal4::IGobSnapshotRuntime& SnapshotRuntime() override {
        calls.emplace_back("snapshot runtime");
        return snapshot;
    }
    std::uint32_t ValidateSnapshot(std::uint32_t address) override {
        Check(address == NormalSnapshotRuntime::snapshot, "snapshot address");
        calls.emplace_back("validate snapshot");
        return validation;
    }
    std::uint32_t SetVisibility(std::uint32_t address,
                                std::uint32_t visible) override {
        Check(address == object && visible == 3u, "visibility args");
        calls.emplace_back("set visibility");
        return 0x77u;
    }
};

void CheckLifecycle() {
    Runtime runtime;
    Check(pal4::GobNormal::Create(runtime) == Runtime::object &&
          runtime.words[Runtime::object] == pal4::GobNormal::kVtable &&
          runtime.calls == (std::vector<std::string>{
              "allocate", "construct base", "write word"}) &&
          pal4::GobNormal::ValidateScript() == 0u,
          "normal construction and zero thunk");
    runtime.calls.clear();
    pal4::GobNormal::Destruct(Runtime::object, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "write word", "destruct base"}), "normal destructor order");
    Runtime null;
    null.allocation = 0;
    Check(pal4::GobNormal::Create(null) == 0u &&
          null.calls == (std::vector<std::string>{"allocate"}),
          "normal null allocation");
    Runtime failing;
    failing.fail_construct = true;
    try {
        (void)pal4::GobNormal::Create(failing);
        throw std::runtime_error("missing constructor exception");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "base failed" &&
              failing.calls == (std::vector<std::string>{
                  "allocate", "construct base", "free"}),
              "normal constructor exception cleanup");
    }
}

void CheckScene() {
    const std::vector<std::pair<std::string, std::vector<std::string>>> routes{
        {"palobject/palweapon", {"preload cleanup", "check object",
             "load object", "model", "cleanup packages"}},
        {"palweapon", {"preload cleanup", "check object",
             "check weapon", "load weapon", "model", "cleanup packages"}},
        {"scene", {"preload cleanup", "check object", "check weapon",
             "scene package", "load scene", "model", "cleanup packages"}}
    };
    for (const auto& [path, calls] : routes) {
        Runtime runtime;
        runtime.scene_package = 0x7000u;
        Check(pal4::GobNormal::InitializeSceneAndConfig(
                  Runtime::object, 2, path, 3, 4, runtime) == 7u &&
              runtime.calls == calls, "normal package route and order");
    }
    Runtime empty;
    empty.model_result = 0u;
    Check(pal4::GobNormal::InitializeSceneAndConfig(
              Runtime::object, 2, "scene", 3, 4, empty) == 0u &&
          empty.calls == (std::vector<std::string>{
              "preload cleanup", "check object", "check weapon",
              "scene package", "model", "cleanup packages"}),
          "missing scene package and failed model cleanup");
}

void CheckScript() {
    Runtime disabled;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, disabled) == 0u &&
          disabled.calls == (std::vector<std::string>{"read word"}),
          "zero use count bypasses script and consumption");
    Runtime no_entry;
    no_entry.words[Runtime::object + 0x198u] = 1;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, no_entry) == 0x99u &&
          no_entry.calls == (std::vector<std::string>{
              "read word", "read word", "consume use"}),
          "empty entry still consumes use");
    Runtime cached;
    cached.words[Runtime::object + 0x198u] = 1;
    cached.words[Runtime::object + 0x17cu] = 7;
    cached.words[Runtime::object + 0x178u] = 0x5000u;
    cached.cached = true;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, cached) == 0x88u &&
          cached.calls == (std::vector<std::string>{
              "read word", "read word", "script init", "script path",
              "read word", "read string", "cache check",
              "cleanup packages", "cleanup path"}),
          "cache hit skips interpreter and consumption");
    Runtime registered;
    registered.words[Runtime::object + 0x198u] = 1;
    registered.words[Runtime::object + 0x17cu] = 7;
    registered.words[Runtime::object + 0x178u] = 0x5000u;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, registered) == 0x99u &&
          registered.calls == (std::vector<std::string>{
              "read word", "read word", "script init", "script path",
              "read word", "read string", "cache check", "create interpreter",
              "add module", "push stack", "release interpreter",
              "cleanup packages", "cleanup path", "consume use"}),
          "module registration, stack push and consumption order");
    Runtime not_found;
    not_found.words[Runtime::object + 0x198u] = 1;
    not_found.words[Runtime::object + 0x17cu] = 7;
    not_found.words[Runtime::object + 0x178u] = 0x5000u;
    not_found.interpreter = 0;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, not_found) == 0x99u &&
          not_found.calls == (std::vector<std::string>{
              "read word", "read word", "script init", "script path",
              "read word", "read string", "cache check", "create interpreter",
              "not found", "cleanup packages", "cleanup path", "consume use"}),
          "missing interpreter branch");
    Runtime rejected;
    rejected.words[Runtime::object + 0x198u] = 1;
    rejected.words[Runtime::object + 0x17cu] = 7;
    rejected.words[Runtime::object + 0x178u] = 0x5000u;
    rejected.registered = false;
    Check(pal4::GobNormal::RunScriptAndConsumeUse(
              Runtime::object, rejected) == 0x99u &&
          rejected.calls[9] == "not found", "rejected module registration branch");
}

void CheckSnapshot() {
    Runtime runtime;
    Check(pal4::GobNormal::CreateStateSnapshot(
              Runtime::object, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"read word"}),
          "store disabled capture gate");
    runtime.words[Runtime::object + 0x1a4u] = 1;
    runtime.words[Runtime::object + 4u] = 0x6000u;
    runtime.words[Runtime::object + 0x184u] = 3;
    runtime.calls.clear();
    Check(pal4::GobNormal::CreateStateSnapshot(
              Runtime::object, runtime) == NormalSnapshotRuntime::snapshot &&
          runtime.snapshot.name == "Normal" &&
          runtime.snapshot.words[NormalSnapshotRuntime::snapshot + 0x14u] == 3u,
          "snapshot captures name and visibility");
    runtime.snapshot.words[NormalSnapshotRuntime::snapshot + 0x14u] = 3u;
    runtime.calls.clear();
    Check(pal4::GobNormal::ApplyStateSnapshot(
              Runtime::object, NormalSnapshotRuntime::snapshot, runtime) ==
              0x77u && runtime.calls == (std::vector<std::string>{
                  "read word", "validate snapshot", "snapshot runtime",
                  "set visibility"}), "valid snapshot dispatches visibility");
    runtime.validation = 5u;
    runtime.calls.clear();
    Check(pal4::GobNormal::ApplyStateSnapshot(
              Runtime::object, NormalSnapshotRuntime::snapshot, runtime) ==
              5u && runtime.calls == (std::vector<std::string>{
                  "read word", "validate snapshot"}),
          "invalid snapshot blocks visibility");
    runtime.words[Runtime::object + 0x1a4u] = 0;
    runtime.calls.clear();
    Check(pal4::GobNormal::ApplyStateSnapshot(
              Runtime::object, NormalSnapshotRuntime::snapshot, runtime) ==
              0u && runtime.calls == (std::vector<std::string>{"read word"}),
          "disabled snapshot application");
    Runtime no_memory;
    no_memory.words[Runtime::object + 0x1a4u] = 1;
    no_memory.snapshot.allocation = 0;
    Check(pal4::GobNormal::CreateStateSnapshot(
              Runtime::object, no_memory) == 0u,
          "snapshot allocation failure");
}

}  // namespace

int main() {
    try {
        CheckLifecycle();
        CheckScene();
        CheckScript();
        CheckSnapshot();
        std::cout << "gob_normal_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
