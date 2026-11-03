#pragma once

#include "pal4/pal_actor_control.h"
#include "pal4/pal_actor_control_interaction.h"

#include <cstdint>

namespace pal4 {

class IActorControlDispatchRuntime : public IActorControlInteractionRuntime {
public:
    virtual ~IActorControlDispatchRuntime() = default;
    virtual bool HandlePlayerInteraction(PalActorControlState& state,
                                         std::uint32_t camera_internal);
    virtual bool TryMotion(std::uint32_t* motion,
                           std::uint32_t actor, float delta);
    virtual bool DatabaseHasPriority(std::uint32_t* database);
    virtual bool TryDatabase(std::uint32_t* database,
                             std::uint32_t actor, float delta);
    virtual void BindDispatchSubobject(std::uint32_t* component,
                                       std::uint32_t actor);
    virtual std::uint32_t SubobjectAddress(
        PalActorControlState& state, std::uint32_t word_offset);
};

class PalActorControlDispatch final {
public:
    // IDA 0x424ec0. The active subobject pointer is stored at word 183.
    [[nodiscard]] static bool Select(
        PalActorControlState& state, float delta,
        std::uint32_t actor, std::uint32_t camera_internal,
        IActorControlDispatchRuntime& runtime);
};

}  // namespace pal4
