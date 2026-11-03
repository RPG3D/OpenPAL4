#pragma once

#include <cstdint>

namespace pal4 {

class IJigsawRecordVectorRuntime {
public:
    virtual ~IJigsawRecordVectorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class JigsawRecordVector final {
public:
    // IDA 0x4a08c0.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t vector, IJigsawRecordVectorRuntime& runtime);
    // IDA 0x4a0950; the second argument is ignored by the native wrapper.
    [[nodiscard]] static std::uint32_t FreeBuffer(
        std::uint32_t buffer, std::uint32_t ignored,
        IJigsawRecordVectorRuntime& runtime);
    // IDA 0x4a0960.
    static void DestroyRecord(std::uint32_t record,
                              IJigsawRecordVectorRuntime& runtime);
};

class IEventQueueRuntime {
public:
    virtual ~IEventQueueRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyEventData(std::uint32_t destination,
                               std::uint32_t source) = 0;
};

class EventQueue final {
public:
    // IDA 0x4a0900.
    [[nodiscard]] static std::uint32_t Add(
        std::uint32_t queue, std::uint32_t source,
        IEventQueueRuntime& runtime);
};

class IJigsawEventCallbackRuntime {
public:
    virtual ~IJigsawEventCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t object, std::uint32_t slot_offset) = 0;
};

class JigsawEventCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x843e3cu;

    // IDA 0x4a0970.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IJigsawEventCallbackRuntime& runtime);
    // IDA 0x4a09c0.
    [[nodiscard]] static std::uint32_t Invoke(
        std::uint32_t object, IJigsawEventCallbackRuntime& runtime);
};

class PuzzleEventCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x843e94u;

    // IDA 0x4a1c90.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IJigsawEventCallbackRuntime& runtime);
};

}  // namespace pal4
