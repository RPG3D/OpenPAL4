#pragma once

#include "pal4/ui_text_buffer_record.h"

#include <cstdint>

namespace pal4 {

class IPalGameObjectTextRecordRuntime : public IUiTextBufferRecordRuntime {
public:
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class PalGameObjectTextRecord final {
public:
    // IDA 0x5a8e40: initialize a 20-byte text and metadata record.
    static void InitializeAndCopy(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);

    // IDA 0x5a8fa0: deleting destructor; flag bit 0 owns the record itself.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t record, std::uint8_t flags,
        IPalGameObjectTextRecordRuntime& runtime);
};

}  // namespace pal4
