#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEfEffectSystemLifecycleRuntime {
public:
    virtual ~IEfEffectSystemLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destroy) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(std::uint32_t object) = 0;
    virtual void Destruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t object) = 0;
    virtual void ProcessScript() = 0;
    [[nodiscard]] virtual int CleanupScripts() = 0;
};

class EfEffectSystemLifecycleRuntimeStub
    : public IEfEffectSystemLifecycleRuntime {
public:
    [[nodiscard]] std::uint32_t SingletonManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t, std::string_view, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Construct(std::uint32_t) override { return 0u; }
    void Destruct(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    void ProcessScript() override {}
    [[nodiscard]] int CleanupScripts() override { return 0; }
};

class EfEffectSystemLifecycle final {
public:
    static constexpr std::uint32_t kObjectSize = 0xa8u;
    static constexpr std::uint32_t kSingletonId = 1037u;
    static constexpr std::string_view kSingletonName = "efEffectSystem";
    static constexpr std::uint32_t kFactoryAddress = 0x635d50u;
    static constexpr std::uint32_t kDestroyAddress = 0x635db0u;

    // IDA 0x635d10: guarded singleton registration.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& registered,
        IEfEffectSystemLifecycleRuntime& runtime);
    // IDA 0x635d50: allocate then construct the singleton instance.
    [[nodiscard]] static std::uint32_t Create(
        IEfEffectSystemLifecycleRuntime& runtime);
    // IDA 0x635db0: destruct and free a non-null instance.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IEfEffectSystemLifecycleRuntime& runtime);
    // IDA 0x635dd0: run the native script helper and return its owner.
    [[nodiscard]] static std::uint32_t ProcessScript(
        std::uint32_t owner, IEfEffectSystemLifecycleRuntime& runtime);
    // IDA 0x635de0: direct thunk to native script cleanup.
    [[nodiscard]] static int CleanupScripts(
        IEfEffectSystemLifecycleRuntime& runtime);
};

}  // namespace pal4
