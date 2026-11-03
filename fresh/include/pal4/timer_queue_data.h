#pragma once

#include "pal4/timer_queue_entry.h"

#include <cstdint>

namespace pal4 {

class ITimerQueueDataRuntime : public ITimerQueueEntryRuntime {
public:
    virtual void AssignName(TimerQueueEntryState& destination,
                            const TimerQueueEntryState& source,
                            std::uint32_t offset,
                            std::uint32_t count) = 0;
};

class TimerQueueData final {
public:
    static void Copy(TimerQueueEntryState* destination,
                     TimerQueueEntryState& source,
                     ITimerQueueDataRuntime& runtime);

    [[nodiscard]] static TimerQueueEntryState& Assign(
        TimerQueueEntryState& destination, TimerQueueEntryState& source,
        std::uint32_t empty_string, ITimerQueueDataRuntime& runtime);
};

}  // namespace pal4
