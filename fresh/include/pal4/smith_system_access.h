#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISmithSystemRegistrationRuntime {
public:
    virtual ~ISmithSystemRegistrationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::string_view name, std::uint32_t id,
        std::uint32_t factory, std::uint32_t type_info) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(std::uint32_t memory) = 0;
    virtual void Free(std::uint32_t memory) = 0;
};

class ISmithUiEventRuntime {
public:
    virtual ~ISmithUiEventRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    [[nodiscard]] virtual std::uint8_t DispatchEvent(
        std::uint32_t manager, std::uint32_t descriptor,
        std::uint32_t first, std::uint32_t second) = 0;
};

class SmithSystemAccess final {
public:
    static constexpr std::uint32_t kFactory = 0x590790u;
    static constexpr std::uint32_t kTypeInfo = 0x428c00u;

    // IDA 0x590750/0x590790.
    [[nodiscard]] static std::uint32_t Register(
        std::uint8_t& guard, std::uint32_t& cached,
        ISmithSystemRegistrationRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        ISmithSystemRegistrationRuntime& runtime);
};

class SmithUiEvents final {
public:
    static constexpr std::uint32_t kFlagEvent = 0x8b0f1cu;
    static constexpr std::uint32_t kStateEvent = 0x8b13a8u;

    // IDA 0x5907f0/0x590830/0x590860.
    [[nodiscard]] static std::uint8_t SetFlag(
        std::uint32_t system, std::uint8_t value,
        ISmithUiEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t ClearFlags(
        ISmithUiEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetState(
        std::uint32_t unused, ISmithUiEventRuntime& runtime);
};

}  // namespace pal4
