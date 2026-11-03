#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/timed_callback_collection.h"
#include "pal4/timer_queue_entry.h"
#include "pal4/timer_queue_task.h"
#include "pal4/combat_event_lifecycle.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>

namespace pal4 {

// Owns the x86 repeating-timer list used by UIFrameManager +0x1a0.
class NativeGameTimerQueue final : private ITimedCallbackCollectionRuntime,
                                   private ITimerQueueEntryRuntime,
                                   private ITimerCallbackInvocationRuntime,private ICombatEventCallbackRuntime,private ICombatEventValueInvokeRuntime {
public:
    using Callback = std::function<void(std::uint32_t owner,
                                        std::uint32_t method)>;
    using ParameterCallback = std::function<void(std::uint32_t owner,
        std::uint32_t method,std::int32_t row,std::uint8_t flag)>;

    using ValueCallback=std::function<void(std::uint32_t owner,std::uint32_t method,std::uint32_t value)>;
    explicit NativeGameTimerQueue(Callback callback,ParameterCallback parameter_callback={},ValueCallback value_callback={});
    // Original24-byte8480EC member/value callback; next event frame owns execution.
    void AddCombatEndEvent(std::string_view name,float delay,std::uint32_t owner,std::uint32_t result);
    NativeGameTimerQueue(const NativeGameTimerQueue&) = delete;
    NativeGameTimerQueue& operator=(const NativeGameTimerQueue&) = delete;
    ~NativeGameTimerQueue();

    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    // IDA5ED0C0: release callbacks in event-then-timer order. Keep list nodes,
    // sentinels, counts and facade address; nonzero Update later unlinks nodes.
    void ReleaseCallbacks();
    [[nodiscard]] std::uint32_t Address() const noexcept { return address_; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::uint32_t size, std::uint32_t vtable,
        std::uint32_t owner, std::uint32_t method, std::uint32_t offset = 0u);
    // Original32-byte843CD4 callback owns a flagged, reference-counted location.
    using WorldMapCallback = std::function<void(std::uint32_t owner,
        std::uint32_t method, std::string_view location)>;
    void SetWorldMapCallback(WorldMapCallback callback);
    [[nodiscard]] std::uint32_t AllocateWorldMapStorage();
    [[nodiscard]] std::uint32_t ConstructWorldMapCallback(std::uint32_t storage,
        std::uint32_t owner, std::uint32_t method, std::string_view location,
        std::uint32_t adjustment = 0u);
    void DiscardCallback(std::uint32_t callback);
    void AddOwnedCallback(std::string_view name, float period,
                          std::uint32_t callback);
    void AddEvent(std::string_view name, float delay, std::uint32_t owner, std::uint32_t method);
    void AddPoetryResetEvent(float delay, std::uint32_t owner);
    //54D670: original16-byte callback84553C in the one-shot event list.
    void AddCombatRefreshEvent(std::string_view name,float delay,std::uint32_t owner);
    // Original Trade callback843D8C, invoking48DE50 with row/byte arguments.
    void AddTradeCallback(std::string_view name,float delay,std::uint32_t owner,
        std::uint32_t method,std::int32_t row,std::uint8_t flag,bool repeating);
    [[nodiscard]] std::uint32_t RemoveEvent(std::string_view name);
    [[nodiscard]] std::uint32_t EventCount() const noexcept;
    void AddRepeating(std::string_view name, float period,
                      std::uint32_t owner, std::uint32_t method);
    void AddRepeatingWithOffset(std::string_view name, float period,
        std::uint32_t owner, std::uint32_t method, std::uint32_t offset = 0u);
    [[nodiscard]] std::uint32_t Remove(std::string_view name);
    void Update(float elapsed);
    [[nodiscard]] std::uint32_t Count() const noexcept;
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    friend class NativeTimedEventInsertion;
    [[nodiscard]] TimedCallbackCollectionState& State() const;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void DeleteOwned(std::uint32_t address, std::uint32_t flags) override;
    std::uint32_t InvokeValue(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void InvokeEvent(std::uint32_t callback) override;
    void InvokeTimer(std::uint32_t callback) override;
    [[nodiscard]] std::uint32_t InvokeThiscall(
        std::uint32_t function, std::uint32_t context) override;
    void CleanupEventData(std::uint32_t payload) override;
    void CleanupTimerData(std::uint32_t payload) override;
    void FreeNode(std::uint32_t address) override;
    [[nodiscard]] int CompareBytes(std::uint32_t first,
                                   std::uint32_t second,
                                   std::uint32_t count) override;

    void EraseName(TimerQueueEntryState& entry, std::uint32_t offset,
                   std::uint32_t count) override;
    void SetTextFlag(TimerQueueEntryState& entry, std::uint8_t flag) override;
    [[nodiscard]] std::uint8_t ReadReferenceCount(std::uint32_t data) override;
    void WriteReferenceCount(std::uint32_t data, std::uint8_t count) override;
    [[nodiscard]] bool AllocateName(TimerQueueEntryState& entry,
                                    std::uint32_t length,
                                    std::uint8_t mode) override;
    void CopyName(std::uint32_t destination, std::uint32_t source,
                  std::uint32_t length) override;
    void SetNameSize(TimerQueueEntryState& entry,
                     std::uint32_t length) override;
    void ReleaseCallback(std::uint32_t callback,
                         std::uint32_t deleting) override;
    [[nodiscard]] std::uint32_t FreeName(std::uint32_t header) override;

    NativeBaseObjectMemory memory_;
    Callback callback_;
    ParameterCallback parameter_callback_;
    ValueCallback value_callback_;
    WorldMapCallback world_map_callback_;
    std::uint32_t address_{};
};

}  // namespace pal4
