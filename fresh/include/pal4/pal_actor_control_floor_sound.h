#pragma once

#include "pal4/pal_actor_control_foot_effect.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorControlFloorText {
    std::uint32_t length{};
    std::string_view path{};
};

class IActorControlFloorSoundRuntime : public IActorControlFootEffectRuntime {
public:
    [[nodiscard]] virtual ActorControlFloorText CurrentFloorText();
    [[nodiscard]] virtual std::uint32_t ClassifyFloorSound(
        std::string_view path, PalActorControlState& state);
};

class PalActorControlFloorSound final {
public:
    // IDA 0x426530. The timer lives in controller word 224 (+0x380).
    [[nodiscard]] static std::uint32_t Tick(
        PalActorControlState& state, float delta_seconds,
        IActorControlFloorSoundRuntime& runtime);
};

}  // namespace pal4
