#include "pal4/cegui_bit_array.h"

#include "pal4/cegui_texture.h"

namespace pal4 {

std::uint32_t CeguiBitWordMemoryStub::ReadWord(
    const std::uint32_t address) { return words_.at(address); }
void CeguiBitWordMemoryStub::WriteWord(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

CeguiBitCursor& CeguiBitArrayHelpers::OffsetCursor(
    const CeguiBitCursor& source, CeguiBitCursor& output,
    const std::uint32_t offset) noexcept {
    const auto absolute = source.bit + offset;
    output.bit = absolute & 31u;
    output.word_address = source.word_address + 4u * (absolute >> 5u);
    return output;
}

std::int32_t CeguiBitArrayHelpers::Distance(
    const CeguiBitCursor& cursor,
    const CeguiBitCursor& base) noexcept {
    const auto byte_delta = static_cast<std::int32_t>(
        cursor.word_address - base.word_address);
    const auto word_delta = byte_delta >= 0
        ? byte_delta / 4
        : -static_cast<std::int32_t>(
              (-(static_cast<std::int64_t>(byte_delta)) + 3) / 4);
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(cursor.bit + 32u * word_delta - base.bit));
}

CeguiBitCursor& CeguiBitArrayHelpers::Begin(
    const CeguiTextureState& state, CeguiBitCursor& output) noexcept {
    output.bit = 0u;
    output.word_address = state.ReadDword(24u);
    return output;
}

void CeguiBitArrayHelpers::Insert(
    CeguiTextureState& state, const std::uint32_t index,
    const std::uint32_t count, const bool value) {
    state.InsertMaskBits(index, count, value);
}

CeguiBitReference& CeguiBitArrayHelpers::WriteBit(
    CeguiBitReference& reference, const bool value,
    ICeguiBitWordMemory& memory) {
    const auto original = memory.ReadWord(reference.word_address);
    memory.WriteWord(reference.word_address,
                     value ? original | reference.mask
                           : original & ~reference.mask);
    return reference;
}

std::uint32_t CeguiBitArrayHelpers::Decrement(
    CeguiBitCursor& cursor) noexcept {
    if (cursor.bit != 0u) {
        return --cursor.bit;
    }
    cursor.bit = 31u;
    cursor.word_address -= 4u;
    return cursor.word_address;
}

std::uint32_t CeguiBitArrayHelpers::Increment(
    CeguiBitCursor& cursor) noexcept {
    if (cursor.bit < 31u) {
        return ++cursor.bit;
    }
    cursor.bit = 0u;
    cursor.word_address += 4u;
    return cursor.word_address;
}

CeguiBitReference& CeguiBitArrayHelpers::MakeReference(
    const CeguiBitCursor& cursor,
    CeguiBitReference& output) noexcept {
    output.word_address = cursor.word_address;
    output.mask = 1u << cursor.bit;
    return output;
}

CeguiBitCursor& CeguiBitArrayHelpers::Advance(
    CeguiBitCursor& cursor, const std::uint32_t offset) noexcept {
    CeguiBitCursor output;
    static_cast<void>(OffsetCursor(cursor, output, offset));
    cursor = output;
    return cursor;
}

bool CeguiBitArrayHelpers::Equal(
    const CeguiBitCursor& left,
    const CeguiBitCursor& right) noexcept {
    return left.word_address == right.word_address && left.bit == right.bit;
}

}  // namespace pal4
