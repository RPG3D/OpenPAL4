#include "pal4/movement_task.h"

#include "pal4/action_task_object.h"
#include "pal4/reference_text_assignment.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address,
                IMovementTaskRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

MoveTaskVector ReadVector(const std::uint32_t address,
                          IMovementTaskRuntime& runtime) {
    return {ReadFloat(address, runtime),
            ReadFloat(address + 4u, runtime),
            ReadFloat(address + 8u, runtime)};
}

void WriteVector(const std::uint32_t address,
                 const MoveTaskVector& value,
                 IMovementTaskRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value.x));
    runtime.WriteWord(address + 4u,
                      std::bit_cast<std::uint32_t>(value.y));
    runtime.WriteWord(address + 8u,
                      std::bit_cast<std::uint32_t>(value.z));
}

void ReleaseTextRecord(const std::uint32_t record,
                       IMovementTaskRuntime& runtime) {
    const auto data = runtime.ReadWord(record + 4u);
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    for (const auto offset : {4u, 8u, 12u})
        runtime.WriteWord(record + offset, 0u);
}

float ShortestYawDelta(const float start, const float target) {
    // IDA5978BA..597913: subtract from360 before adding the other angle.
    // Reassociating to (target-start)+/-360 loses one ULP for M14 trans01.
    if (start < 180.0f && target > 180.0f && target - start > 180.0f) {
        const float complement = 360.0f - target;
        return -(complement + start);
    }
    if (start > 180.0f && target < 180.0f && start - target > 180.0f) {
        const float complement = 360.0f - start;
        return complement + target;
    }
    return target - start;
}

}  // namespace

std::uint32_t MovementTask::Construct(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    const auto transient = runtime.TransientInitialByte();
    runtime.WriteByte(object + 0x84u, transient);
    for (const auto offset : {0x88u, 0x8cu, 0x90u, 0x9cu,
                              0xa0u, 0xa4u, 0xacu})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 0x98u, transient);
    try {
        runtime.ConstructNameNode(object + 0xb0u);
    } catch (...) {
        runtime.ReleaseText(object + 0x98u);
        runtime.ReleaseText(object + 0x84u);
        runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x40u, 0u);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0xb8u, 0u);
    return object;
}

void MovementTask::Destruct(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x40u, 0u);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x48u, 0u);
    runtime.WriteWord(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0xb8u, 0u);
    if (runtime.AudioIsActive(object + 0xacu))
        runtime.AudioPlaySound(object + 0xacu);
    if (runtime.EffectNeedsUpdate(object + 0xb0u))
        runtime.EffectUpdate(object + 0xb0u);
    runtime.EffectReset(object + 0xb0u);
    ReleaseTextRecord(object + 0x98u, runtime);
    ReleaseTextRecord(object + 0x84u, runtime);
    runtime.WriteWord(object, 0x8486fcu);
    runtime.BaseDestruct(object);
}

std::uint8_t MovementTask::Enter(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x3cu);
    if (owner == 0u) return 0u;
    runtime.WriteWord(object + 0x44u, 0u);
    for (const auto offset : {0u, 4u, 8u}) {
        runtime.WriteWord(object + 0x54u + offset,
                          runtime.ReadWord(owner + 0xacu + offset));
        runtime.WriteWord(object + 0x6cu + offset,
                          runtime.ReadWord(owner + 0xa0u + offset));
    }
    const auto yaw_delta = ShortestYawDelta(
        ReadFloat(object + 0x70u, runtime),
        ReadFloat(object + 0x7cu, runtime));
    runtime.WriteWord(object + 0x4cu,
                      std::bit_cast<std::uint32_t>(yaw_delta));
    runtime.WriteWord(object + 0x50u,
                      std::bit_cast<std::uint32_t>(
                          yaw_delta / ReadFloat(object + 0x48u, runtime)));

    if (runtime.ReadWord(object + 0x8cu) != 0u) {
        const auto path = runtime.BuildSoundPath(object + 0x84u, ".wav");
        try {
            if (runtime.AudioIsActive(object + 0xacu))
                runtime.AudioPlaySound(object + 0xacu);
            auto parameter = runtime.ReadWord(object + 0x94u);
            if (static_cast<std::int32_t>(parameter) < 0) {
                parameter = 0u;
                runtime.WriteWord(object + 0x94u, 0u);
            }
            const auto filename = path != 0u ? path :
                runtime.EmptyStringAddress();
            runtime.WriteWord(object + 0xacu,
                              runtime.PlayMusic(filename, parameter));
        } catch (...) {
            runtime.ReleaseSoundPath(path);
            throw;
        }
        runtime.ReleaseSoundPath(path);
    }
    if (runtime.ReadWord(object + 0xa0u) != 0u) {
        if (runtime.EffectNeedsUpdate(object + 0xb0u))
            runtime.EffectUpdate(object + 0xb0u);
        auto name = runtime.ReadWord(object + 0x9cu);
        if (name == 0u) name = runtime.EmptyStringAddress();
        const auto created = runtime.CreateEffect(
            name, owner + 0xacu, runtime.ReadWord(object + 0xa8u),
            runtime.ReadWord(owner + 0x1a8u) != 0u);
        runtime.WriteWord(object + 0xb0u, created.first);
        runtime.WriteWord(object + 0xb4u, created.second);
        runtime.ReleaseTemporaryEffect(created);
        const auto view = runtime.ReadWord(owner + 0x14u);
        runtime.SetEffectVisibility(object + 0xb0u,
                                    runtime.ReadWord(view + 4u));
    }
    return 1u;
}

std::uint8_t MovementTask::Update(
    const std::uint32_t object, const float delta_seconds,
    IMovementTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x3cu);
    if (owner == 0u) {
        runtime.WriteByte(object + 0x40u, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x40u) != 0u) return 0u;
    auto elapsed = ReadFloat(object + 0x44u, runtime) + delta_seconds;
    const auto duration = ReadFloat(object + 0x48u, runtime);
    runtime.WriteWord(object + 0x44u,
                      std::bit_cast<std::uint32_t>(elapsed));
    if (elapsed >= duration) {
        elapsed = duration;
        runtime.WriteWord(object + 0x44u,
                          std::bit_cast<std::uint32_t>(duration));
        runtime.WriteByte(object + 0x40u, 1u);
        if (runtime.AudioIsActive(object + 0xacu))
            runtime.AudioPlaySound(object + 0xacu);
    }
    const auto fraction = elapsed / duration;
    const MoveTaskVector position{
        runtime.InterpolateFloat(
            ReadFloat(object + 0x54u, runtime),
            ReadFloat(object + 0x60u, runtime), fraction, 0u),
        runtime.InterpolateFloat(
            ReadFloat(object + 0x58u, runtime),
            ReadFloat(object + 0x64u, runtime), fraction, 0u),
        runtime.InterpolateFloat(
            ReadFloat(object + 0x5cu, runtime),
            ReadFloat(object + 0x68u, runtime), fraction, 0u)};
    const MoveTaskVector rotation{
        ReadFloat(object + 0x6cu, runtime),
        ReadFloat(object + 0x70u, runtime) +
            ReadFloat(object + 0x50u, runtime) * elapsed,
        ReadFloat(object + 0x74u, runtime)};
    runtime.SetOwnerPosition(owner, position, 0u);
    runtime.SetOwnerRotation(owner, rotation, 0u);
    return 1u;
}

std::uint8_t MovementTask::SpawnFollowup(
    const std::uint32_t object, IMovementTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0xb8u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x3cu);
    if (owner == 0u) return 0u;
    const auto child = MovementTaskFactory::Create(runtime);
    if (child != 0u)
        runtime.AddReverseTask(context, child, object + 0x0cu);
    auto effect_name = runtime.ReadWord(object + 0x9cu);
    if (effect_name == 0u) effect_name = runtime.EmptyStringAddress();
    auto sound_name = runtime.ReadWord(object + 0x88u);
    if (sound_name == 0u) sound_name = runtime.EmptyStringAddress();
    (void)Initialize(child, owner, runtime.ReadWord(object + 0x48u),
                     ReadVector(object + 0x54u, runtime),
                     ReadVector(object + 0x6cu, runtime),
                     sound_name, runtime.ReadWord(object + 0x94u),
                     effect_name, runtime.ReadWord(object + 0xa8u),
                     0u, runtime);
    runtime.InsertOwnerTask(owner + 0x2a0u, child);
    const auto previous = runtime.ReadWord(object + 0x30u);
    if (previous != 0u) {
        runtime.NotifyPreviousTask(previous);
        return 1u;
    }
    const auto helper = ActionTaskFactory::CreateHelper(runtime);
    if (helper != 0u)
        runtime.AddHelperTask(context, helper, 1u, 0u, 0u);
    runtime.SetHelperOwner(helper, owner);
    TaskPointerCollection::Append(owner + 0x2a0u, helper, runtime);
    runtime.MarkProcessed(context);
    return 1u;
}

std::uint32_t MovementTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IMovementTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint8_t MovementTask::IsComplete(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    return runtime.ReadByte(object + 0x40u);
}

std::uint8_t MovementTask::StopSoundIfPlaying(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0xacu);
    return playing != 0u ? runtime.AudioStopSound(object + 0xacu) : playing;
}

std::uint8_t MovementTask::PauseSoundIfNotPlaying(
    const std::uint32_t object, IMovementTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0xacu);
    return playing == 0u ? runtime.AudioPauseSound(object + 0xacu) : playing;
}

std::uint32_t MovementTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t duration_bits,
    const MoveTaskVector& target_position,
    const MoveTaskVector& target_rotation,
    const std::uint32_t sound_name,
    const std::uint32_t sound_time,
    const std::uint32_t effect_name,
    const std::uint32_t effect_time,
    const std::uint32_t context, IMovementTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x3cu, owner);
    runtime.WriteByte(object + 0x40u, 0u);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x48u, duration_bits);
    WriteVector(object + 0x60u, target_position, runtime);
    WriteVector(object + 0x78u, target_rotation, runtime);
    (void)detail::AssignText(object + 0x84u, sound_name, runtime);
    runtime.WriteWord(object + 0x94u, sound_time);
    const auto effect = detail::AssignText(
        object + 0x98u, effect_name, runtime);
    runtime.WriteWord(object + 0xa8u, effect_time);
    runtime.WriteWord(object + 0xb8u, context);
    return effect.branch == detail::TextAssignmentResult::shared_empty ?
        effect_time : context;
}

std::uint32_t MovementTaskFactory::Create(
    IMovementTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0xbcu);
    if (memory == 0u) return 0u;
    try {
        return MovementTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
