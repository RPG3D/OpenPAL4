#include "pal4/action_task_object.h"

#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {
namespace {

constexpr std::uint32_t base_vtable = 0x8486fcu;
constexpr std::uint32_t action_vtable = 0x8486d8u;
constexpr std::uint32_t animation_offset = 0x1dcu;

bool NameDiffers(const std::uint32_t action_record,
                 const std::uint32_t animation,
                 IActionTaskRuntime& runtime) {
    const auto name = runtime.CurrentAnimationName(animation);
    return runtime.CompareCaseInsensitive(action_record, name) != 0;
}

}  // namespace

std::uint32_t ActionTaskObject::Construct(
    const std::uint32_t object, IActionTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, base_vtable);
    try {
        runtime.InitializeEmptyString(object + 0x40u);
    } catch (...) {
        static_cast<void>(DestructBase(object, runtime));
        throw;
    }
    runtime.WriteWord(object, action_vtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.WriteWord(object + 0x4cu, 1u);
    runtime.WriteWord(object + 0x50u, 0u);
    return object;
}

std::uint32_t ActionTaskObject::DestructBase(
    const std::uint32_t object, IActionTaskRuntime& runtime) {
    runtime.WriteWord(object, base_vtable);
    return runtime.BaseDestruct(object);
}

std::uint32_t ActionTaskObject::Destruct(
    const std::uint32_t object, IActionTaskRuntime& runtime) {
    runtime.WriteWord(object, action_vtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    try {
        runtime.CleanupString(object + 0x40u);
    } catch (...) {
        static_cast<void>(DestructBase(object, runtime));
        throw;
    }
    return DestructBase(object, runtime);
}

std::uint8_t ActionTaskObject::Enter(
    const std::uint32_t object, IActionTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x54u);
    if (owner == 0u) return 0u;

    const auto animation = owner + animation_offset;
    const auto action_record = object + 0x40u;
    if (NameDiffers(action_record, animation, runtime))
        runtime.SetAnimation(animation, runtime.ReadWord(action_record));
    static_cast<void>(runtime.ClearAnimationFlag(animation, 0u));
    runtime.SetAnimationFlag(animation, 0u);

    if (runtime.ReadWord(owner + 0x1b0u) == 4u) {
        auto path = runtime.ReadStringRecord(0x94d560u);
        path += runtime.ReadStringRecord(0x94d524u);
        static_cast<void>(runtime.AppendSoundSuffix(path, "WC003.wav"));
        runtime.PlayMusic(path, 1u);
    }
    return 1u;
}

std::uint8_t ActionTaskObject::Update(
    const std::uint32_t object, const std::int32_t,
    IActionTaskRuntime& runtime) {
    const auto owner = runtime.ReadWord(object + 0x54u);
    if (owner == 0u) {
        runtime.WriteByte(object + 0x3cu, 0u);
        return 0u;
    }
    if (runtime.ReadByte(object + 0x3cu) != 0u) return 0u;

    const auto animation = owner + animation_offset;
    const auto action_record = object + 0x40u;
    if (NameDiffers(action_record, animation, runtime)) {
        runtime.WriteByte(object + 0x3cu, 1u);
        return 0u;
    }
    if (runtime.AnimationFlag(animation)) {
        const auto remaining = runtime.ReadWord(object + 0x50u) - 1u;
        runtime.WriteWord(object + 0x50u, remaining);
        if (std::bit_cast<std::int32_t>(remaining) > 0) {
            runtime.SetAnimation(animation, runtime.ReadWord(action_record));
        } else {
            runtime.WriteByte(object + 0x3cu, 1u);
            runtime.SetAnimationTime(animation,
                                     runtime.AnimationLength(animation));
            static_cast<void>(runtime.ClearAnimationFlag(animation, 1u));
        }
    }
    return 1u;
}

std::uint32_t ActionTaskObject::Initialize(
    const std::uint32_t object, const std::uint32_t owner,
    const std::uint32_t repeats, const std::uint32_t name,
    const std::uint32_t context, IActionTaskRuntime& runtime) {
    runtime.WriteWord(object + 0x54u, owner);
    runtime.WriteByte(object + 0x3cu, 0u);
    runtime.InitializeStringFromCString(object + 0x40u, name);
    runtime.WriteWord(object + 0x58u, context);
    runtime.WriteWord(object + 0x4cu, repeats);
    runtime.WriteWord(object + 0x50u, repeats);

    const auto animation = owner + animation_offset;
    if (NameDiffers(object + 0x40u, animation, runtime))
        runtime.SetAnimation(animation, runtime.ReadWord(object + 0x40u));
    else
        runtime.SetAnimationTime(animation, 0.0f);
    return runtime.ClearAnimationFlag(animation, 1u);
}

}  // namespace pal4
