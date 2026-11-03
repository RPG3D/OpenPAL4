#include "pal4/move_task.h"

#include "pal4/action_task_object.h"
#include "pal4/reference_text_assignment.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

void ReleaseTextRecord(const std::uint32_t record,
                       IMoveTaskRuntime& runtime) {
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

float ReadFloat(const std::uint32_t address,
                IMoveTaskRuntime& runtime) {
    return std::bit_cast<float>(runtime.ReadWord(address));
}

}  // namespace

std::uint32_t MoveTask::Construct(
    const std::uint32_t object, IMoveTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    const auto transient = runtime.TransientInitialByte();
    runtime.WriteByte(object + 0x60u, transient);
    for (const auto offset : {0x64u, 0x68u, 0x6cu, 0x78u,
                              0x7cu, 0x80u, 0x88u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 0x74u, transient);
    try {
        runtime.ConstructNameNode(object + 0x8cu);
    } catch (...) {
        runtime.ReleaseText(object + 0x74u);
        runtime.ReleaseText(object + 0x60u);
        runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0x94u, 0x98u})
        runtime.WriteWord(object + offset, 0u);
    return object;
}

void MoveTask::Destruct(const std::uint32_t object,
                        IMoveTaskRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    for (const auto offset : {0x40u, 0x44u, 0x94u, 0x98u})
        runtime.WriteWord(object + offset, 0u);
    if (runtime.AudioIsActive(object + 0x88u))
        runtime.AudioPlaySound(object + 0x88u);
    if (runtime.EffectNeedsUpdate(object + 0x8cu))
        runtime.EffectUpdate(object + 0x8cu);
    runtime.EffectReset(object + 0x8cu);
    ReleaseTextRecord(object + 0x74u, runtime);
    ReleaseTextRecord(object + 0x60u, runtime);
    runtime.WriteWord(object, 0x8486fcu);
    runtime.BaseDestruct(object);
}

std::uint8_t MoveTask::Enter(
    const std::uint32_t object, IMoveTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x94u);
    if (owner == 0u) return 0u;
    runtime.WriteWord(object + 0x40u, 0u);
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(object + 0x48u + offset,
                          runtime.ReadWord(owner + 0xacu + offset));

    if (runtime.ReadWord(object + 0x68u) != 0u) {
        const auto path = runtime.BuildSoundPath(object + 0x60u, ".wav");
        try {
            if (runtime.AudioIsActive(object + 0x88u))
                runtime.AudioPlaySound(object + 0x88u);
            auto parameter = runtime.ReadWord(object + 0x70u);
            if (static_cast<std::int32_t>(parameter) < 0) {
                parameter = 0u;
                runtime.WriteWord(object + 0x70u, 0u);
            }
            const auto filename = path != 0u ? path :
                runtime.EmptyStringAddress();
            runtime.WriteWord(object + 0x88u,
                              runtime.PlayMusic(filename, parameter));
        } catch (...) {
            runtime.ReleaseSoundPath(path);
            throw;
        }
        runtime.ReleaseSoundPath(path);
    }

    if (runtime.ReadWord(object + 0x7cu) != 0u) {
        if (runtime.EffectNeedsUpdate(object + 0x8cu))
            runtime.EffectUpdate(object + 0x8cu);
        auto name = runtime.ReadWord(object + 0x78u);
        if (name == 0u) name = runtime.EmptyStringAddress();
        const auto created = runtime.CreateEffect(
            name, owner + 0xacu, runtime.ReadWord(object + 0x84u),
            runtime.ReadWord(owner + 0x1a8u) != 0u);
        runtime.WriteWord(object + 0x8cu, created.first);
        runtime.WriteWord(object + 0x90u, created.second);
        runtime.ReleaseTemporaryEffect(created);
        const auto view = runtime.ReadWord(owner + 0x14u);
        runtime.SetEffectVisibility(object + 0x8cu,
                                    runtime.ReadWord(view + 4u));
    }
    return 1u;
}

std::uint8_t MoveTask::Update(
    const std::uint32_t object, const float delta_seconds,
    IMoveTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x94u);
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
        if (runtime.AudioIsActive(object + 0x88u))
            runtime.AudioPlaySound(object + 0x88u);
    }
    const auto fraction = elapsed / duration;
    MoveTaskVector position{};
    position.x = runtime.InterpolateFloat(
        ReadFloat(object + 0x48u, runtime),
        ReadFloat(object + 0x54u, runtime), fraction, 0u);
    position.y = runtime.InterpolateFloat(
        ReadFloat(object + 0x4cu, runtime),
        ReadFloat(object + 0x58u, runtime), fraction, 0u);
    position.z = runtime.InterpolateFloat(
        ReadFloat(object + 0x50u, runtime),
        ReadFloat(object + 0x5cu, runtime), fraction, 0u);
    runtime.SetOwnerPosition(owner, position, 0u);
    return 1u;
}

std::uint8_t MoveTask::SpawnReverseAndFollowup(
    const std::uint32_t object, IMoveTaskSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x98u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x94u);
    if (owner == 0u) return 0u;

    const auto reverse = MoveTaskFactory::Create(runtime);
    if (reverse != 0u)
        runtime.AddReverseTask(context, reverse, object + 0x0cu);
    auto effect_name = runtime.ReadWord(object + 0x78u);
    if (effect_name == 0u) effect_name = runtime.EmptyStringAddress();
    auto sound_name = runtime.ReadWord(object + 0x64u);
    if (sound_name == 0u) sound_name = runtime.EmptyStringAddress();
    (void)Initialize(reverse, owner, runtime.ReadWord(object + 0x44u),
                     object + 0x48u, sound_name,
                     runtime.ReadWord(object + 0x70u), effect_name,
                     runtime.ReadWord(object + 0x84u), 0u, runtime);
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

std::uint32_t MoveTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IMoveTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint8_t MoveTask::StopSoundIfPlaying(
    const std::uint32_t object, IMoveTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x88u);
    return playing != 0u ? runtime.AudioStopSound(object + 0x88u) : playing;
}

std::uint8_t MoveTask::PauseSoundIfNotPlaying(
    const std::uint32_t object, IMoveTaskRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x88u);
    return playing == 0u ? runtime.AudioPauseSound(object + 0x88u) : playing;
}

std::uint32_t MoveTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t duration_bits,
    const std::uint32_t target_vector,
    const std::uint32_t sound_name,
    const std::uint32_t sound_time,
    const std::uint32_t effect_name,
    const std::uint32_t effect_time,
    const std::uint32_t context, IMoveTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x94u, owner);
    runtime.WriteWord(object + 0x44u, duration_bits);
    for (const auto offset : {0u, 4u, 8u})
        runtime.WriteWord(object + 0x54u + offset,
                          runtime.ReadWord(target_vector + offset));
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x40u, 0u);
    (void)detail::AssignText(object + 0x60u, sound_name, runtime);
    runtime.WriteWord(object + 0x70u, sound_time);
    const auto effect = detail::AssignText(
        object + 0x74u, effect_name, runtime);
    runtime.WriteWord(object + 0x84u, effect_time);
    runtime.WriteWord(object + 0x98u, context);
    return effect.branch == detail::TextAssignmentResult::shared_empty ?
        effect_time : context;
}

std::uint32_t MoveTaskFactory::Create(
    IMoveTaskSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x9cu);
    if (memory == 0u) return 0u;
    try {
        return MoveTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

}  // namespace pal4
