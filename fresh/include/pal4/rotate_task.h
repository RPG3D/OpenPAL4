#pragma once

#include "pal4/move_task.h"

#include <cstdint>

namespace pal4 {

class IRotateTaskRuntime : public virtual IMoveTaskRuntime {
public:
    virtual void SetOwnerRotation(std::uint32_t owner,
                                  const MoveTaskVector& rotation,
                                  std::uint32_t mode) = 0;
};

class IRotateTaskSpawnRuntime : public virtual IRotateTaskRuntime,
                                public IMoveTaskSpawnRuntime {};

class RotateTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x84893cu;
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IRotateTaskSpawnRuntime& runtime);
};

class RotateTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848918u;
    // IDA 0x598380..0x598da0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IRotateTaskRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta_seconds,
        IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnReverseAndFollowup(
        std::uint32_t object, IRotateTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopSoundIfPlaying(
        std::uint32_t object, IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseSoundIfNotPlaying(
        std::uint32_t object, IRotateTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t duration_bits, const MoveTaskVector& target_rotation,
        std::uint32_t sound_name, std::uint32_t sound_time,
        std::uint32_t effect_name, std::uint32_t effect_time,
        std::uint32_t context, IRotateTaskRuntime& runtime);
};

}  // namespace pal4
