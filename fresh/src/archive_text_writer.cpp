#include "pal4/archive_text_writer.h"

namespace pal4 {

std::uint8_t ArchiveTextWriter::Write(
    const ArchiveStreamAdapterState& stream,
    const ArchiveTextRecordState& record) {
    auto byte_count = record.byte_count;
    if ((adapter_.Write(stream, &byte_count, 4u) & 0xffu) == 0u)
        return 0u;
    const auto data = record.data != 0u ?
        runtime_.DataAt(record.data) : runtime_.EmptyData();
    return static_cast<std::uint8_t>(
        adapter_.Write(stream, data, byte_count));
}

}  // namespace pal4
