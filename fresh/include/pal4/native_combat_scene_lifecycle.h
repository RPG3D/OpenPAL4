#pragma once
#include <cstdint>

namespace pal4 {
// Lifecycle of the existing registered Combat owner. Implementations retain
// the request and saved flags; PALGameIV owns the surrounding state exchange.
// Previous/next state IDs are not consumed by original58ADB0/58B2D0.
class INativeCombatSceneLifecycle {
public:
    virtual ~INativeCombatSceneLifecycle() = default;
    virtual std::uint32_t Address() const = 0;
    virtual bool Enter(std::uint32_t document) = 0;
    virtual void Leave() = 0;
};
}
