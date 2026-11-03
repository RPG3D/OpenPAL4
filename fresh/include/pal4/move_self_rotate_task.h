#pragma once

#include "pal4/move_task.h"

#include <cstdint>

namespace pal4 {

class IMoveSelfRotateRuntime : public virtual IMoveTaskRuntime {
public:
    // RenderWare GetObjectTransform/frame-matrix access stub.
    [[nodiscard]] virtual MoveTaskVector FrameForwardVector(
        std::uint32_t) { return {}; }
    virtual void SetEffectProperties(std::uint32_t handle,
                                     std::uint32_t properties) = 0;
    [[nodiscard]] virtual MoveTaskVector UpdateFrameRotation(
        std::uint32_t owner, const MoveTaskVector& delta) = 0;
    virtual void RotateOwner(std::uint32_t owner,
                             const MoveTaskVector& rotation,
                             std::uint32_t mode) = 0;
};

class IMoveSelfRotateSpawnRuntime : public virtual IMoveSelfRotateRuntime,
                                    public IMoveTaskSpawnRuntime {};

class MoveSelfRotateTaskFactory final {
public:
    static constexpr std::uint32_t kDescriptorVtable = 0x8488a4u;
    // IDA 0x597360/0x597580.
    [[nodiscard]] static std::uint32_t ConstructDescriptor(
        std::uint32_t descriptor, IMoveSelfRotateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        IMoveSelfRotateSpawnRuntime& runtime);
};

class MoveSelfRotateTask final {
public:
    static constexpr std::uint32_t kVtable = 0x848880u;
    static constexpr std::uint32_t kEffectPosition = 0x84b1dcu;
    // IDA 0x596a50/0x596b20.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IMoveSelfRotateRuntime& runtime);
    static void Destruct(std::uint32_t object,
                         IMoveSelfRotateRuntime& runtime);
    // IDA 0x596c40/0x596e90/0x597060.
    [[nodiscard]] static std::uint8_t Enter(
        std::uint32_t object, IMoveSelfRotateRuntime& runtime);
    [[nodiscard]] static std::uint8_t Update(
        std::uint32_t object, float delta_seconds,
        IMoveSelfRotateRuntime& runtime);
    [[nodiscard]] static std::uint8_t SpawnReverseAndFollowup(
        std::uint32_t object, IMoveSelfRotateSpawnRuntime& runtime);
    // IDA 0x597370/0x597390/0x5973b0.
    [[nodiscard]] static std::uint8_t StopSoundIfPlaying(
        std::uint32_t object, IMoveSelfRotateRuntime& runtime);
    [[nodiscard]] static std::uint8_t PauseSoundIfNotPlaying(
        std::uint32_t object, IMoveSelfRotateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t owner,
        std::uint32_t duration_bits, const MoveTaskVector& target,
        const MoveTaskVector& speed, std::uint32_t direct,
        std::uint32_t distance_bits, std::uint32_t sound_name,
        std::uint32_t sound_time, std::uint32_t effect_name,
        std::uint32_t effect_time, std::uint32_t context,
        IMoveSelfRotateRuntime& runtime);
};

}  // namespace pal4
