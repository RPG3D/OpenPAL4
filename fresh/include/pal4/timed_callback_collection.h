#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct TimedCallbackCollectionState {
    std::uint32_t unknown_0{};
    std::uint32_t timer_head{};   // +4
    std::uint32_t timer_count{};  // +8
    std::uint32_t unknown_c{};
    std::uint32_t event_head{};   // +10
    std::uint32_t event_count{};  // +14
};
static_assert(offsetof(TimedCallbackCollectionState, timer_head) == 4u);
static_assert(offsetof(TimedCallbackCollectionState, event_head) == 16u);
static_assert(offsetof(TimedCallbackCollectionState, event_count) == 20u);
static_assert(sizeof(TimedCallbackCollectionState) == 24u);

struct TimedCallbackName {
    std::uint32_t unknown_0{};
    std::uint32_t data{};    // +4
    std::uint32_t length{};  // +8
};
static_assert(sizeof(TimedCallbackName) == 12u);

class ITimedCallbackCollectionRuntime {
public:
    virtual ~ITimedCallbackCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void DeleteOwned(std::uint32_t address, std::uint32_t flags) = 0;
    virtual void InvokeEvent(std::uint32_t callback) = 0;
    virtual void InvokeTimer(std::uint32_t callback) = 0;
    virtual void CleanupEventData(std::uint32_t payload) = 0;
    virtual void CleanupTimerData(std::uint32_t payload) = 0;
    virtual void FreeNode(std::uint32_t address) = 0;
    [[nodiscard]] virtual int CompareBytes(std::uint32_t first,
                                           std::uint32_t second,
                                           std::uint32_t count) = 0;
};

class TimedCallbackCollection final {
public:
    explicit TimedCallbackCollection(ITimedCallbackCollectionRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ed0c0: event list first, then timer list.
    void ReleaseAll(TimedCallbackCollectionState& state);
    // 0x5ed100/0x5ed160: release slots at payload +0x14/+0x18.
    void ReleaseEvent(std::uint32_t payload);
    void ReleaseTimer(std::uint32_t payload);
    // 0x5ed1c0/0x5ed270: release every matching timer/event slot.
    [[nodiscard]] std::uint32_t ReleaseTimerByName(
        TimedCallbackCollectionState& state, const TimedCallbackName& name);
    [[nodiscard]] std::uint32_t ReleaseEventByName(
        TimedCallbackCollectionState& state, const TimedCallbackName& name);
    // 0x5ed320: update event and timer lists for one frame.
    void Update(TimedCallbackCollectionState& state, float delta);
    // 0x5ed450: run event-data cleanup, optionally freeing the payload.
    [[nodiscard]] std::uint32_t DestroyEventPayload(
        std::uint32_t payload, std::uint32_t flags);

private:
    void ReleaseSlot(std::uint32_t slot);
    [[nodiscard]] std::uint32_t ReleaseByName(
        std::uint32_t head, const TimedCallbackName& name,
        bool event_list);
    [[nodiscard]] float ReadFloat(std::uint32_t address);
    void WriteFloat(std::uint32_t address, float value);
    void Unlink(std::uint32_t node);
    ITimedCallbackCollectionRuntime& runtime_;
};

}  // namespace pal4

