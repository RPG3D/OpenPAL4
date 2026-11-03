#pragma once

#include "pal4/pal_actor_control.h"
#include "pal4/pal_actor_control_effect_selector.h"

#include <cstdint>

namespace pal4 {

class IActorControlDialogRuntime {
public:
    virtual ~IActorControlDialogRuntime() = default;
    virtual std::uint32_t IterateAndProcess(std::uint32_t actor);
    virtual std::uint32_t ReadWord(std::uint32_t address);
    virtual float ReadFloat(std::uint32_t address);
    virtual bool DialogEnabled(std::uint32_t dialog);
    virtual bool RestrictHeight(std::uint32_t dialog);
    virtual double ActorProperty(std::uint32_t actor);
    virtual float PlanarMagnitude(ActorControlEffectPoint offset);
    virtual void AddActiveItem(std::uint32_t dialog, float weight);
};

class PalActorControlDialog final {
public:
    // IDA 0x425c30. No observed reference-count write occurs here.
    [[nodiscard]] static std::uint32_t Release(
        PalActorControlState& state, IActorControlDialogRuntime& runtime);
    // IDA 0x425c60. The callback always returns its dialog address.
    [[nodiscard]] static std::uint32_t Visit(
        std::uint32_t dialog, std::uint32_t actor,
        IActorControlDialogRuntime& runtime);
};

}  // namespace pal4
