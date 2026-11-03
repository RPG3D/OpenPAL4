#pragma once

#include <cstdint>

namespace pal4 {

class IGlobalDataSubsystemRuntime {
public:
    virtual ~IGlobalDataSubsystemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t cleanup) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object) = 0;
    virtual void Cleanup(std::uint32_t object) = 0;
    virtual void FreeMemory(std::uint32_t object) = 0;
};

class GlobalDataSubsystemRegistration final {
public:
    static constexpr std::uint32_t kNameAddress = 0x8bdee0u;
    static constexpr std::uint32_t kSingletonId = 1047u;
    static constexpr std::uint32_t kFactoryAddress = 0x5ccdf0u;
    static constexpr std::uint32_t kCleanupAddress = 0x5cce50u;
    static constexpr std::uint32_t kObjectSize = 0xc4u;

    // IDA 0x5ccdb0 / 0x5ccdf0 / 0x5cce50.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint8_t& guard, std::uint32_t& cached_instance,
        IGlobalDataSubsystemRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IGlobalDataSubsystemRuntime& runtime);
    static void Destroy(std::uint32_t object,
                        IGlobalDataSubsystemRuntime& runtime);
};

}  // namespace pal4
