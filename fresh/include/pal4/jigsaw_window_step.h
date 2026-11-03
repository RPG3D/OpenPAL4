#pragma once

#include "pal4/timer_queue_entry.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IJigsawWindowStepRuntime {
public:
    virtual ~IJigsawWindowStepRuntime() = default;
    virtual void ProcessPuzzleEvent(std::uint32_t puzzle,
                                    std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual StringBufferState BuildStepName(
        std::uint8_t text_flag, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void CopyFloorAttribute(TimerQueueEntryState& entry,
                                    const TimerQueueEntryState& source) = 0;
    virtual void InsertTimer(std::uint32_t queue,
                             std::uint32_t step,
                             std::uint32_t cursor,
                             const TimerQueueEntryState& entry) = 0;
    [[nodiscard]] virtual ITimerQueueEntryRuntime& EntryRuntime() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateCounter(
        std::uint32_t frame) = 0;
};

class JigsawWindowStep final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843e74u;
    static constexpr std::uint32_t kDecrementFunction = 0x4a0490u;

    // IDA 0x49ff40, whose body ends at 0x4a0093.
    [[nodiscard]] static std::uint32_t SetWithTimer(
        std::uint32_t frame, std::uint32_t step,
        IJigsawWindowStepRuntime& runtime);
};

}  // namespace pal4
