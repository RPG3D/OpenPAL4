#include "pal4/game_world_fade_queue.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

std::uint32_t FadeColor(const float progress, const bool fade_in) {
    const auto weight = fade_in ? static_cast<double>(progress)
                                : 1.0 - static_cast<double>(progress);
    const auto scaled = weight * 255.0 + 0.5;
    if (!std::isfinite(scaled) || scaled < -0x1p63 ||
        scaled >= 0x1p63)
        return 0u;
    const auto channel = static_cast<std::uint8_t>(
        static_cast<std::int64_t>(std::trunc(scaled)));
    return static_cast<std::uint32_t>(channel) * 0x01010101u;
}

std::uint32_t EraseRecord(IGameWorldFadeRuntime& runtime,
                          const std::uint32_t world,
                          const std::uint32_t record,
                          const std::uint32_t end_offset) {
    const auto old_end = runtime.ReadWord(world + end_offset);
    for (auto source = record + GameWorldFadeQueue::kRecordBytes;
         source != old_end; source += GameWorldFadeQueue::kRecordBytes) {
        for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
            runtime.WriteWord(source - GameWorldFadeQueue::kRecordBytes +
                                  offset,
                              runtime.ReadWord(source + offset));
    }
    runtime.WriteWord(world + end_offset,
                      old_end - GameWorldFadeQueue::kRecordBytes);
    return old_end;
}

struct FadeStep {
    bool complete;
    float progress;
};

FadeStep AdvanceTime(IGameWorldFadeRuntime& runtime,
                     const std::uint32_t record, const float delta) {
    const auto duration = std::bit_cast<float>(runtime.ReadWord(record + 4u));
    const auto previous = std::bit_cast<float>(runtime.ReadWord(record + 8u));
    // Original 519E70/519FC0/51A0B0 run under PC24: compare the
    // rounded single-precision sum, including additions below half an ULP.
    const float elapsed = previous + delta;
    runtime.WriteWord(record + 8u, std::bit_cast<std::uint32_t>(elapsed));
    const bool complete = elapsed > duration;
    if (complete)
        runtime.WriteWord(record + 8u,
                          std::bit_cast<std::uint32_t>(duration));
    const auto stored = complete ? duration : elapsed;
    const auto progress = static_cast<float>(
        static_cast<double>(stored) / static_cast<double>(duration));
    return {complete, progress};
}

}  // namespace

std::uint32_t GameWorldFadeQueue::Advance(
    const std::uint32_t world, const float delta,
    IGameWorldFadeRuntime& runtime) {
    auto record = runtime.ReadWord(world + 620u);
    auto result = runtime.ReadWord(world + 624u);
    while (record != runtime.ReadWord(world + 624u)) {
        const auto object = runtime.ReadWord(record);
        if (runtime.ReadWord(object + 388u) == 0u) {
            result = EraseRecord(runtime, world, record, 624u);
            continue;
        }
        const auto step = AdvanceTime(runtime, record, delta);
        const auto rgba = FadeColor(step.progress, false);
        result = runtime.SetMaterialFlags(runtime.ReadWord(object + 20u), rgba);
        if (!step.complete) {
            record += kRecordBytes;
            continue;
        }

        const auto removed_object = runtime.ReadWord(record);
        static_cast<void>(runtime.SetMaterialFlags(
            runtime.ReadWord(removed_object + 20u), rgba));
        runtime.ScriptOnRemove(world, runtime.ReadWord(record),
                               GameWorldFadeRemoval::ScriptOnRemove3);
        result = EraseRecord(runtime, world, record, 624u);
    }
    return result;
}

std::uint32_t GameWorldFadeQueue::AdvanceIn(
    const std::uint32_t world, const float delta,
    IGameWorldFadeRuntime& runtime) {
    auto record = runtime.ReadWord(world + 636u);
    auto result = runtime.ReadWord(world + 640u);
    while (record != runtime.ReadWord(world + 640u)) {
        const auto step = AdvanceTime(runtime, record, delta);
        const auto object = runtime.ReadWord(record);
        result = runtime.SetMaterialFlags(runtime.ReadWord(object + 20u),
                                           FadeColor(step.progress, true));
        if (step.complete)
            result = EraseRecord(runtime, world, record, 640u);
        else
            record += kRecordBytes;
    }
    return result;
}

std::uint32_t GameWorldFadeQueue::AdvanceOutSecondary(
    const std::uint32_t world, const float delta,
    IGameWorldFadeRuntime& runtime) {
    auto record = runtime.ReadWord(world + 652u);
    auto result = runtime.ReadWord(world + 656u);
    while (record != runtime.ReadWord(world + 656u)) {
        const auto step = AdvanceTime(runtime, record, delta);
        const auto object = runtime.ReadWord(record);
        const auto rgba = FadeColor(step.progress, false);
        result = runtime.SetMaterialFlags(runtime.ReadWord(object + 20u), rgba);
        if (!step.complete) {
            record += kRecordBytes;
            continue;
        }

        const auto removed_object = runtime.ReadWord(record);
        static_cast<void>(runtime.SetMaterialFlags(
            runtime.ReadWord(removed_object + 20u), rgba));
        runtime.ScriptOnRemove(world, runtime.ReadWord(record),
                               GameWorldFadeRemoval::ScriptOnRemove5);
        result = EraseRecord(runtime, world, record, 656u);
    }
    return result;
}

}  // namespace pal4
