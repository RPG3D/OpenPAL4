#include "pal4/jigsaw_window_result.h"
#include "pal4/preview_text_codec.h"

#include <new>
#include <bit>

namespace pal4 {
namespace {

constexpr JigsawResultOverlaySpec kOverlayBase{
    0x43c80000u, 0x43960000u, "zi0", {}, 0x41a00000u,
    0x42700000u, "", "system", 0x3f800000u,
    0x40000000u, 0x40400000u};

std::uint32_t ShowResultCore(
    const std::uint32_t frame, const std::uint8_t success,
    const std::uint32_t trigger_id, const std::uint32_t finished_offset,
    const std::uint32_t callback_vtable,
    IJigsawWindowResultRuntime& runtime) {
    std::string formatted;
    try {
        const auto count = runtime.ReadCounter(frame);
        const auto format = runtime.TriggerFormat(trigger_id);
        formatted = runtime.FormatText(format.value_or(""), count);
        runtime.SetText(runtime.ReadWord(frame + 0x188u),
                        PreviewTextCodec::DecodeUtf8(
                            runtime.ConvertToUtf8(formatted)));

        runtime.WriteByte(frame + finished_offset, 1u);
        const auto manager = runtime.FrameManager();
        runtime.WriteByte(manager + 0x222u, success);
        runtime.PlaySound(success != 0u ? "WA23.wav" : "WA24.wav", 0u);

        const auto overlay = runtime.CreateOverlay(
            0x48u, JigsawWindowResult::kOverlayVtable);
        if (overlay == 0u) throw std::bad_alloc();
        try {
            auto spec = kOverlayBase;
            spec.image = success != 0u ? "gamewin" : "gamelose";
            runtime.ConfigureOverlay(overlay, spec);
            runtime.AddOverlay(manager + 0x1dcu, 1u, overlay);
        } catch (...) {
            runtime.ReleaseOverlay(overlay);
            throw;
        }

        const auto queue = runtime.ReadWord(manager + 0x1a0u);
        const auto callback = runtime.AllocateCallback(0x18u);
        try {
            if (callback != 0u) {
                runtime.WriteWord(callback, callback_vtable);
                runtime.WriteWord(callback + 4u, frame);
                runtime.WriteWord(callback + 8u,
                                  JigsawWindowResult::kCloseFunction);
                runtime.WriteWord(callback + 0xcu, 0u);
                runtime.WriteByte(callback + 0x10u, 0u);
            }
            runtime.QueueCloseEvent(queue + 0xcu, "closeGame",
                                    0x40800000u, callback,
                                    callback != 0u);
        } catch (...) {
            if (callback != 0u) runtime.ReleaseCallback(callback);
            throw;
        }
    } catch (...) {
        (void)runtime.ReleaseFormatted(formatted);
        throw;
    }
    return runtime.ReleaseFormatted(formatted);
}

}  // namespace

std::uint32_t JigsawWindowResult::Show(
    const std::uint32_t frame, const std::uint8_t success,
    IJigsawWindowResultRuntime& runtime) {
    return ShowResultCore(frame, success, 0xf5u, 0x18cu,
                          kCallbackVtable, runtime);
}

std::uint32_t PuzzleWindowResult::Show(
    const std::uint32_t frame, const std::uint8_t success,
    IPuzzleWindowResultRuntime& runtime) {
    return ShowResultCore(frame, success, 0xe9u, 0x190u,
                          kCallbackVtable, runtime);
}

std::uint32_t JigsawResultOverlay::UpdateAnimation(
    const std::uint32_t overlay, const float delta,
    IJigsawResultOverlayRuntime& runtime) {
    const auto progress = runtime.UpdateAnimationProgress(delta);
    const auto remaining = static_cast<double>(runtime.ReadFloat(overlay + 0x28u)) -
        static_cast<double>(delta) * runtime.ReadFloat(overlay + 0x44u);
    runtime.WriteFloat(overlay + 0x28u, static_cast<float>(remaining));
    const auto status = runtime.CompareStatusWord(remaining);
    if ((std::popcount(static_cast<unsigned>(status & 0x4100u)) & 1) != 0)
        runtime.WriteFloat(overlay + 0x28u, 1.0f);
    runtime.WriteFloat(overlay + 0x24u,
                       runtime.ReadFloat(overlay + 0x28u));
    return (progress & 0xffff0000u) | status;
}

}  // namespace pal4
