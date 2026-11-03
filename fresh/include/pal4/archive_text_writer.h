#pragma once

#include "pal4/archive_stream_adapter.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ArchiveTextRecordState {
    std::uint32_t tag{};
    std::uint32_t data{};
    std::uint32_t byte_count{};
};
static_assert(offsetof(ArchiveTextRecordState, data) == 4u);
static_assert(offsetof(ArchiveTextRecordState, byte_count) == 8u);

class IArchiveTextRuntime {
public:
    virtual ~IArchiveTextRuntime() = default;
    [[nodiscard]] virtual const void* DataAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual const void* EmptyData() = 0;
};

class ArchiveTextWriter final {
public:
    ArchiveTextWriter(ArchiveStreamAdapter& adapter,
                      IArchiveTextRuntime& runtime)
        : adapter_(adapter), runtime_(runtime) {}

    // 0x5ec330: write byte count, then record data if the first write succeeds.
    [[nodiscard]] std::uint8_t Write(
        const ArchiveStreamAdapterState& stream,
        const ArchiveTextRecordState& record);

private:
    ArchiveStreamAdapter& adapter_;
    IArchiveTextRuntime& runtime_;
};

}  // namespace pal4
