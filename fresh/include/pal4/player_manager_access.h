#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct PlayerManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPlayerManagerAccessRuntime {
public:
    virtual ~IPlayerManagerAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t registration_argument) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(std::uint32_t address) = 0;
};

class PlayerManagerAccessRuntimeStub : public IPlayerManagerAccessRuntime {
public:
    std::uint32_t SingletonManager() override;
    std::uint32_t Register(std::uint32_t, std::string_view,
        std::uint32_t, std::uint32_t, std::uint32_t) override;
    std::uint32_t Allocate(std::uint32_t) override;
    std::uint32_t Construct(std::uint32_t) override;
};

class PlayerManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e43a4u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e43a0u;
    static constexpr std::uint32_t kSingletonId = 0x40fu;
    static constexpr std::uint32_t kFactoryAddress = 0x440ec0u;
    static constexpr std::uint32_t kRegistrationArgument = 0x440f20u;
    static constexpr std::uint32_t kObjectBytes = 0x2c64u;

    // IDA 0x440e80.
    [[nodiscard]] static std::uint32_t Get(
        PlayerManagerAccessState& state,
        IPlayerManagerAccessRuntime& runtime);
    // IDA 0x440ec0.
    [[nodiscard]] static std::uint32_t CreateInstance(
        IPlayerManagerAccessRuntime& runtime);
};

}  // namespace pal4
