#pragma once

#include <cstdint>

namespace pal4 {

class IActorPropertyCountCallbackRuntime {
public:
    virtual ~IActorPropertyCountCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t OwnerAllows(
        std::uint32_t owner, std::uint32_t virtual_offset,
        std::int32_t first, std::int32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t OwnerValue(
        std::uint32_t owner, std::uint32_t virtual_offset,
        std::int32_t first, std::int32_t mode) = 0;
};

class ActorPropertyCountCallbacks final {
public:
    // IDA 0x56d9b0/0x56d9e0.
    [[nodiscard]] static std::uint8_t AllowsExceptThreeFour(
        std::uint32_t object, std::int32_t first,
        std::int32_t mode, IActorPropertyCountCallbackRuntime& runtime);
    [[nodiscard]] static std::uint8_t AllowsExceptTwoFour(
        std::uint32_t object, std::int32_t first,
        std::int32_t mode, IActorPropertyCountCallbackRuntime& runtime);

    // IDA 0x56da10/0x56da20/0x56da50/0x56da90.
    [[nodiscard]] static std::int32_t Constant98();
    [[nodiscard]] static std::int32_t ReadOwnerValue(
        std::uint32_t object, std::int32_t first,
        std::int32_t mode, IActorPropertyCountCallbackRuntime& runtime);
    [[nodiscard]] static std::int32_t Constant99();
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IActorPropertyCountCallbackRuntime& runtime);
};

}  // namespace pal4
