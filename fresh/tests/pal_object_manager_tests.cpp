#include "pal4/pal_object_manager.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalObjectManagerRuntime {
    std::vector<std::string> calls;
    std::string fail_at;
    bool allocation_fails{};

    void Record(const char* operation) {
        calls.emplace_back(operation);
        if (fail_at == operation) throw std::runtime_error(operation);
    }
    [[nodiscard]] std::uint32_t GetSingletonManager() override {
        Record("get-singleton");
        return 0x2000u;
    }
    [[nodiscard]] std::uint32_t Register(
        const std::uint32_t manager, const char* name, const std::uint32_t id,
        const std::uint32_t create, const std::uint32_t destroy) override {
        Check(manager == 0x2000u && std::string(name) == "palObjectManager" &&
                  id == 1022u && create == 0x5fc520u && destroy == 0x5fc580u,
              "singleton registration arguments");
        Record("register");
        return 0x3000u;
    }
    [[nodiscard]] std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 40u, "manager allocation size");
        Record("allocate");
        return allocation_fails ? 0u : 0x4000u;
    }
    [[nodiscard]] std::uint32_t Free(const std::uint32_t object) override {
        Check(object == 0x4000u, "manager free address");
        Record("free");
        return 0x55u;
    }
    void InitializeBase(const std::uint32_t object) override {
        Check(object == 0x4000u, "base initialization address");
        Record("base");
    }
    void ConstructLock(const std::uint32_t address) override {
        Check(address == 0x4020u, "reader-writer lock offset");
        Record("lock");
    }
    void SetReleaseCallback(const std::uint32_t object,
                            const std::uint32_t callback) override {
        Check(object == 0x4000u && callback == 0x7de5d0u,
              "release callback");
        Record("release-callback");
    }
    void SetCompareCallback(const std::uint32_t object,
                            const std::uint32_t callback) override {
        Check(object == 0x4000u && callback == 0x5fc630u,
              "compare callback");
        Record("compare-callback");
    }
    void SetCopyCallback(const std::uint32_t object,
                         const std::uint32_t callback) override {
        Check(object == 0x4000u && callback == 0x5eaf50u,
              "copy callback");
        Record("copy-callback");
    }
    void SetFreeCallback(const std::uint32_t object,
                         const std::uint32_t callback) override {
        Check(object == 0x4000u && callback == 0x5fc6c0u,
              "free callback");
        Record("free-callback");
    }
    void SetName(const std::uint32_t object, const char* name) override {
        Check(object == 0x4000u &&
                  std::string(name) == "object-manager-binPtrTree",
              "manager tree name");
        Record("name");
    }
    void DestructorThunk(const std::uint32_t object) override {
        Check(object == 0x4000u, "destructor thunk address");
        Record("destructor");
    }
};

void TestRegistration() {
    Runtime runtime;
    pal4::PalObjectManagerRegistration state{0u, 0x80u};
    Check(pal4::PalObjectManager::RegisterSingleton(state, runtime) == 0x3000u &&
              state.cached == 0x3000u && state.guard == 0x81u,
          "first registration caches instance and preserves guard bits");
    Check(pal4::PalObjectManager::RegisterSingleton(state, runtime) == 0x3000u &&
              runtime.calls == std::vector<std::string>{"get-singleton", "register"},
          "repeat registration uses cache");
}

void TestCreateAndDestroy() {
    Runtime runtime;
    Check(pal4::PalObjectManager::Create(runtime) == 0x4000u,
          "create returns constructed manager");
    Check(runtime.calls == std::vector<std::string>{
              "allocate", "base", "lock", "release-callback",
              "compare-callback", "copy-callback", "free-callback", "name"},
          "construction order");
    Check(pal4::PalObjectManager::Destroy(0x4000u, runtime) == 0x55u &&
              runtime.calls[runtime.calls.size() - 2u] == "destructor" &&
              runtime.calls.back() == "free", "destructor before free");
    const auto prior_count = runtime.calls.size();
    Check(pal4::PalObjectManager::Destroy(0u, runtime) == 0u &&
              runtime.calls.size() == prior_count, "null destruction is inert");
    pal4::PalObjectManager::ForwardDestructor(0x4000u, runtime);
    Check(runtime.calls.back() == "destructor", "direct thunk forwards");
    Check(pal4::PalObjectManager::FreeThunk(0x4000u, runtime) == 0x55u &&
              runtime.calls.back() == "free", "free thunk forwards result");
}

void TestAllocationFailure() {
    Runtime runtime;
    runtime.allocation_fails = true;
    Check(pal4::PalObjectManager::Create(runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "failed allocation stops construction");
}

void TestConstructionFailure() {
    for (const auto& operation : {"lock", "release-callback",
                                  "compare-callback", "name"}) {
        Runtime runtime;
        runtime.fail_at = operation;
        bool threw = false;
        try {
            (void)pal4::PalObjectManager::Create(runtime);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Check(threw && runtime.calls.size() >= 4u &&
                  runtime.calls[runtime.calls.size() - 2u] == "destructor" &&
                  runtime.calls.back() == "free",
              "constructed base destroyed before failed allocation is freed");
    }
}

}  // namespace

int main() {
    TestRegistration();
    TestCreateAndDestroy();
    TestAllocationFailure();
    TestConstructionFailure();
}
