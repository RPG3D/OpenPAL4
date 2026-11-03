#include "pal4/cegui_collection_helpers.h"

#include <bit>

namespace pal4 {

std::uint32_t CeguiCollectionMemoryStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }

void CeguiCollectionMemoryStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

void CeguiCollectionMemoryStub::CopyRect(
    const std::uint32_t destination, const std::uint32_t source) {
    for (std::uint32_t offset = 0u; offset < 16u; offset += 4u) {
        WriteDword(destination + offset, ReadDword(source + offset));
    }
}

std::int32_t CeguiCollectionHelpers::CountWords(
    const CeguiCollectionHeader& header) noexcept {
    if (header.begin == 0u) {
        return 0;
    }
    return std::bit_cast<std::int32_t>(header.end - header.begin) >> 1;
}

std::uint32_t CeguiCollectionHelpers::CopyRecords60(
    std::uint32_t source_begin, const std::uint32_t source_end,
    std::uint32_t destination, ICeguiCollectionMemory& memory) {
    while (source_begin != source_end) {
        if (destination != 0u) {
            for (std::uint32_t offset = 0u; offset < 60u; offset += 4u) {
                memory.WriteDword(destination + offset,
                                  memory.ReadDword(source_begin + offset));
            }
        }
        source_begin += 60u;
        destination += 60u;
    }
    return destination;
}

std::int32_t CeguiCollectionHelpers::CountElements28(
    const CeguiCollectionHeader& header) noexcept {
    if (header.begin == 0u) {
        return 0;
    }
    return std::bit_cast<std::int32_t>(header.end - header.begin) / 28;
}

namespace {

void CopyRaw60(const std::uint32_t destination,
               const std::uint32_t source,
               ICeguiCollectionMemory& memory) {
    for (std::uint32_t offset = 0u; offset < 60u; offset += 4u) {
        memory.WriteDword(destination + offset,
                          memory.ReadDword(source + offset));
    }
}

void CopyStructured60(const std::uint32_t destination,
                      const std::uint32_t source,
                      ICeguiCollectionMemory& memory) {
    memory.WriteDword(destination, memory.ReadDword(source));
    memory.CopyRect(destination + 4u, source + 4u);
    memory.WriteDword(destination + 20u,
                      memory.ReadDword(source + 20u));
    memory.CopyRect(destination + 24u, source + 24u);
    for (std::uint32_t offset = 40u; offset < 60u; offset += 4u) {
        memory.WriteDword(destination + offset,
                          memory.ReadDword(source + offset));
    }
}

}  // namespace

std::uint32_t CeguiCollectionHelpers::FillRecords60(
    std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t pattern, ICeguiCollectionMemory& memory) {
    if (count == 0u) {
        return 0u;
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        if (destination != 0u) {
            CopyRaw60(destination, pattern, memory);
        }
        destination += 60u;
    }
    return destination;
}

std::uint32_t CeguiCollectionHelpers::CopyStructuredRange60(
    std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t pattern, ICeguiCollectionMemory& memory) {
    while (begin != end) {
        CopyStructured60(begin, pattern, memory);
        begin += 60u;
    }
    return end;
}

std::uint32_t CeguiCollectionHelpers::MoveStructuredBackward60(
    const std::uint32_t source_begin, std::uint32_t source_end,
    std::uint32_t destination_end, ICeguiCollectionMemory& memory) {
    while (source_end != source_begin) {
        source_end -= 60u;
        destination_end -= 60u;
        CopyStructured60(destination_end, source_end, memory);
    }
    return destination_end;
}

void CeguiCollectionHelpers::CopyOne60(
    const std::uint32_t destination, const std::uint32_t source,
    ICeguiCollectionMemory& memory) {
    if (destination != 0u) {
        CopyRaw60(destination, source, memory);
    }
}

}  // namespace pal4
