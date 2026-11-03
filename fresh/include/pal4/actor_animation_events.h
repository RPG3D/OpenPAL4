#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorAnimationEventRuntime {
public:
    virtual ~IActorAnimationEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view TextAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool EqualIgnoringCase(
        std::string_view left, std::string_view right) = 0;
    virtual void SetAnimationFound(bool found) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::uint32_t router, std::uint32_t event_key,
        const std::array<std::uint32_t, 2>& payload,
        std::uint32_t mode) = 0;
};

class ActorAnimationEvents final {
public:
    [[nodiscard]] static std::uint32_t CompareNameAndFlag(
        std::uint32_t object, std::uint32_t search,
        IActorAnimationEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessMessage(
        std::uint32_t object, std::uint32_t unused,
        std::uint32_t message, IActorAnimationEventRuntime& runtime);
};

}  // namespace pal4
