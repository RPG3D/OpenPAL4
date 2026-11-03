#pragma once

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerParameterRuntime {
public:
    virtual ~IActorManagerPlayerParameterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ActorSlot48(
        std::uint32_t actor, std::uint32_t mode) = 0;
};
class IActorManagerPlayerForwarderRuntime : public IActorManagerPlayerParameterRuntime {
public:
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    virtual void Query37(std::uint32_t collection,
                         std::uint32_t output) = 0;
    virtual void Query38(std::uint32_t collection,
                         std::uint32_t output) = 0;
    virtual void FirstIterator(std::uint32_t manager,
                               std::uint32_t output) = 0;
    virtual void SecondIterator(std::uint32_t manager,
                                std::uint32_t output) = 0;
};

class ActorManagerPlayerForwarders final {
public:
    [[nodiscard]] static std::uint32_t SharedTextRecord(
        std::uint32_t ignored);
    [[nodiscard]] static std::uint32_t EmbeddedProperty144(
        std::uint32_t actor);
    [[nodiscard]] static std::uint32_t ForceActorModeOne(
        std::uint32_t actor, IActorManagerPlayerParameterRuntime& runtime);
    [[nodiscard]] static std::uint32_t Query37(
        std::uint32_t output, IActorManagerPlayerForwarderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Query38(
        std::uint32_t output, IActorManagerPlayerForwarderRuntime& runtime);
    [[nodiscard]] static std::uint32_t FirstIterator(
        std::uint32_t output, IActorManagerPlayerForwarderRuntime& runtime);
    [[nodiscard]] static std::uint32_t SecondIterator(
        std::uint32_t output, IActorManagerPlayerForwarderRuntime& runtime);
};

}  // namespace pal4
