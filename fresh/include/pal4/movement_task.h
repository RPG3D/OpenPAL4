#pragma once

#include "pal4/move_task.h"

#include <cstdint>

namespace pal4 {

class IMovementTaskRuntime : public virtual IMoveTaskRuntime {
public:
    virtual void SetOwnerRotation(std::uint32_t owner,
                                  const MoveTaskVector& rotation,
                                  std::uint32_t mode) = 0;
};

class IMovementTaskSpawnRuntime : public virtual IMovementTaskRuntime,
                                  public IMoveTaskSpawnRuntime {};

class MovementTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x8488ccu;
    // IDA 0x597ec0/0x5980e0.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IMovementTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IMovementTaskSpawnRuntime& runtime);
};

class MovementTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8488a8u;
    // IDA 0x597610/0x5976e0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IMovementTaskRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IMovementTaskRuntime& runtime);
    // IDA 0x597800/0x597af0.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IMovementTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta_seconds,
        IMovementTaskRuntime& runtime);
    // IDA 0x597bf0/0x597ed0/0x597ee0/0x597f00/0x597f20.
    [[nodiscard]] static std::uint8_t SpawnFollowup(
        std::uint32_t object, IMovementTaskSpawnRuntime& runtime);
    [[nodiscard]] static std::uint8_t IsComplete(
        std::uint32_t object, IMovementTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t StopSoundIfPlaying(
        std::uint32_t object, IMovementTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseSoundIfNotPlaying(
        std::uint32_t object, IMovementTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t duration_bits,
        const MoveTaskVector& target_position,
        const MoveTaskVector& target_rotation,
        std::uint32_t sound_name, std::uint32_t sound_time,
        std::uint32_t effect_name, std::uint32_t effect_time,
        std::uint32_t context, IMovementTaskRuntime& runtime);
};

}  // namespace pal4
