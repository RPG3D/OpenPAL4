#include "pal4/cegui_bit_array.h"
#include "pal4/cegui_texture.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void OffsetCursorCrossesWordAndReturnsOutput() {
    const pal4::CeguiBitCursor source{29u, 0x1000u};
    pal4::CeguiBitCursor output{};
    Check(&pal4::CeguiBitArrayHelpers::OffsetCursor(
              source, output, 5u) == &output &&
              output.bit == 2u && output.word_address == 0x1004u &&
              source.bit == 29u && source.word_address == 0x1000u,
          "offset returns new cursor without changing source");
}

void DistanceCountsBitsAcrossWords() {
    const pal4::CeguiBitCursor base{5u, 0x1000u};
    Check(pal4::CeguiBitArrayHelpers::Distance(
              {3u, 0x1008u}, base) == 62 &&
              pal4::CeguiBitArrayHelpers::Distance(
                  {1u, 0x0ffcu}, base) == -36,
          "distance applies signed word delta and bit offset");
}

void BeginUsesCollectionStorageAddress() {
    pal4::CeguiTextureState state;
    state.WriteDword(24u, 0x34567890u);
    pal4::CeguiBitCursor output{17u, 0u};
    Check(&pal4::CeguiBitArrayHelpers::Begin(state, output) == &output &&
              output.bit == 0u && output.word_address == 0x34567890u,
          "begin resets bit and reads collection pointer at +24");
}

void InsertMovesTailAcrossWordBoundary() {
    pal4::CeguiTextureState state;
    pal4::CeguiBitArrayHelpers::Insert(state, 0u, 31u, false);
    pal4::CeguiBitArrayHelpers::Insert(state, 31u, 1u, true);
    pal4::CeguiBitArrayHelpers::Insert(state, 0u, 2u, true);
    Check(state.MaskBitCount() == 34u &&
              state.MaskBit(0u) && state.MaskBit(1u) &&
              !state.MaskBit(2u) && !state.MaskBit(32u) &&
              state.MaskBit(33u),
          "insertion shifts existing tail past the next DWORD");
    pal4::CeguiBitArrayHelpers::Insert(state, 1u, 2u, false);
    Check(state.MaskBitCount() == 36u && state.MaskBit(0u) &&
              !state.MaskBit(1u) && !state.MaskBit(2u) &&
              state.MaskBit(3u) && state.MaskBit(35u),
          "middle insertion fills zero bits and shifts later ones");
    pal4::CeguiBitArrayHelpers::Insert(state, 36u, 0u, true);
    Check(state.MaskBitCount() == 36u,
          "zero-bit insertion leaves collection unchanged");
    bool rejected = false;
    try {
        pal4::CeguiBitArrayHelpers::Insert(state, 37u, 1u, true);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected && state.MaskBitCount() == 36u,
          "C++ boundary rejects insertion after the end");
}

void BitReferenceWritesOnlySelectedBit() {
    pal4::CeguiBitWordMemoryStub memory;
    memory.WriteWord(0x2000u, 0xa0u);
    pal4::CeguiBitReference reference{0x10u, 0x2000u};
    Check(&pal4::CeguiBitArrayHelpers::WriteBit(
              reference, true, memory) == &reference &&
              memory.ReadWord(0x2000u) == 0xb0u,
          "set-bit preserves unrelated bits and return identity");
    Check(&pal4::CeguiBitArrayHelpers::WriteBit(
              reference, false, memory) == &reference &&
              memory.ReadWord(0x2000u) == 0xa0u,
          "clear-bit preserves unrelated bits");
}

void DecrementHandlesBothBranches() {
    pal4::CeguiBitCursor cursor{3u, 0x2000u};
    Check(pal4::CeguiBitArrayHelpers::Decrement(cursor) == 2u &&
              cursor.bit == 2u && cursor.word_address == 0x2000u,
          "decrement inside word returns bit index");
    cursor.bit = 0u;
    Check(pal4::CeguiBitArrayHelpers::Decrement(cursor) == 0x1ffcu &&
              cursor.bit == 31u && cursor.word_address == 0x1ffcu,
          "decrement from zero returns previous word address");
}

void IncrementHandlesBothBranches() {
    pal4::CeguiBitCursor cursor{30u, 0x2000u};
    Check(pal4::CeguiBitArrayHelpers::Increment(cursor) == 31u &&
              cursor.bit == 31u && cursor.word_address == 0x2000u,
          "increment inside word returns bit index");
    Check(pal4::CeguiBitArrayHelpers::Increment(cursor) == 0x2004u &&
              cursor.bit == 0u && cursor.word_address == 0x2004u,
          "increment from bit 31 returns next word address");
}

void MakeReferenceAndAdvanceUseCursorFields() {
    pal4::CeguiBitCursor cursor{30u, 0x3000u};
    pal4::CeguiBitReference reference{};
    Check(&pal4::CeguiBitArrayHelpers::MakeReference(
              cursor, reference) == &reference &&
              reference.mask == 0x40000000u &&
              reference.word_address == 0x3000u,
          "reference carries one-bit mask and word pointer");
    Check(&pal4::CeguiBitArrayHelpers::Advance(cursor, 35u) == &cursor &&
              cursor.bit == 1u && cursor.word_address == 0x3008u,
          "advance handles multiple word crossings");
}

void EqualComparesBothFields() {
    Check(pal4::CeguiBitArrayHelpers::Equal(
              {7u, 0x4000u}, {7u, 0x4000u}) &&
              !pal4::CeguiBitArrayHelpers::Equal(
                  {7u, 0x4000u}, {8u, 0x4000u}) &&
              !pal4::CeguiBitArrayHelpers::Equal(
                  {7u, 0x4000u}, {7u, 0x4004u}),
          "equality requires same bit and word address");
}

}  // namespace

int main() {
    OffsetCursorCrossesWordAndReturnsOutput();
    DistanceCountsBitsAcrossWords();
    BeginUsesCollectionStorageAddress();
    InsertMovesTailAcrossWordBoundary();
    BitReferenceWritesOnlySelectedBit();
    DecrementHandlesBothBranches();
    IncrementHandlesBothBranches();
    MakeReferenceAndAdvanceUseCursorFields();
    EqualComparesBothFields();
    std::cout << "9 CEGUI bit-array test cases passed\n";
}
