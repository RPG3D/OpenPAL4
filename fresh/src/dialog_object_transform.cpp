#include "pal4/dialog_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {
int SetVector(DialogObjectState& state, std::size_t first_word,
              const std::array<float, 3u>& value, bool additive,
              IDialogObjectTransformRuntime& runtime) {
    for (std::size_t i = 0u; i < value.size(); ++i) {
        const auto current = std::bit_cast<float>(state.words[first_word + i]);
        state.words[first_word + i] = std::bit_cast<std::uint32_t>(
            additive ? current + value[i] : value[i]);
    }
    return runtime.RefreshTransform(state);
}
}  // namespace

int DialogObject::SetPosition(DialogObjectState& state,
                              const std::array<float, 3u>& value,
                              bool additive, IDialogObjectTransformRuntime& runtime) {
    return SetVector(state, 44u, value, additive, runtime);
}

int DialogObject::SetRotation(DialogObjectState& state,
                              const std::array<float, 3u>& value,
                              bool additive, IDialogObjectTransformRuntime& runtime) {
    return SetVector(state, 50u, value, additive, runtime);
}

int DialogObject::SetScale(DialogObjectState& state,
                           const std::array<float, 3u>& value,
                           bool additive, IDialogObjectTransformRuntime& runtime) {
    return SetVector(state, 47u, value, additive, runtime);
}

int DialogObject::SetParentFrame(DialogObjectState& state,
                                  std::uint32_t parent,
                                  IDialogObjectTransformRuntime& runtime) {
    const auto frame = state.words[43u];
    if (parent != 0u) {
        if (runtime.FrameParent(frame) != 0u)
            static_cast<void>(runtime.RemoveFrameChild(frame));
        return runtime.AddFrameChild(parent, frame);
    }
    const auto old_parent = runtime.FrameParent(frame);
    return old_parent != 0u ? runtime.RemoveFrameChild(frame)
                            : static_cast<int>(old_parent);
}

int DialogObject::SetVisibility(DialogObjectState& state,
                                 std::uint32_t value) {
    auto& byte_word = state.words[53u];
    byte_word = (byte_word & ~0xffu) | (value != 0u ? 1u : 0u);
    state.words[54u] = value;
    return static_cast<int>(value);
}

int DialogObject::RemoveFromParentFrame(DialogObjectState& state,
                                         IDialogObjectTransformRuntime& runtime) {
    const auto frame = state.words[43u];
    if (frame != 0u && runtime.FrameParent(frame) != 0u)
        return runtime.RemoveFrameChild(frame);
    return static_cast<int>(frame);
}

int DialogObject::ResetVisibility(DialogObjectState& state) {
    state.words[53u] &= ~0xffu;
    state.words[54u] = 0u;
    return 0;
}

int DialogObject::RefreshTransform(
    const DialogObjectState& state, IDialogObjectFrameRuntime& runtime) {
    const auto frame = state.words[43u];
    if (frame == 0u)
        return 0;
    const auto vector_at = [&state](std::size_t first) {
        return std::array<float, 3u>{
            std::bit_cast<float>(state.words[first]),
            std::bit_cast<float>(state.words[first + 1u]),
            std::bit_cast<float>(state.words[first + 2u])};
    };
    runtime.SetIdentity(frame);
    runtime.Scale(frame, vector_at(47u), 0u);
    const auto angles = vector_at(50u);
    runtime.Rotate(frame, {1.0f, 0.0f, 0.0f}, angles[0u], 2u);
    runtime.Rotate(frame, {0.0f, 1.0f, 0.0f}, angles[1u], 2u);
    runtime.Rotate(frame, {0.0f, 0.0f, 1.0f}, angles[2u], 2u);
    runtime.Translate(frame, vector_at(44u), 2u);
    return runtime.UpdateObjects(frame);
}

}  // namespace pal4
