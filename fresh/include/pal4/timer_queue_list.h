#pragma once

#include <cstdint>

namespace pal4 {

class ITimerQueueListRuntime {
public:
    virtual ~ITimerQueueListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyEntry(std::uint32_t entry) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class TimerQueueList final {
public:
    // IDA 0x4c0320.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t list, ITimerQueueListRuntime& runtime);
};

}  // namespace pal4
