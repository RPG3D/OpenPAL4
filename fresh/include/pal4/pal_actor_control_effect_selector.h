#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ActorControlEffectObject {
    std::array<std::uint32_t, 23> words{};
};
static_assert(sizeof(ActorControlEffectObject) == 0x5cu);

struct ActorControlEffectPoint {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class IActorControlEffectSelectorRuntime {
public:
    virtual ~IActorControlEffectSelectorRuntime() = default;
    virtual ActorControlEffectPoint Vertex(
        ActorControlEffectObject& effect, std::int32_t index);
    virtual ActorControlEffectPoint ActorPosition();
    virtual float VerticalTolerance();
    virtual float Magnitude(ActorControlEffectPoint planar_offset);
    virtual void SelectEffect(ActorControlEffectObject& effect);
};

class PalActorControlEffectSelector final {
public:
    // IDA 0x425080: callback returns the same effect pointer.
    [[nodiscard]] static ActorControlEffectObject* Visit(
        ActorControlEffectObject& effect, float& best_distance,
        IActorControlEffectSelectorRuntime& runtime);
};

}  // namespace pal4
