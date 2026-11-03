#pragma once

#include <cstdint>

namespace pal4 {

struct SkillSystemGlobals {
    std::uint32_t instance = 0u;
    std::uint8_t guard = 0u;
};

class ISkillSystemLifecycleRuntime {
public:
    virtual ~ISkillSystemLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t UninitializedHeaderByte() = 0;
    virtual void ConstructEventLogger(std::uint32_t object) = 0;
    virtual void CleanupEventLogger(std::uint32_t object) = 0;
    virtual void SetLogFile(std::uint32_t name, std::uint32_t mode) = 0;
    virtual void InitializeEventObjects() = 0;
    [[nodiscard]] virtual std::uint32_t TimeGetTime() = 0;
    virtual void SeedRandomNumberGenerator(std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SingletonManagerInstance() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t cleanup) = 0;
};

class SkillSystemLifecycle final {
public:
    static constexpr std::uint32_t kObjectSize = 0x938u;
    static constexpr std::uint32_t kSingletonName = 0x8b939cu;
    static constexpr std::uint32_t kSingletonId = 1033u;

    // IDA 0x58a010 / 0x58a050.
    [[nodiscard]] static std::uint32_t GetInstance(
        SkillSystemGlobals& globals, ISkillSystemLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        ISkillSystemLifecycleRuntime& runtime);

    // IDA 0x58a0b0 / 0x58a0d0 / 0x58a100.
    [[nodiscard]] static std::uint32_t CleanupResource(
        std::uint32_t object, ISkillSystemLifecycleRuntime& runtime);
    static void CleanupEventData(
        std::uint32_t object, ISkillSystemLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeEventSystem(
        std::uint32_t object, ISkillSystemLifecycleRuntime& runtime);
};

}  // namespace pal4
