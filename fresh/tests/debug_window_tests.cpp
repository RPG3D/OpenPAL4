#include "pal4/debug_window.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AccessRuntime final : public pal4::IDebugWindowAccessRuntime {
public:
    std::uint32_t GetSingletonManager() override {
        ++get_calls;
        return 0x500u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        ++register_calls;
        Check(manager == 0x500u && name == "palDebugWindow" &&
                  id == 1021u && factory == 0x418cf0u &&
                  destroy == 0x418d50u,
              "singleton registration matches IDA operands");
        return 0x600u;
    }
    int get_calls{};
    int register_calls{};
};

class ObjectRuntime final : public pal4::IDebugWindowRuntime {
public:
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("allocate");
        Check(size == 0x2cu, "object allocation is 44 bytes");
        return allocation;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "free receives object address");
        events.push_back("free");
    }
    pal4::DebugWindowState& Access(std::uint32_t address) override {
        Check(address == 0x1000u, "state lookup uses object address");
        return state;
    }
    void ConstructString(std::uint32_t address) override {
        events.push_back("construct:" + std::to_string(address));
        if (throw_on_second_constructor && address == 0x1020u) {
            throw std::runtime_error("construct");
        }
    }
    void ReallocateString(std::uint32_t address,
                          std::uint32_t capacity) override {
        events.push_back("reallocate:" + std::to_string(address));
        Check(capacity == 0x1000u, "both strings use recorded capacity");
        if (throw_on_second_reallocation && address == 0x1020u) {
            throw std::runtime_error("reallocate");
        }
    }
    void CleanupString(std::uint32_t address) override {
        events.push_back("cleanup:" + std::to_string(address));
        if (throw_on_second_cleanup && address == 0x1020u) {
            throw std::runtime_error("cleanup");
        }
    }
    std::uint32_t DestroyWindow(std::uint32_t handle) override {
        events.push_back("destroy:" + std::to_string(handle));
        return handle + 100u;
    }
    void KillTimer(std::uint32_t handle, std::uint32_t timer_id) override {
        Check(timer_id == 1u, "debug window kills timer 1");
        events.push_back("timer:" + std::to_string(handle));
    }
    std::uint32_t allocation{0x1000u};
    bool throw_on_second_constructor{};
    bool throw_on_second_reallocation{};
    bool throw_on_second_cleanup{};
    pal4::DebugWindowState state;
    std::vector<std::string> events;
};

void SingletonRegistrationCachesResult() {
    pal4::DebugWindowAccessState state{0x80u, 0u};
    AccessRuntime runtime;
    Check(pal4::DebugWindowAccess::Get(state, runtime) == 0x600u,
          "first lookup registers the singleton");
    Check(state.guard == 0x81u && state.instance == 0x600u &&
              runtime.get_calls == 1 && runtime.register_calls == 1,
          "guard and instance are cached");
    Check(pal4::DebugWindowAccess::Get(state, runtime) == 0x600u &&
              runtime.get_calls == 1 && runtime.register_calls == 1,
          "second lookup returns cached instance");
}

void CreateConstructsAndDestroyCleansInReverseOrder() {
    ObjectRuntime runtime;
    runtime.state.Bytes().fill(0xa5u);
    Check(pal4::DebugWindowObject::Create(runtime) == 0x1000u,
          "factory returns constructed address");
    Check(runtime.state.ReadDword(0u) == 0u &&
              runtime.state.ReadDword(4u) == 0u &&
              runtime.state.ReadDword(0x10u) == 0x1000u,
          "constructor sets both handles and capacity");
    Check(runtime.state.ReadDword(8u) == 0xa5a5a5a5u,
          "constructor preserves unobserved bytes");
    Check(runtime.events == std::vector<std::string>({
              "allocate", "construct:4116", "construct:4128",
              "reallocate:4116", "reallocate:4128"}),
          "factory performs two string constructions and reallocations");
    pal4::DebugWindowObject::Destroy(0x1000u, runtime);
    Check(runtime.events.back() == "free" &&
              runtime.events.at(5) == "cleanup:4128" &&
              runtime.events.at(6) == "cleanup:4116",
          "destructor cleans second string before first, then frees");
}

void AllocationAndConstructorFailure() {
    ObjectRuntime runtime;
    runtime.allocation = 0u;
    Check(pal4::DebugWindowObject::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "allocation failure stops before construction");
    runtime = ObjectRuntime{};
    runtime.throw_on_second_constructor = true;
    try {
        static_cast<void>(pal4::DebugWindowObject::Create(runtime));
        Check(false, "second string construction must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "construct:4116", "construct:4128",
                  "cleanup:4116", "free"}),
              "constructor failure cleans first string and frees object");
    }
    runtime = ObjectRuntime{};
    runtime.throw_on_second_reallocation = true;
    try {
        static_cast<void>(pal4::DebugWindowObject::Create(runtime));
        Check(false, "second string reallocation must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "construct:4116", "construct:4128",
                  "reallocate:4116", "reallocate:4128",
                  "cleanup:4128", "cleanup:4116", "free"}),
              "reallocation failure cleans both strings and frees object");
    }
}

void DestroyWindowsHandlesAllBranches() {
    ObjectRuntime runtime;
    Check(pal4::DebugWindowObject::DestroyWindows(0x1000u, runtime) == 0u &&
              runtime.events.empty(), "empty handles do nothing");
    runtime.state.WriteDword(0u, 7u);
    Check(pal4::DebugWindowObject::DestroyWindows(0x1000u, runtime) == 0u &&
              runtime.state.ReadDword(0u) == 0u &&
              runtime.events == std::vector<std::string>{"destroy:7"},
          "first window is destroyed without a timer");
    runtime.events.clear();
    runtime.state.WriteDword(0u, 8u);
    runtime.state.WriteDword(4u, 9u);
    Check(pal4::DebugWindowObject::DestroyWindows(0x1000u, runtime) == 109u &&
              runtime.state.ReadDword(0u) == 0u &&
              runtime.state.ReadDword(4u) == 0u,
          "both handles are cleared and second result is returned");
    Check(runtime.events == std::vector<std::string>({
              "destroy:8", "timer:9", "destroy:9"}),
          "two-window teardown follows IDA call order");
}

void DestructorUnwindsFirstStringAfterSecondThrows() {
    ObjectRuntime runtime;
    runtime.throw_on_second_cleanup = true;
    try {
        pal4::DebugWindowObject::Destruct(0x1000u, runtime);
        Check(false, "second-string cleanup must throw");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "cleanup:4128", "cleanup:4116"}),
              "destructor SEH path still cleans first string");
    }
}

}  // namespace

int main() {
    SingletonRegistrationCachesResult();
    CreateConstructsAndDestroyCleansInReverseOrder();
    AllocationAndConstructorFailure();
    DestroyWindowsHandlesAllBranches();
    DestructorUnwindsFirstStringAfterSecondThrows();
    std::cout << "5 debug window test cases passed\n";
}
