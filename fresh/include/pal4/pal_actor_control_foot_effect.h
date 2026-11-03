#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorControlFootEffectRuntime {
public:
    virtual ~IActorControlFootEffectRuntime() = default;
    virtual std::uint32_t EffectTransform(std::uint32_t linked_object);
    virtual std::uint32_t CreateEffect(std::string_view name,
                                       std::uint32_t transform,
                                       std::uint32_t first_flag,
                                       std::uint32_t second_flag);
    virtual void UpdateEffect(std::uint32_t effect,
                              std::uint32_t actor_position,
                              std::uint8_t option);
    virtual void ReleaseEffect(std::uint32_t effect);
};

class PalActorControlFootEffect final {
public:
    // IDA 0x4251b0. Mode 1/2/3 maps to foot_2/foot_1/foot_0.
    static void Spawn(PalActorControlState& state, std::uint32_t mode,
                      IActorControlFootEffectRuntime& runtime);
};

class PalActorControlFloorEffectClassifier final {
public:
    // IDA 0x425300. A matching floor code triggers one foot effect.
    static void Classify(std::string_view path,
                         PalActorControlState& state,
                         IActorControlFootEffectRuntime& runtime);
};

}  // namespace pal4
