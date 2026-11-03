#include "pal4/rotate_task.h"

#include "pal4/action_task_object.h"
#include "pal4/reference_text_assignment.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

float ReadFloat(const std::uint32_t address, IRotateTaskRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

MoveTaskVector ReadVector(const std::uint32_t address,
                          IRotateTaskRuntime& runtime) {
    return {ReadFloat(address, runtime), ReadFloat(address + 4u, runtime),
            ReadFloat(address + 8u, runtime)};
}

void WriteVector(const std::uint32_t address,
                 const MoveTaskVector& value, IRotateTaskRuntime& runtime) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value.x));
    runtime.WriteWord(address + 4u, std::bit_cast<std::uint32_t>(value.y));
    runtime.WriteWord(address + 8u, std::bit_cast<std::uint32_t>(value.z));
}

void ReleaseText(const std::uint32_t record, IRotateTaskRuntime& runtime) {
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
    auto delta = target - start;
    if (start < 180.0f && target > 180.0f && delta > 180.0f)
        delta -= 360.0f;
    else if (start > 180.0f && target < 180.0f &&
             start - target > 180.0f)
        delta += 360.0f;
    return delta;
}

}  // namespace

std::uint32_t RotateTask::Construct(
    const std::uint32_t object, IRotateTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    const auto transient = runtime.TransientInitialByte();
    runtime.WriteByte(object + 0x64u, transient);
    for (const auto offset : {0x68u, 0x6cu, 0x70u, 0x7cu,
                              0x80u, 0x84u, 0x8cu})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 0x78u, transient);
    try {
        runtime.ConstructNameNode(object + 0x90u);
    } catch (...) {
        runtime.ReleaseText(object + 0x78u);
        runtime.ReleaseText(object + 0x64u);
        runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x98u, 0u);
    runtime.WriteWord(object + 0x9cu, 0u);
    return object;
}

void RotateTask::Destruct(
    const std::uint32_t object, IRotateTaskRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    runtime.WriteWord(object + 0x44u, 0u);
    runtime.WriteWord(object + 0x98u, 0u);
    runtime.WriteWord(object + 0x9cu, 0u);
    if (runtime.AudioIsActive(object + 0x8cu))
        runtime.AudioPlaySound(object + 0x8cu);
    if (runtime.EffectNeedsUpdate(object + 0x90u))
        runtime.EffectUpdate(object + 0x90u);
    runtime.EffectReset(object + 0x90u);
    ReleaseText(object + 0x78u, runtime);
    ReleaseText(object + 0x64u, runtime);
    runtime.WriteWord(object, 0x8486fcu);
    runtime.BaseDestruct(object);
}

std::uint8_t RotateTask::Enter(
    const std::uint32_t object, IRotateTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x98u);
    if (owner == 0u) return 0u;
    runtime.WriteWord(object + 0x40u, 0u);
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(object + 0x4cu + offset,
                          runtime.ReadWord(owner + 0xa0u + offset));
    runtime.WriteWord(object + 0x48u, std::bit_cast<std::uint32_t>(
        ShortestYawDelta(ReadFloat(object + 0x50u, runtime),
                         ReadFloat(object + 0x5cu, runtime))));

    if (runtime.ReadWord(object + 0x6cu) != 0u) {
        const auto path = runtime.BuildSoundPath(object + 0x64u, ".wav");
        try {
            if (runtime.AudioIsActive(object + 0x8cu))
                runtime.AudioPlaySound(object + 0x8cu);
            auto parameter = runtime.ReadWord(object + 0x74u);
            if (static_cast<std::int32_t>(parameter) < 0) {
                parameter = 0u;
                runtime.WriteWord(object + 0x74u, 0u);
            }
            const auto filename = path != 0u ? path :
                runtime.EmptyStringAddress();
            runtime.WriteWord(object + 0x8cu,
                              runtime.PlayMusic(filename, parameter));
        } catch (...) {
            runtime.ReleaseSoundPath(path);
            throw;
        }
        runtime.ReleaseSoundPath(path);
    }
    if (runtime.ReadWord(object + 0x80u) != 0u) {
        if (runtime.EffectNeedsUpdate(object + 0x90u))
            runtime.EffectUpdate(object + 0x90u);
        auto name = runtime.ReadWord(object + 0x7cu);
        if (name == 0u) name = runtime.EmptyStringAddress();
        const auto created = runtime.CreateEffect(
            name, owner + 0xacu, runtime.ReadWord(object + 0x88u),
            runtime.ReadWord(owner + 0x1a8u) != 0u);
        runtime.WriteWord(object + 0x90u, created.first);
        runtime.WriteWord(object + 0x94u, created.second);
        runtime.ReleaseTemporaryEffect(created);
        const auto view = runtime.ReadWord(owner + 0x14u);
        runtime.SetEffectVisibility(object + 0x90u,
                                    runtime.ReadWord(view + 4u));
    }
    return 1u;
}

std::uint8_t RotateTask::Update(
    const std::uint32_t object, const float delta_seconds,
    IRotateTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x98u);
    if (owner == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;
    auto elapsed = ReadFloat(object + 0x40u, runtime) + delta_seconds;
    const auto duration = ReadFloat(object + 0x44u, runtime);
    runtime.WriteWord(object + 0x40u,
                      std::bit_cast<std::uint32_t>(elapsed));
    if (elapsed >= duration) {
        elapsed = duration;
        runtime.WriteWord(object + 0x40u,
                          std::bit_cast<std::uint32_t>(duration));
        runtime.WriteByte(object + 0x3cu, 1u);
        if (runtime.AudioIsActive(object + 0x8cu))
            runtime.AudioPlaySound(object + 0x8cu);
    }
    const MoveTaskVector rotation{
        ReadFloat(object + 0x4cu, runtime),
        ReadFloat(object + 0x50u, runtime) + runtime.InterpolateFloat(
            0.0f, ReadFloat(object + 0x48u, runtime),
            elapsed / duration, 0u),
        ReadFloat(object + 0x54u, runtime)};
    runtime.SetOwnerRotation(owner, rotation, 0u);
    return 1u;
}

std::uint8_t RotateTask::SpawnReverseAndFollowup(
    const std::uint32_t object, IRotateTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x9cu);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x98u);
    if (owner == 0u) return 0u;
    const auto child = RotateTaskFactory::Create(runtime);
    if (child != 0u)
        runtime.AddReverseTask(context, child, object + 0x0cu);
    auto sound_name = runtime.ReadWord(object + 0x68u);
    if (sound_name == 0u) sound_name = runtime.EmptyStringAddress();
    auto effect_name = runtime.ReadWord(object + 0x7cu);
    if (effect_name == 0u) effect_name = runtime.EmptyStringAddress();
    (void)Initialize(child, owner, runtime.ReadWord(object + 0x44u),
                     ReadVector(object + 0x4cu, runtime),
                     sound_name, runtime.ReadWord(object + 0x74u),
                     effect_name, runtime.ReadWord(object + 0x88u),
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

std::uint32_t RotateTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IRotateTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint8_t RotateTask::StopSoundIfPlaying(
    const std::uint32_t object, IRotateTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x8cu);
    return playing != 0u ? runtime.AudioStopSound(object + 0x8cu) : playing;
}

std::uint8_t RotateTask::PauseSoundIfNotPlaying(
    const std::uint32_t object, IRotateTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x8cu);
    return playing == 0u ? runtime.AudioPauseSound(object + 0x8cu) : playing;
}

std::uint32_t RotateTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t duration_bits,
    const MoveTaskVector& target_rotation,
    const std::uint32_t sound_name, const std::uint32_t sound_time,
    const std::uint32_t effect_name, const std::uint32_t effect_time,
    const std::uint32_t context, IRotateTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x98u, owner);
    runtime.WriteWord(object + 0x44u, duration_bits);
    WriteVector(object + 0x58u, target_rotation, runtime);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    (void)detail::AssignText(object + 0x64u, sound_name, runtime);
    runtime.WriteWord(object + 0x74u, sound_time);
    const auto effect = detail::AssignText(
        object + 0x78u, effect_name, runtime);
    runtime.WriteWord(object + 0x88u, effect_time);
    runtime.WriteWord(object + 0x9cu, context);
    return effect.branch == detail::TextAssignmentResult::shared_empty ?
        effect_time : context;
}

std::uint32_t RotateTaskFactory::Create(
    IRotateTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0xa0u);
    if (memory == 0u) return 0u;
    try {
        return RotateTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
