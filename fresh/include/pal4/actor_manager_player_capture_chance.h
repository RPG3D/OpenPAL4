#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerCaptureRuntime {
public:
    virtual ~IActorManagerPlayerCaptureRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot144(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t TargetSlot168(
        std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t Database() = 0;
    [[nodiscard]] virtual float FloatElement(
        std::uint32_t values, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
};

class ActorManagerPlayerCaptureChance final {
public:
    [[nodiscard]] static bool Check(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerPlayerCaptureRuntime& runtime);
};

}  // namespace pal4
