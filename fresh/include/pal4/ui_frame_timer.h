#pragma once

#include "pal4/timed_event_queue.h"

#include <cstdint>

namespace pal4 {

class UiFrameTimer final {
public:
    static constexpr std::uint32_t kFirstCallbackVtable = 0x8454f0u;
    static constexpr std::uint32_t kSecondCallbackVtable = 0x8454f8u;

    // IDA UiFrameTimer_Add at 0x54ad20.
    static void Add(std::uint32_t queue, std::uint32_t name,
                    std::uint32_t delay, std::uint32_t owner,
                    std::uint32_t function, ITimedEventQueueRuntime& runtime);
    // IDA UiFrameTimer_AddSecond at 0x54ae60.
    static void AddSecond(std::uint32_t queue, std::uint32_t name,
                          std::uint32_t delay, std::uint32_t owner,
                          std::uint32_t function,
                          ITimedEventQueueRuntime& runtime);
};

class IUiTimerCallbackRuntime {
public:
    virtual ~IUiTimerCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class UiTimerCallback final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x843b40u;

    // IDA UiTimerCallback_Delete at 0x54afa0.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t callback, std::uint8_t flags,
        IUiTimerCallbackRuntime& runtime);
    // IDA UiTimerCallback_Destruct at 0x54afc0.
    static void Destruct(std::uint32_t callback,
                         IUiTimerCallbackRuntime& runtime);
};

}  // namespace pal4
