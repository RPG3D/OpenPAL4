#include "pal4/even_rotate_task.h"

#include "pal4/action_task_object.h"
#include "pal4/reference_text_assignment.h"
#include "pal4/task_pointer_collection.h"

#include <bit>
#include <initializer_list>

namespace pal4 {
namespace {

void ReleaseTextRecord(const std::uint32_t object,
                       const std::uint32_t data_offset,
                       IEvenRotateTaskRuntime& runtime) {
    const auto data = runtime.ReadWord(object + data_offset);
    if (data != 0u) {
        const auto count = runtime.ReadByte(data - 1u);
        if (count == 0u || count == 0xffu)
            runtime.FreeMemory(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    for (auto offset = data_offset; offset <= data_offset + 8u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
}

}  // namespace

std::uint32_t EvenRotateTask::Construct(
    const std::uint32_t object, IEvenRotateTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, 0x8486fcu);
    const auto transient = runtime.TransientInitialByte();
    runtime.WriteByte(object + 0x40u, transient);
    for (const auto offset : {0x44u, 0x48u, 0x4cu, 0x58u, 0x5cu,
                              0x60u, 0x68u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteByte(object + 0x54u, transient);
    try {
        runtime.ConstructNameNode(object + 0x6cu);
    } catch (...) {
        runtime.ReleaseText(object + 0x54u);
        runtime.ReleaseText(object + 0x40u);
        (void)runtime.BaseDestruct(object);
        throw;
    }
    runtime.WriteWord(object + 0x74u, 0u);
    runtime.WriteWord(object + 0x78u, 0u);
    runtime.WriteWord(object, 0x8487e8u);
    runtime.WriteWord(object + 0x3cu, 0x41a00000u);
    return object;
}

std::uint32_t EvenRotateTask::Destruct(
    const std::uint32_t object, IEvenRotateTaskRuntime& runtime) {
    runtime.WriteWord(object, 0x8487e8u);
    runtime.WriteWord(object + 0x3cu, 0x41a00000u);
    runtime.WriteWord(object + 0x74u, 0u);
    runtime.WriteWord(object + 0x78u, 0u);
    if (runtime.AudioIsActive(object + 0x68u))
        runtime.AudioPlaySound(object + 0x68u);
    if (runtime.EffectNeedsUpdate(object + 0x6cu))
        runtime.EffectUpdate(object + 0x6cu);
    runtime.EffectReset(object + 0x6cu);
    ReleaseTextRecord(object, 0x58u, runtime);
    ReleaseTextRecord(object, 0x44u, runtime);
    runtime.WriteWord(object, 0x8486fcu);
    return runtime.BaseDestruct(object);
}

std::uint8_t EvenRotateTask::Enter(
    const std::uint32_t object, IEvenRotatePlaybackRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x74u);
    if (owner == 0u) return 0u;

    if (runtime.ReadWord(object + 0x48u) != 0u) {
        const auto path = runtime.BuildSoundPath(object + 0x40u, ".wav");
        try {
            if (runtime.AudioIsActive(object + 0x68u))
                runtime.AudioPlaySound(object + 0x68u);
            auto parameter = runtime.ReadWord(object + 0x50u);
            if (static_cast<std::int32_t>(parameter) < 0) {
                parameter = 0u;
                runtime.WriteWord(object + 0x50u, 0u);
            }
            const auto filename = path != 0u ? path :
                runtime.EmptyStringAddress();
            const auto manager = runtime.AudioManager();
            const auto sound = runtime.PlayMusic(manager, filename, parameter);
            runtime.WriteWord(object + 0x68u, sound);
        } catch (...) {
            runtime.ReleaseSoundPath(path);
            throw;
        }
        runtime.ReleaseSoundPath(path);
    }

    if (runtime.ReadWord(object + 0x5cu) != 0u) {
        if (runtime.EffectNeedsUpdate(object + 0x6cu))
            runtime.EffectUpdate(object + 0x6cu);
        auto name = runtime.ReadWord(object + 0x58u);
        const auto anchored = runtime.ReadWord(owner + 0x1a8u) != 0u;
        if (name == 0u) name = runtime.EmptyStringAddress();
        const auto manager = runtime.EffectManager();
        const auto effect = runtime.CreateEffect(
            manager, name, owner + 0xacu,
            runtime.ReadWord(object + 0x64u), anchored);
        runtime.WriteWord(object + 0x6cu, effect.first);
        runtime.WriteWord(object + 0x70u, effect.second);
        runtime.ReleaseTemporaryEffect(effect);
        const auto view = runtime.ReadWord(owner + 0x14u);
        runtime.SetEffectVisibility(object + 0x6cu,
                                    runtime.ReadWord(view + 4u));
    }
    return 1u;
}

std::uint8_t EvenRotateTask::Update(
    const std::uint32_t object, const float delta_seconds,
    IEvenRotatePlaybackRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x74u);
    if (owner == 0u) return 0u;
    const auto speed = std::bit_cast<float>(runtime.ReadWord(object + 0x3cu));
    const EvenRotateVector delta{0.0f, delta_seconds * speed, 0.0f};
    runtime.RotateOwner(owner, delta, 1u);
    return 1u;
}

std::uint8_t EvenRotateTask::SpawnReverseAndFollowup(
    const std::uint32_t object, IEvenRotateSpawnRuntime& runtime) {
    const auto context = runtime.ReadWord(object + 0x78u);
    if (context == 0u) return 0u;
    const auto owner = runtime.ReadWord(object + 0x74u);
    if (owner == 0u) return 0u;

    const auto reverse = EvenRotateTaskFactory::Create(runtime);
    if (reverse != 0u)
        runtime.AddReverseTask(context, reverse, object + 0x30u);
    auto effect_name = runtime.ReadWord(object + 0x58u);
    if (effect_name == 0u) effect_name = runtime.EmptyStringAddress();
    auto sound_name = runtime.ReadWord(object + 0x44u);
    if (sound_name == 0u) sound_name = runtime.EmptyStringAddress();
    const auto speed = std::bit_cast<float>(runtime.ReadWord(object + 0x3cu));
    runtime.InitializeReverseTask(
        reverse, owner, std::bit_cast<std::uint32_t>(-speed), sound_name,
        runtime.ReadWord(object + 0x50u), effect_name,
        runtime.ReadWord(object + 0x64u), 0u);
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

std::uint32_t EvenRotateTaskFactory::ConstructDescriptor(
    const std::uint32_t descriptor, IEvenRotateTaskRuntime& runtime) {
    runtime.WriteWord(descriptor, kDescriptorVtable);
    return descriptor;
}

std::uint32_t EvenRotateTaskFactory::Create(
    IEvenRotateSpawnRuntime& runtime) {
    const auto memory = runtime.AllocateMemory(0x7cu);
    if (memory == 0u) return 0u;
    try {
        return EvenRotateTask::Construct(memory, runtime);
    } catch (...) {
        runtime.FreeMemory(memory);
        throw;
    }
}

std::uint8_t EvenRotateTask::StopSoundIfPlaying(
    const std::uint32_t object, IEvenRotatePlaybackRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x68u);
    return playing != 0u ? runtime.AudioStopSound(object + 0x68u) : playing;
}

std::uint8_t EvenRotateTask::PauseSoundIfNotPlaying(
    const std::uint32_t object, IEvenRotatePlaybackRuntime& runtime) {
    const auto playing = runtime.AudioIsPlaying(object + 0x68u);
    return playing == 0u ? runtime.AudioPauseSound(object + 0x68u) : playing;
}

std::uint32_t EvenRotateTask::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t speed_bits, const std::uint32_t sound_name,
    const std::uint32_t sound_time, const std::uint32_t effect_name,
    const std::uint32_t effect_time, const std::uint32_t context,
    IEvenRotateTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x74u, owner);
    runtime.WriteWord(object + 0x3cu, speed_bits);
    (void)detail::AssignText(object + 0x40u, sound_name, runtime);
    runtime.WriteWord(object + 0x50u, sound_time);
    const auto effect = detail::AssignText(object + 0x54u, effect_name, runtime);
    runtime.WriteWord(object + 0x64u, effect_time);
    runtime.WriteWord(object + 0x78u, context);
    if (effect.branch == detail::TextAssignmentResult::shared_empty)
        return context;
    if (effect.branch == detail::TextAssignmentResult::empty)
        return effect.native_result;
    return object;
}

}  // namespace pal4
