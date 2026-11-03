#include "pal4/game_database_lifecycle.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    bool operator==(const Call&) const = default;
};

struct SingletonRuntime final : pal4::IGameDatabaseSingletonRuntime {
    std::vector<Call> calls;
    std::uint32_t result = 0x9000u;
    bool throw_on_register = false;

    std::uint32_t GetSingletonManager() override {
        calls.push_back({"manager"});
        return 0x8000u;
    }
    std::uint32_t Register(const std::uint32_t manager, const char* name,
                           const std::uint32_t id,
                           const std::uint32_t factory,
                           const std::uint32_t destructor) override {
        Check(std::string(name) == "palGameDB" &&
                  manager == 0x8000u && id == 1025u &&
                  factory == 0x50c7b0u && destructor == 0x50c810u,
              "GameDB singleton registration identity");
        calls.push_back({"register", manager, id});
        if (throw_on_register) throw std::runtime_error("registration");
        return result;
    }
};

struct LifecycleRuntime final : pal4::IGameDatabaseLifecycleRuntime {
    std::vector<Call> calls;
    std::vector<std::uint32_t> allocations{0x1000u, 0x2000u, 0x3000u};
    std::size_t next_allocation = 0u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t assertion_logger_callback = 0u;

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        calls.push_back({"allocate", bytes});
        return allocations.at(next_allocation++);
    }
    std::uint32_t Free(const std::uint32_t address) override {
        calls.push_back({"free", address});
        return address + 1u;
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back({"write", address, value});
        words[address] = value;
    }
    void InitializeComponent(const std::uint32_t address,
                             const std::uint32_t initializer) override {
        calls.push_back({"initialize", address, initializer});
    }
    std::uint32_t InitializeManager(const std::uint32_t address) override {
        calls.push_back({"manager-init", address});
        return address + 0x10u;
    }
    void SetEffectGlobal(const std::uint32_t callback) override {
        calls.push_back({"effect-global", callback});
    }
    void SetGameDatabaseGlobal(const std::uint32_t callback) override {
        calls.push_back({"game-global", callback});
        static_cast<void>(pal4::GameDatabaseLifecycle::SetAssertionLoggerCallback(
            assertion_logger_callback, callback));
    }
    void BindManager(const std::uint32_t component,
                     const std::uint32_t manager) override {
        calls.push_back({"bind", component, manager});
    }
    void SetPropertyManager(const std::uint32_t property,
                            const std::uint32_t manager) override {
        calls.push_back({"property", property, manager});
    }
    void DestroyManager(const std::uint32_t address) override {
        calls.push_back({"manager-destroy", address});
    }
    void DestroyComponent(const std::uint32_t address) override {
        calls.push_back({"component-destroy", address});
    }
};

void GlobalAccessors() {
    std::uint32_t callback_slot = 0u;
    Check(pal4::GameDatabaseLifecycle::kAssertionLoggerSlot == 0x90f948u &&
              pal4::GameDatabaseLifecycle::SetAssertionLoggerCallback(
                  callback_slot, 0x50cd70u) == 0x50cd70u &&
              callback_slot == 0x50cd70u,
          "GameDB assertion callback setter stores and returns its argument");
    Check(pal4::GameDatabaseLifecycle::SetAssertionLoggerCallback(
              callback_slot, 0u) == 0u && callback_slot == 0u,
          "GameDB assertion callback setter accepts null replacement");
    Check(pal4::GlobalSystemAccess::GetInstanceAddress() == 0x90f148u,
          "global system accessor returns the static object's address");
}

void SingletonRegistersOnlyOnce() {
    pal4::GameDatabaseSingletonState state{0xa0u, 0u};
    pal4::GameDatabaseSingleton singleton(state);
    SingletonRuntime runtime;
    Check(singleton.GetInstance(runtime) == 0x9000u &&
              state.initialization_bits == 0xa1u &&
              state.instance == 0x9000u &&
              singleton.GetInstance(runtime) == 0x9000u &&
              runtime.calls == std::vector<Call>{{"manager"},
                                                  {"register", 0x8000u, 1025u}},
          "GameDB registration caches its instance and preserves guard bits");

    pal4::GameDatabaseSingletonState failure_state{};
    pal4::GameDatabaseSingleton failure(failure_state);
    SingletonRuntime throwing;
    throwing.throw_on_register = true;
    try {
        static_cast<void>(failure.GetInstance(throwing));
        Check(false, "registration should throw");
    } catch (const std::runtime_error&) {
        Check(failure_state.initialization_bits == 1u &&
                  failure_state.instance == 0u,
              "GameDB guard is set before the external registration call");
    }
}

void AliasForwardsToGameDatabaseSingleton() {
    pal4::GameDatabaseSingletonState state{};
    pal4::GameDatabaseSingleton singleton(state);
    SingletonRuntime runtime;
    Check(singleton.GetInstanceAlias(runtime) == 0x9000u &&
              singleton.GetInstance(runtime) == 0x9000u &&
              runtime.calls == std::vector<Call>{{"manager"},
                                                  {"register", 0x8000u, 1025u}},
          "GameDB alias forwards through the same singleton cache");
}

void ConstructorUsesExactComponentAndManagerOrder() {
    LifecycleRuntime runtime;
    Check(pal4::GameDatabaseLifecycle::CreateInstance(runtime) == 0x1000u,
          "GameDB factory returns the constructed 0x14c-byte object");
    const std::array<Call, 12> initializers{{
        {"initialize", 0x1000u, 0x7f0190u},
        {"initialize", 0x100cu, 0x4eee90u},
        {"initialize", 0x1020u, 0x5fc0a0u},
        {"initialize", 0x1034u, 0x4eee90u},
        {"initialize", 0x1058u, 0x4eee90u},
        {"initialize", 0x106cu, 0x4eee90u},
        {"initialize", 0x1080u, 0x7f0190u},
        {"initialize", 0x108cu, 0x40c350u},
        {"initialize", 0x1094u, 0x5fc0a0u},
        {"initialize", 0x10a4u, 0x40c350u},
        {"initialize", 0x10acu, 0x7f0190u},
        {"initialize", 0x10b8u, 0x4cfa30u},
    }};
    Check(runtime.calls.front() == Call{"allocate", 0x14cu},
          "GameDB factory allocation size");
    for (std::size_t i = 0; i < initializers.size(); ++i)
        Check(runtime.calls[i + 1u] == initializers[i],
              "GameDB component initialization address and order");

    const std::size_t managers = 13u;
    Check(runtime.calls[managers] == Call{"allocate", 0x20u} &&
              runtime.calls[managers + 1u] == Call{"manager-init", 0x2000u} &&
              runtime.calls[managers + 2u] == Call{"write", 0x1144u, 0x2010u} &&
              runtime.calls[managers + 3u] == Call{"allocate", 0x20u} &&
              runtime.calls[managers + 4u] == Call{"manager-init", 0x3000u} &&
              runtime.calls[managers + 5u] == Call{"write", 0x1148u, 0x3010u} &&
              runtime.calls[managers + 6u] == Call{"effect-global", 0x50cd40u} &&
              runtime.calls[managers + 7u] == Call{"game-global", 0x50cd70u},
          "GameDB manager creation and global callback order");
    constexpr std::array<std::uint32_t, 10> first_fields{
        0u, 12u, 32u, 52u, 88u, 128u, 140u, 148u, 164u, 172u};
    for (std::size_t i = 0; i < first_fields.size(); ++i)
        Check(runtime.calls[managers + 8u + i] ==
                  Call{"bind", 0x1000u + first_fields[i], 0x2010u},
              "GameDB first manager binding order");
    Check(runtime.calls[managers + 18u] ==
              Call{"property", 0x10b8u, 0x2010u} &&
              runtime.calls[managers + 19u] ==
                  Call{"bind", 0x106cu, 0x3010u} &&
              runtime.calls.size() == managers + 20u,
          "GameDB property and second manager bind last");
}

void FactoryAndManagerAllocationFailures() {
    LifecycleRuntime empty;
    empty.allocations = {0u};
    Check(pal4::GameDatabaseLifecycle::CreateInstance(empty) == 0u &&
              empty.calls == std::vector<Call>{{"allocate", 0x14cu}},
          "failed GameDB object allocation skips construction");

    LifecycleRuntime partial;
    partial.allocations = {0u, 0x3000u};
    Check(pal4::GameDatabaseLifecycle::Construct(0x1000u, partial) == 0x1000u &&
              partial.words[0x1144u] == 0u &&
              partial.words[0x1148u] == 0x3010u,
          "failed first manager allocation is stored as null");
    std::uint32_t initialized = 0u;
    for (const auto& call : partial.calls)
        if (call.name == "manager-init") ++initialized;
    Check(initialized == 1u,
          "only allocated GameDB managers are initialized");
}

void DestructorReleasesManagersBeforeReverseFields() {
    LifecycleRuntime runtime;
    runtime.words[0x1144u] = 0x2010u;
    runtime.words[0x1148u] = 0x3010u;
    Check(pal4::GameDatabaseLifecycle::DestroyInstance(
              0x1000u, runtime) == 0x1001u,
          "GameDB factory destructor forwards the final free result");
    const std::array<Call, 6> manager_calls{{
        {"read", 0x1144u}, {"manager-destroy", 0x2010u},
        {"free", 0x2010u}, {"read", 0x1148u},
        {"manager-destroy", 0x3010u}, {"free", 0x3010u},
    }};
    for (std::size_t i = 0; i < manager_calls.size(); ++i)
        Check(runtime.calls[i] == manager_calls[i],
              "GameDB managers are destroyed before components");
    constexpr std::array<std::uint32_t, 12> reverse_offsets{
        184u, 172u, 164u, 148u, 140u, 128u,
        108u, 88u, 52u, 32u, 12u, 0u};
    for (std::size_t i = 0; i < reverse_offsets.size(); ++i)
        Check(runtime.calls[manager_calls.size() + i] ==
                  Call{"component-destroy", 0x1000u + reverse_offsets[i]},
              "GameDB component destruction order is reversed");
    Check(runtime.calls.back() == Call{"free", 0x1000u} &&
              runtime.calls.size() == manager_calls.size() +
                                          reverse_offsets.size() + 1u,
          "GameDB object storage is freed last");

    LifecycleRuntime null_runtime;
    Check(pal4::GameDatabaseLifecycle::DestroyInstance(
              0u, null_runtime) == 0u && null_runtime.calls.empty(),
          "null GameDB destruction has no side effects");
}

}  // namespace

int main() {
    try {
        SingletonRegistersOnlyOnce();
        GlobalAccessors();
        AliasForwardsToGameDatabaseSingleton();
        ConstructorUsesExactComponentAndManagerOrder();
        FactoryAndManagerAllocationFailures();
        DestructorReleasesManagersBeforeReverseFields();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
