#pragma once

#include <cstdint>

namespace pal4 {

struct CombatSystemSingletonState {
    std::uint8_t initialization_bits = 0u;
    std::uint32_t instance = 0u;
};

class ICombatSystemSingletonRuntime {
public:
    virtual ~ICombatSystemSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t destructor) = 0;
};

class CombatSystemSingleton final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8fa1dcu;
    static constexpr std::uint32_t kInstanceAddress = 0x8fa1d8u;
    static constexpr std::uint32_t kFactoryAddress = 0x574f50u;
    static constexpr std::uint32_t kDestructorAddress = 0x574fb0u;

    explicit CombatSystemSingleton(CombatSystemSingletonState& state)
        : state_(state) {}

    // IDA 0x574f10.
    [[nodiscard]] std::uint32_t GetInstance(
        ICombatSystemSingletonRuntime& runtime);

private:
    CombatSystemSingletonState& state_;
};

class ICombatSystemLifecycleRuntime {
public:
    virtual ~ICombatSystemLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(std::uint32_t object) = 0;
    virtual void Destruct(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t object) = 0;
};

class CombatSystemLifecycle final {
public:
    static constexpr std::uint32_t kObjectBytes = 0x208u;

    // IDA 0x574f50 and 0x574fb0.
    [[nodiscard]] static std::uint32_t Create(
        ICombatSystemLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, ICombatSystemLifecycleRuntime& runtime);
};

}  // namespace pal4
