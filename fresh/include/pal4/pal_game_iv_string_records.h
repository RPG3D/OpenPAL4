#pragma once

#include <cstdint>

namespace pal4 {

// A native PAL4 string record occupies three 32-bit words.
class IPalGameIVStringRecordRuntime {
public:
    virtual ~IPalGameIVStringRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Cleanup(std::uint32_t record) = 0;
    [[nodiscard]] virtual std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeBuffer(
        std::uint32_t destination, std::uint32_t data,
        std::uint32_t length) = 0;
    // The original routines leave EAX unspecified when no call is made.
    [[nodiscard]] virtual std::uint32_t EmptyResult() = 0;
};

class PalGameIVStringRecords final {
public:
    // 0x5b72b0: owner stores begin/end/capacity at +4/+8/+12.
    [[nodiscard]] static std::uint32_t CleanupAndFree(
        std::uint32_t owner, IPalGameIVStringRecordRuntime& runtime);
    // 0x5b72f0: destroy [begin, end), retaining its allocation.
    [[nodiscard]] static std::uint32_t CleanupRange(
        std::uint32_t begin, std::uint32_t end,
        IPalGameIVStringRecordRuntime& runtime);
    // 0x5b7320: copy construct [source_begin, source_end) at destination.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination, IPalGameIVStringRecordRuntime& runtime);
    // 0x5b7360: repeatedly copy construct one source into consecutive slots.
    [[nodiscard]] static std::uint32_t CopyCount(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t source, IPalGameIVStringRecordRuntime& runtime);
    // 0x5b7390: initialize every destination from one buffer descriptor.
    [[nodiscard]] static std::uint32_t InitializeRange(
        std::uint32_t destination_begin, std::uint32_t destination_end,
        std::uint32_t source, IPalGameIVStringRecordRuntime& runtime);
    // 0x5b73c0: initialize backwards from each source descriptor.
    [[nodiscard]] static std::uint32_t InitializeReverse(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination_end, IPalGameIVStringRecordRuntime& runtime);
};

}  // namespace pal4
