#include "pal4/poetry_resume_dialog.h"
#include "pal4/timed_event_queue.h"

#include <string>

namespace pal4 {

std::uint32_t PoetryResumeDialog::Initialize(
    const std::uint32_t dialog, const std::uint32_t layout,
    IPoetryResumeDialogRuntime& runtime) {
    runtime.LoadLayout(dialog, layout, 0u, 0u);
    runtime.WriteByte(dialog + 156u, static_cast<std::uint8_t>(layout));
    runtime.WriteWord(dialog + 160u, 0u);
    runtime.WriteWord(dialog + 164u, 0u);
    runtime.WriteWord(dialog + 168u, 0u);
    constexpr std::string_view name = "PoetryResume";
    if (runtime.AllocateText(dialog + 156u,
                             static_cast<std::uint32_t>(name.size()), 1u)) {
        runtime.CopyText(dialog + 156u, name);
        runtime.SetTextSize(dialog + 156u,
                            static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(dialog, kVtable);
    runtime.InitializeTemporaryString();
    runtime.WriteWord(dialog + 152u, kWindowCount);
    for (std::uint32_t index = 1u; index <= kWindowCount; ++index) {
        const auto prefix = runtime.ReadPrefix(dialog + 24u);
        std::string window_name(prefix);
        window_name += index < 10u ? "No0" : "No";
        window_name += std::to_string(index);
        runtime.WriteWord(dialog + 40u + (index - 1u) * 4u,
                          runtime.GetWindow(dialog, window_name));
    }
    runtime.SetEnabled(runtime.ReadWord(dialog + 36u), false);
    runtime.CleanupTemporaryString();
    return dialog;
}

std::uint32_t PoetryResumeDialog::Destruct(
    const std::uint32_t dialog, IPoetryResumeDialogRuntime& runtime) {
    runtime.WriteWord(dialog, kVtable);
    const auto text = runtime.ReadWord(dialog + 160u);
    if (text != 0u) {
        const auto reference = runtime.ReadByte(text - 1u);
        if (reference == 0u || reference == 0xffu) {
            runtime.Free(text - 1u);
        } else {
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(reference - 1u));
        }
    }
    runtime.WriteWord(dialog + 160u, 0u);
    runtime.WriteWord(dialog + 164u, 0u);
    runtime.WriteWord(dialog + 168u, 0u);
    return runtime.DestructBase(dialog);
}

std::uint32_t PoetryResumeDialog::Hide(
    const std::uint32_t dialog, IPoetryResumeDialogRuntime& runtime) {
    std::int32_t index = 0;
    while (index < static_cast<std::int32_t>(
                       runtime.ReadWord(dialog + 152u))) {
        const auto window = runtime.ReadWord(
            dialog + 40u + static_cast<std::uint32_t>(index) * 4u);
        runtime.SetAlpha(window, 0.0f);
        ++index;
    }
    runtime.WriteWord(dialog + 172u, 0u);
    runtime.WriteWord(dialog + 176u, 0u);
    runtime.WriteByte(dialog + 180u, 0u);
    return runtime.CallVirtualSlot4(dialog, 1u, 0u);
}

void PoetryResumeDialog::Update(
    const std::uint32_t dialog, const double elapsed,
    IPoetryResumeDialogRuntime& runtime) {
    if (runtime.ReadByte(dialog + 180u) != 0u) return;
    const auto current = static_cast<std::int32_t>(
        runtime.ReadWord(dialog + 172u));
    const auto count = static_cast<std::int32_t>(
        runtime.ReadWord(dialog + 152u));
    if (current < count) {
        UpdateAlpha(dialog, static_cast<float>(elapsed), runtime);
    } else {
        static_cast<void>(FadeOut(dialog, static_cast<float>(elapsed),
                                  runtime));
    }
}

void PoetryResumeDialog::ResetState(
    const std::uint32_t dialog, IPoetryResumeDialogRuntime& runtime) {
    runtime.WriteByte(dialog + 180u, 0u);
}

void PoetryResumeDialog::UpdateAlpha(
    const std::uint32_t dialog, const float elapsed,
    IPoetryResumeDialogRuntime& runtime) {
    float progress = runtime.ReadFloat(dialog + 176u) + elapsed;
    runtime.WriteFloat(dialog + 176u, progress);
    const auto current = static_cast<std::int32_t>(
        runtime.ReadWord(dialog + 172u));
    for (std::int32_t index = 0; index < current; ++index) {
        const auto window = runtime.ReadWord(
            dialog + 40u + static_cast<std::uint32_t>(index) * 4u);
        const auto alpha = runtime.GetAlpha(window) + elapsed;
        runtime.SetAlpha(window, alpha > 1.0f ? 1.0f : alpha);
    }

    const auto current_window = runtime.ReadWord(
        dialog + 40u + static_cast<std::uint32_t>(current) * 4u);
    if (progress < 0.61799997f) {
        runtime.SetAlpha(current_window, progress);
        return;
    }
    if (progress > 1.0f) {
        progress = 1.0f;
        runtime.WriteFloat(dialog + 176u, progress);
    }
    runtime.SetAlpha(current_window, progress);
    const auto next = current + 1;
    if (current >= static_cast<std::int32_t>(kWindowCount)) {
        if (progress >= 1.0f) {
            runtime.WriteFloat(dialog + 176u, 0.0f);
            runtime.WriteWord(dialog + 172u,
                              static_cast<std::uint32_t>(next));
            runtime.WriteByte(dialog + 180u, 1u);
            runtime.ScheduleReset(dialog, dialog + 156u, 2.0f);
        }
        return;
    }
    if (next % 7 == 0 && progress < 1.0f) return;
    runtime.WriteFloat(dialog + 176u, 0.0f);
    runtime.WriteWord(dialog + 172u, static_cast<std::uint32_t>(next));
    if (next % 7 == 0) {
        runtime.WriteByte(dialog + 180u, 1u);
        runtime.ScheduleReset(dialog, dialog + 156u, 1.5f);
    }
}

std::uint32_t PoetryResumeDialog::FadeOut(
    const std::uint32_t dialog, const float elapsed,
    IPoetryResumeDialogRuntime& runtime) {
    const float progress =
        runtime.ReadFloat(dialog + 176u) + elapsed * 0.30000001f;
    runtime.WriteFloat(dialog + 176u, progress);
    if (progress >= 1.0f) {
        return runtime.CallVirtualSlot4(dialog, 0u, 0u);
    }
    const auto count = static_cast<std::int32_t>(
        runtime.ReadWord(dialog + 152u));
    for (std::int32_t index = 0; index < count; ++index) {
        const auto window = runtime.ReadWord(
            dialog + 40u + static_cast<std::uint32_t>(index) * 4u);
        runtime.SetAlpha(window, 1.0f - progress);
    }
    return static_cast<std::uint32_t>(count);
}

void PoetryResumeDialog::RegisterCallback(
    const std::uint32_t queue, const std::uint32_t name,
    const std::uint32_t delay_bits,
    const std::uint32_t callback_owner,
    const std::uint32_t callback_function,
    ITimedEventQueueRuntime& runtime) {
    TimedEventQueue::AddWithCallbackVtable(
        queue, name, delay_bits, callback_owner, callback_function,
        0x844244u, runtime);
}

}  // namespace pal4
