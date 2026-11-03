#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerPlayerEventRuntime {
public:
    virtual ~IActorManagerPlayerEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::int32_t Dispatch(
        std::uint32_t router, std::string_view event,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t word_count, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t DispatchByte(
        std::uint32_t router, std::string_view event,
        std::uint8_t payload, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot344(
        std::uint32_t actor) = 0;
};

class ActorManagerPlayerEventDispatch final {
public:
    [[nodiscard]] static std::int32_t AddMoney(
        std::uint32_t amount, IActorManagerPlayerEventRuntime& runtime);
    [[nodiscard]] static std::int32_t AddProperty(
        std::uint32_t actor, std::uint32_t property,
        std::uint32_t amount, IActorManagerPlayerEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SendAddProperty(
        std::uint32_t property, std::uint32_t amount,
        IActorManagerPlayerEventRuntime& runtime);
    [[nodiscard]] static std::int32_t SendAddMoney(
        std::uint32_t amount, IActorManagerPlayerEventRuntime& runtime);
    [[nodiscard]] static std::int32_t PickUpFail(
        std::uint32_t receiver,
        IActorManagerPlayerEventRuntime& runtime);
};

}  // namespace pal4
