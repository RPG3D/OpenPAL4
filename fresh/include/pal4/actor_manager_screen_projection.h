#pragma once

#include <cstdint>

namespace pal4 {

struct ActorScreenPoint {
    float x;
    float y;
};

struct ActorWorldPoint {
    float x;
    float y;
    float z;
};

class IActorManagerScreenProjectionRuntime {
public:
    virtual ~IActorManagerScreenProjectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual ActorScreenPoint ProjectCamera(
        std::uint32_t camera, const ActorWorldPoint& point) = 0;
    [[nodiscard]] virtual ActorScreenPoint ConvertUIFrame(
        const ActorScreenPoint& point) = 0;
};

class ActorManagerScreenProjection final {
public:
    [[nodiscard]] static ActorScreenPoint Project(
        std::uint32_t actor, std::uint32_t camera,
        IActorManagerScreenProjectionRuntime& runtime);
};

}  // namespace pal4
