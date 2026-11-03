#include "pal4/move_self_rotate_task.h"

#include "pal4/action_task_object.h"
#include "pal4/reference_text_assignment.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <cmath>
#include <initializer_list>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address,
                IMoveSelfRotateRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

MoveTaskVector ReadVector(const std::uint32_t address,
                          IMoveSelfRotateRuntime& runtime) {
    return {ReadFloat(address, runtime),
            ReadFloat(address + 4u, runtime),
            ReadFloat(address + 8u, runtime)};
}

void WriteVector(const std::uint32_t address,
                 const MoveTaskVector& value,
                 IMoveSelfRotateRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value.x));
    runtime.WriteWord(address + 4u,
                      std::bit_cast<std::uint32_t>(value.y));
    runtime.WriteWord(address + 8u,
                      std::bit_cast<std::uint32_t>(value.z));
}

void ReleaseTextRecord(const std::uint32_t record,
                       IMoveSelfRotateRuntime& runtime) {
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

bool Rotates(const MoveTaskVector& speed) {
    return (speed.x != 0.0f && !std::isnan(speed.x)) ||
           (speed.y != 0.0f && !std::isnan(speed.y)) ||
           (speed.z != 0.0f && !std::isnan(speed.z));
}

}  // namespace

std::uint32_t MoveSelfRotateTask::Construct(
    const std::uint32_t object, IMoveSelfRotateRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    const auto transient = runtime.TransientInitialByte();
    runtime.WriteByte(object + 0x80u, transient);
    for (const auto offset : {0x84u, 0x88u, 0x8cu, 0x98u,
                              0x9cu, 0xa0u, 0xa8u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 0x94u, transient);
    try {
        runtime.ConstructNameNode(object + 0xacu);
    } catch (...) {
        runtime.ReleaseText(object + 0x94u);
        runtime.ReleaseText(object + 0x80u);
        runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0xb4u, 0xb8u})
        runtime.WriteWord(object + offset, 0u);
    return object;
}

void MoveSelfRotateTask::Destruct(
    const std::uint32_t object, IMoveSelfRotateRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0xb4u, 0xb8u})
        runtime.WriteWord(object + offset, 0u);
    if (runtime.AudioIsActive(object + 0xa8u))
        runtime.AudioPlaySound(object + 0xa8u);
    if (runtime.EffectNeedsUpdate(object + 0xacu))
        runtime.EffectUpdate(object + 0xacu);
    runtime.EffectReset(object + 0xacu);
    ReleaseTextRecord(object + 0x94u, runtime);
    ReleaseTextRecord(object + 0x80u, runtime);
    runtime.WriteWord(object, 0x8486fcu);
    runtime.BaseDestruct(object);
}

std::uint8_t MoveSelfRotateTask::Enter(
    const std::uint32_t object, IMoveSelfRotateRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0xb4u);
    if (owner == 0u) return 0u;
    runtime.WriteWord(object + 0x40u, 0u);
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(object + 0x48u + offset,
                          runtime.ReadWord(owner + 0xacu + offset));
    if (runtime.ReadWord(object + 0x6cu) != 0u)
        WriteVector(object + 0x74u,
                    runtime.FrameForwardVector(owner), runtime);

    if (runtime.ReadWord(object + 0x88u) != 0u) {
        const auto path = runtime.BuildSoundPath(object + 0x80u, ".wav");
        try {
            if (runtime.AudioIsActive(object + 0xa8u))
                runtime.AudioPlaySound(object + 0xa8u);
            auto parameter = runtime.ReadWord(object + 0x90u);
            if (static_cast<std::int32_t>(parameter) < 0) {
                parameter = 0u;
                runtime.WriteWord(object + 0x90u, 0u);
            }
            const auto filename = path != 0u ? path :
                runtime.EmptyStringAddress();
            runtime.WriteWord(object + 0xa8u,
                              runtime.PlayMusic(filename, parameter));
        } catch (...) {
            runtime.ReleaseSoundPath(path);
            throw;
        }
        runtime.ReleaseSoundPath(path);
    }

    if (runtime.ReadWord(object + 0x9cu) != 0u) {
        if (runtime.EffectNeedsUpdate(object + 0xacu))
            runtime.EffectUpdate(object + 0xacu);
        auto name = runtime.ReadWord(object + 0x98u);
        if (name == 0u) name = runtime.EmptyStringAddress();
        const auto created = runtime.CreateEffect(
            name, kEffectPosition, runtime.ReadWord(object + 0xa4u),
            runtime.ReadWord(owner + 0x1a8u) != 0u);
        runtime.WriteWord(object + 0xacu, created.first);
        runtime.WriteWord(object + 0xb0u, created.second);
        runtime.ReleaseTemporaryEffect(created);
        const auto view = runtime.ReadWord(owner + 0x14u);
        runtime.SetEffectProperties(object + 0xacu,
                                    runtime.ReadWord(view + 4u));
    }
    return 1u;
}

std::uint8_t MoveSelfRotateTask::Update(
    const std::uint32_t object, const float delta_seconds,
    IMoveSelfRotateRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0xb4u);
    if (owner == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    auto elapsed = ReadFloat(object + 0x40u, runtime) + delta_seconds;
    const auto duration = ReadFloat(object + 0x44u, runtime);
    runtime.WriteWord(object + 0x40u, std::bit_cast<std::uint32_t>(elapsed));
    if (elapsed >= duration) {
        elapsed = duration;
        runtime.WriteWord(object + 0x40u,
                          std::bit_cast<std::uint32_t>(duration));
        runtime.WriteByte(object + 0x3cu, 1u);
        if (runtime.AudioIsActive(object + 0xa8u))
            runtime.AudioPlaySound(object + 0xa8u);
        if (runtime.EffectNeedsUpdate(object + 0xacu))
            runtime.EffectUpdate(object + 0xacu);
    }
    const auto fraction = elapsed / duration;
    const auto start = ReadVector(object + 0x48u, runtime);
    MoveTaskVector position{};
    if (runtime.ReadWord(object + 0x6cu) != 0u) {
        const auto distance = runtime.InterpolateFloat(
            0.0f, ReadFloat(object + 0x70u, runtime), fraction, 0u);
        const auto forward = ReadVector(object + 0x74u, runtime);
        position = {start.x + forward.x * distance,
                    start.y + forward.y * distance,
                    start.z + forward.z * distance};
    } else {
        const auto target = ReadVector(object + 0x54u, runtime);
        position = {
            runtime.InterpolateFloat(start.x, target.x, fraction, 0u),
            runtime.InterpolateFloat(start.y, target.y, fraction, 0u),
            runtime.InterpolateFloat(start.z, target.z, fraction, 0u)};
    }
    runtime.SetOwnerPosition(owner, position, 0u);
    const auto speed = ReadVector(object + 0x60u, runtime);
    if (Rotates(speed)) {
        const MoveTaskVector delta{speed.x * delta_seconds,
                                   speed.y * delta_seconds,
                                   speed.z * delta_seconds};
        const auto rotation = runtime.UpdateFrameRotation(owner, delta);
        runtime.RotateOwner(owner, rotation, 0u);
    }
    return 1u;
}

std::uint8_t MoveSelfRotateTask::SpawnReverseAndFollowup(
    const std::uint32_t object, IMoveSelfRotateSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0xb8u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0xb4u);
    if (owner == 0u) return 0u;
    const auto speed = ReadVector(object + 0x60u, runtime);
    const MoveTaskVector reverse_speed{-speed.x, -speed.y, -speed.z};
    const auto reverse = MoveSelfRotateTaskFactory::Create(runtime);
    if (reverse != 0u)
        runtime.AddReverseTask(context, reverse, object + 0x0cu);
    auto effect_name = runtime.ReadWord(object + 0x98u);
    if (effect_name == 0u) effect_name = runtime.EmptyStringAddress();
    auto sound_name = runtime.ReadWord(object + 0x84u);
    if (sound_name == 0u) sound_name = runtime.EmptyStringAddress();
    (void)Initialize(reverse, owner, runtime.ReadWord(object + 0x44u),
                     ReadVector(object + 0x48u, runtime), reverse_speed,
                     runtime.ReadWord(object + 0x6cu),
                     runtime.ReadWord(object + 0x70u), sound_name,
                     runtime.ReadWord(object + 0x90u), effect_name,
                     runtime.ReadWord(object + 0xa4u), 0u, runtime);
    runtime.InsertOwnerTask(owner + 0x2a0u, reverse);
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

std::uint32_t MoveSelfRotateTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IMoveSelfRotateRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint8_t MoveSelfRotateTask::StopSoundIfPlaying(
    const std::uint32_t object, IMoveSelfRotateRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0xa8u);
    return playing != 0u ? runtime.AudioStopSound(object + 0xa8u) : playing;
}

std::uint8_t MoveSelfRotateTask::PauseSoundIfNotPlaying(
    const std::uint32_t object, IMoveSelfRotateRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0xa8u);
    return playing == 0u ? runtime.AudioPauseSound(object + 0xa8u) : playing;
}

std::uint32_t MoveSelfRotateTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t duration_bits, const MoveTaskVector& target,
    const MoveTaskVector& speed, const std::uint32_t direct,
    const std::uint32_t distance_bits,
    const std::uint32_t sound_name,
    const std::uint32_t sound_time,
    const std::uint32_t effect_name,
    const std::uint32_t effect_time,
    const std::uint32_t context, IMoveSelfRotateRuntime& runtime) {
    runtime.WriteWord(object + 0xb4u, owner);
    runtime.WriteWord(object + 0x44u, duration_bits);
    WriteVector(object + 0x54u, target, runtime);
    WriteVector(object + 0x60u, speed, runtime);
    runtime.WriteWord(object + 0x6cu, direct);
    runtime.WriteWord(object + 0x70u, distance_bits);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    (void)detail::AssignText(object + 0x80u, sound_name, runtime);
    runtime.WriteWord(object + 0x90u, sound_time);
    const auto effect = detail::AssignText(
        object + 0x94u, effect_name, runtime);
    runtime.WriteWord(object + 0xa4u, effect_time);
    runtime.WriteWord(object + 0xb8u, context);
    return effect.branch == detail::TextAssignmentResult::shared_empty ?
        effect_time : context;
}

std::uint32_t MoveSelfRotateTaskFactory::Create(
    IMoveSelfRotateSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0xbcu);
    if (memory == 0u) return 0u;
    try {
        return MoveSelfRotateTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
