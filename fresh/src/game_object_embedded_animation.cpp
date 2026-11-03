#include "pal4/game_object_embedded_animation.h"

#include <bit>
#include <limits>

namespace pal4 {

namespace {

float ReadFloat(const GameObjectEmbeddedAnimationState& state,
                const std::size_t index) {
    return std::bit_cast<float>(state.words[index]);
}

void WriteFloat(GameObjectEmbeddedAnimationState& state,
                const std::size_t index, const float value) {
    state.words[index] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

GameObjectEmbeddedAnimationState&
GameObjectEmbeddedAnimation::InitializeState(
    GameObjectEmbeddedAnimationState& state,
    const std::uint8_t stack_byte,
    IGameObjectEmbeddedAnimationRuntime& runtime) {
    auto& words = state.words;
    // The source reads this byte from an uninitialized stack slot.
    words[6] = (words[6] & 0xffffff00u) | stack_byte;
    words[7] = words[8] = words[9] = 0;
    runtime.InitializeAnimationData(state);
    for (const auto index : {1, 2, 3, 4, 5, 10, 11, 13, 14, 15, 16, 17,
                             18, 19, 24, 25, 26, 27, 28, 29, 30})
        words[index] = 0;
    words[0] = kVtableAddress;
    words[12] = 0x3f800000u;
    return state;
}

bool GameObjectEmbeddedAnimation::CleanupData(
    GameObjectEmbeddedAnimationState& state,
    IGameObjectEmbeddedAnimationRuntime& runtime) {
    auto& words = state.words;
    words[1] = 0;
    if (words[2] != 0) {
        const auto array = words[2];
        runtime.DestructAnimationArray(
            array, 0x44u, runtime.AnimationArrayCount(array - 4u));
        (void)runtime.FreeMemory(array - 4u);
        words[2] = 0;
    }
    words[3] = 0;
    if (words[4] != 0) {
        runtime.FreeWrappedMemory(words[4]);
        words[4] = 0;
    }
    if (words[5] != 0) {
        runtime.FreeWrappedMemory(words[5]);
        words[5] = 0;
    }
    words[8] = words[7];
    words[10] = words[11] = 0;
    runtime.CleanupAnimationDataRange(state);
    for (const auto index : {28, 29, 30}) {
        if (words[index] != 0) {
            (void)runtime.FreeMemory(words[index]);
            words[index] = 0;
        }
    }
    return true;
}

std::uint32_t GameObjectEmbeddedAnimation::CleanupState(
    GameObjectEmbeddedAnimationState& state,
    IGameObjectEmbeddedAnimationRuntime& runtime) {
    state.words[0] = kVtableAddress;
    (void)CleanupData(state, runtime);
    runtime.CleanupAnimationData(state);
    const auto result = runtime.FreeMemory(state.words[7]);
    state.words[7] = state.words[8] = state.words[9] = 0;
    return result;
}

bool GameObjectEmbeddedAnimation::ProcessAnimation(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t /*argument*/,
    IGameObjectEmbeddedAnimationRuntime& runtime) {
    if (state.words[1] == 0) return false;
    const auto hierarchy = runtime.FindHierarchy(state.words[1]);
    if (hierarchy == 0) return false;
    if (state.words[24] != 0) {
        const auto count = static_cast<std::int32_t>(state.words[25]);
        for (std::int32_t index = 0; index < count; ++index)
            runtime.ProcessAnimationData(state, hierarchy, index);
    }
    return true;
}

bool GameObjectEmbeddedAnimation::UpdateInterpolator(
    GameObjectEmbeddedAnimationState& state, const float delta_time,
    IGameObjectInterpolatorRuntime& runtime) {
    auto& words = state.words;
    if (words[1] == 0 || words[3] == 0) return false;
    if (words[19] != 0) return true;

    const float scaled_delta = delta_time * ReadFloat(state, 12);
    const auto hierarchy = runtime.FindHierarchy(words[1]);
    if (hierarchy == 0) return false;
    const auto interpolator = runtime.HierarchyInterpolator(hierarchy);
    const auto animation = runtime.CurrentAnimation(interpolator);
    if (animation == 0) return false;
    const float duration = runtime.AnimationDuration(animation);

    if (words[15] == 0) {
        if (scaled_delta + runtime.InterpolatorTime(interpolator) >= duration) {
            runtime.SetInterpolatorTime(interpolator, duration);
            runtime.UpdateAnimationTime(
                state, runtime.InterpolatorTime(interpolator));
            if (words[17] != 0) {
                runtime.CleanupCurrentRecord(words[3]);
                if (words[16] != 0) {
                    auto name = runtime.NextActionName(words[3]);
                    if (name == 0) name = runtime.EmptyActionName();
                    runtime.DispatchNextAction(state, name, 0.2f);
                } else {
                    runtime.SetInterpolatorTime(interpolator, 0.0f);
                }
            }
            words[18] = 1;
        } else {
            runtime.AddInterpolatorTime(interpolator, scaled_delta);
            runtime.UpdateAnimationTime(
                state, runtime.InterpolatorTime(interpolator));
            words[18] = 0;
        }
    }

    if (words[15] != 0 && ReadFloat(state, 13) > 0.0f &&
        words[4] != 0 && words[5] != 0) {
        const float blend_duration = ReadFloat(state, 13);
        const float remaining = ReadFloat(state, 14);
        const float weight = 1.0f - remaining / blend_duration;
        runtime.BlendAnimations(interpolator, words[4], words[5], weight);
        const float next_remaining = remaining - scaled_delta;
        WriteFloat(state, 14, next_remaining);
        if (next_remaining <= 0.0f) {
            runtime.SetInterpolatorAnimation(
                interpolator, runtime.AnimationFromInterpolator(words[5]));
            runtime.SetInterpolatorTime(interpolator, 0.0f);
            words[13] = words[14] = words[15] = 0;
        }
    }
    runtime.UpdateHierarchyMatrices(hierarchy);
    return true;
}

std::uint32_t GameObjectEmbeddedAnimation::SetRestartFlag(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t value) noexcept {
    return state.words[17] = value;
}

std::uint32_t GameObjectEmbeddedAnimation::SetTimeScaleBits(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t value) noexcept {
    return state.words[12] = value;
}

std::uint32_t GameObjectEmbeddedAnimation::SetPauseFlag(
    GameObjectEmbeddedAnimationState& state,
    const std::uint32_t value) noexcept {
    return state.words[19] = value;
}

bool GameObjectEmbeddedAnimation::SetTime(
    GameObjectEmbeddedAnimationState& state, const float time,
    IGameObjectInterpolatorRuntime& runtime) {
    if (state.words[1] == 0) return false;
    const auto hierarchy = runtime.FindHierarchy(state.words[1]);
    if (hierarchy == 0) return false;
    const auto interpolator = runtime.HierarchyInterpolator(hierarchy);
    const auto animation = runtime.CurrentAnimation(interpolator);
    if (animation == 0 || time < 0.0f ||
        time > runtime.AnimationDuration(animation))
        return false;
    runtime.SetInterpolatorTime(interpolator, time);
    runtime.UpdateHierarchyMatrices(hierarchy);
    return true;
}

std::uint32_t GameObjectEmbeddedAnimation::CompletedFlag(
    const GameObjectEmbeddedAnimationState& state) noexcept {
    return state.words[18];
}

double GameObjectEmbeddedAnimation::AnimationLength(
    const GameObjectEmbeddedAnimationState& state,
    IGameObjectInterpolatorRuntime& runtime) {
    if (state.words[3] == 0) return 0.0;
    const auto animation = runtime.RecordAnimationPointer(state.words[3]);
    return animation == 0
               ? 0.0
               : static_cast<double>(runtime.AnimationDuration(animation));
}

double GameObjectEmbeddedAnimation::AnimationTime(
    const GameObjectEmbeddedAnimationState& state,
    IGameObjectInterpolatorRuntime& runtime) {
    if (state.words[1] == 0) return 0.0;
    const auto hierarchy = runtime.FindHierarchy(state.words[1]);
    if (hierarchy == 0) return 0.0;
    const auto interpolator = runtime.HierarchyInterpolator(hierarchy);
    if (interpolator == 0) return 0.0;
    return static_cast<double>(runtime.InterpolatorTime(interpolator));
}

bool GameObjectEmbeddedAnimation::InitializeRenderBuffers(
    GameObjectEmbeddedAnimationState& state, const std::uint32_t source,
    IGameObjectAnimationRenderBufferRuntime& runtime) {
    if (source == 0) return false;
    auto& words = state.words;
    for (const auto index : {28, 29, 30}) {
        if (words[index] != 0) {
            runtime.Free(words[index]);
            words[index] = 0;
        }
    }

    const auto count = runtime.FrameCount(source);
    words[25] = static_cast<std::uint32_t>(count);
    if (count <= 0 ||
        static_cast<std::uint32_t>(count) >
            std::numeric_limits<std::uint32_t>::max() / 36u)
        return false;

    const auto positive_count = static_cast<std::uint32_t>(count);
    words[29] = runtime.Allocate(36u * positive_count);
    words[30] = runtime.Allocate(4u * positive_count);
    words[28] = runtime.Allocate(4u * (positive_count >> 1));
    if (words[29] == 0 || words[30] == 0 ||
        (positive_count > 1 && words[28] == 0)) {
        for (const auto index : {28, 29, 30}) {
            if (words[index] != 0) runtime.Free(words[index]);
            words[index] = 0;
        }
        return false;
    }

    for (std::uint32_t index = 0; index < positive_count; ++index) {
        runtime.WriteHalf(words[30] + 4u * index, 0);
        runtime.WriteHalf(words[30] + 4u * index + 2u,
                          static_cast<std::uint16_t>(index + 1u));
        runtime.WriteWord(words[29] + 36u * index + 24u, 0xffffffffu);
    }
    return true;
}

}  // namespace pal4
