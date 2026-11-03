#pragma once

#include <cstdint>

namespace pal4 {

struct TimedQueueRecord {
    std::uint32_t name_flag = 0;
    std::uint32_t name_data = 0;
    std::uint32_t name_length = 0;
    std::uint32_t name_capacity = 0;
    std::uint32_t delay = 0;
    std::uint32_t owns_callback = 0;
    std::uint32_t callback = 0;
};

class ITimedEventQueueRuntime {
public:
    virtual ~ITimedEventQueueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void InitializeName(TimedQueueRecord& record,
                                std::uint32_t source) = 0;
    virtual void CopyRecord(std::uint32_t destination,
                            TimedQueueRecord& source) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t delete_flag) = 0;
    virtual void ReleaseName(TimedQueueRecord& record) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNameResult(
        TimedQueueRecord& record) {
        ReleaseName(record);
        return 0u;
    }
};

class TimedEventQueue final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8440d0u;

    // IDA 0x4ae920. Queue fields +16/+20 hold the list head and count.
    static void Add(std::uint32_t queue, std::uint32_t name,
                    std::uint32_t delay, std::uint32_t callback_owner,
                    std::uint32_t callback_function,
                    ITimedEventQueueRuntime& runtime);
    // The queue insertion at 0x4b3980 differs only in callback vtable.
    static void AddWithCallbackVtable(
        std::uint32_t queue, std::uint32_t name,
        std::uint32_t delay, std::uint32_t callback_owner,
        std::uint32_t callback_function, std::uint32_t callback_vtable,
        ITimedEventQueueRuntime& runtime);
    // IDA 0x57af90. The callback object is already allocated.
    [[nodiscard]] static std::uint32_t AddExistingCallback(
        std::uint32_t queue, std::uint32_t name,
        std::uint32_t delay, std::uint32_t callback,
        ITimedEventQueueRuntime& runtime);
};

class ITimedQueueRecordCopyRuntime {
public:
    virtual ~ITimedQueueRecordCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetNameErrorFlag(TimedQueueRecord& record,
                                  std::uint8_t flag) = 0;
    [[nodiscard]] virtual bool AllocateName(TimedQueueRecord& record,
                                            std::uint32_t length,
                                            std::uint8_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
    virtual void SetNameSize(TimedQueueRecord& record,
                             std::uint32_t length) = 0;
    virtual void EraseName(TimedQueueRecord& record,
                           std::uint32_t start,
                           std::uint32_t count) = 0;
};

class TimedQueueRecordCopier final {
public:
    // IDA 0x4aea60. Copies the 28-byte item and transfers its owner byte.
    [[nodiscard]] static TimedQueueRecord& CopyAssign(
        TimedQueueRecord& destination, TimedQueueRecord& source,
        ITimedQueueRecordCopyRuntime& runtime);
};

}  // namespace pal4
