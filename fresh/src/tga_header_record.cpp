#include "pal4/tga_header_record.h"

namespace pal4 {

std::uint32_t TgaHeaderRecord::Initialize(
    const std::uint32_t record, ITgaHeaderRecordMemory& memory) {
    memory.WriteByte(record + 0x50u, 0xc4u);
    memory.WriteByte(record + 0x51u, 0xc4u);
    memory.WriteByte(record + 0x52u, 0u);
    memory.WriteByte(record + 0x53u, 0xffu);
    return record;
}

}  // namespace pal4
