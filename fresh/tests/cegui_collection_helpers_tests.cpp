#include "pal4/cegui_collection_helpers.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Memory final : public pal4::CeguiCollectionMemoryStub {
public:
    std::vector<std::uint32_t> reads;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> rect_copies;

    std::uint32_t ReadDword(const std::uint32_t address) override {
        reads.push_back(address);
        return pal4::CeguiCollectionMemoryStub::ReadDword(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        writes.emplace_back(address, value);
        pal4::CeguiCollectionMemoryStub::WriteDword(address, value);
    }
    void CopyRect(const std::uint32_t destination,
                  const std::uint32_t source) override {
        rect_copies.emplace_back(destination, source);
        pal4::CeguiCollectionMemoryStub::CopyRect(destination, source);
    }
};

void SeedRecord(Memory& memory, const std::uint32_t address,
                const std::uint32_t first) {
    for (std::uint32_t index = 0u; index < 15u; ++index) {
        memory.WriteDword(address + 4u * index, first + index);
    }
}

void CheckRecord(Memory& memory, const std::uint32_t address,
                 const std::uint32_t first) {
    for (std::uint32_t index = 0u; index < 15u; ++index) {
        Check(memory.ReadDword(address + 4u * index) == first + index,
              "all fifteen fields in a 60-byte record are copied");
    }
}

void WordCountUsesArithmeticShift() {
    pal4::CeguiCollectionHeader header{0xfeedu, 0u, 0x2000u};
    Check(pal4::CeguiCollectionHelpers::CountWords(header) == 0,
          "null begin returns zero even when end is nonzero");
    header.begin = 0x1000u;
    header.end = 0x1007u;
    Check(pal4::CeguiCollectionHelpers::CountWords(header) == 3,
          "positive odd difference shifts by one");
    header.end = 0x0ffdu;
    Check(pal4::CeguiCollectionHelpers::CountWords(header) == -2,
          "negative odd difference rounds downward as SAR");
}

void ElementCountUsesSignedDivision() {
    pal4::CeguiCollectionHeader header{0u, 0u, 0x1000u};
    Check(pal4::CeguiCollectionHelpers::CountElements28(header) == 0,
          "null begin yields zero elements");
    header.begin = 0x1000u;
    header.end = 0x1059u;
    Check(pal4::CeguiCollectionHelpers::CountElements28(header) == 3,
          "element count truncates positive excess bytes");
    header.end = 0x0fe3u;
    Check(pal4::CeguiCollectionHelpers::CountElements28(header) == -1,
          "negative 29-byte difference divides toward zero");
}

void EmptyCopyPreservesDestination() {
    Memory memory;
    Check(pal4::CeguiCollectionHelpers::CopyRecords60(
              0x2000u, 0x2000u, 0x3000u, memory) == 0x3000u &&
              memory.reads.empty() && memory.writes.empty(),
          "empty source range does not access memory");
}

void NonemptyCopyUsesFifteenDwordsPerRecord() {
    Memory memory;
    for (std::uint32_t index = 0u; index < 30u; ++index) {
        memory.WriteDword(0x2000u + 4u * index, 0x9000u + index);
    }
    memory.writes.clear();
    const auto end = pal4::CeguiCollectionHelpers::CopyRecords60(
        0x2000u, 0x2078u, 0x3000u, memory);
    Check(end == 0x3078u && memory.reads.size() == 30u &&
              memory.writes.size() == 30u,
          "two 60-byte records advance both addresses and copy 30 dwords");
    for (std::uint32_t index = 0u; index < 30u; ++index) {
        Check(memory.reads[index] == 0x2000u + 4u * index &&
                  memory.writes[index] ==
                      std::pair<std::uint32_t, std::uint32_t>{
                          0x3000u + 4u * index, 0x9000u + index},
              "copy preserves all dwords and source order");
    }
}

void NullDestinationSkipsCopyButAdvances() {
    Memory memory;
    Check(pal4::CeguiCollectionHelpers::CopyRecords60(
              0x2000u, 0x203cu, 0u, memory) == 60u &&
              memory.reads.empty() && memory.writes.empty(),
          "null destination skips one record but advances 60 bytes");
}

void PatternFillRetainsZeroCountReturnAndNullSkip() {
    Memory memory;
    SeedRecord(memory, 0x2000u, 0x100u);
    memory.reads.clear();
    memory.writes.clear();
    Check(pal4::CeguiCollectionHelpers::FillRecords60(
              0x3000u, 0u, 0x2000u, memory) == 0u &&
              memory.reads.empty() && memory.writes.empty(),
          "zero pattern count returns zero without memory access");
    Check(pal4::CeguiCollectionHelpers::FillRecords60(
              0u, 2u, 0x2000u, memory) == 120u &&
              memory.writes.size() == 15u,
          "null first destination skips first copy but advances to second");
    CheckRecord(memory, 60u, 0x100u);
}

void StructuredCopyCallsRectAssignmentsInOrder() {
    Memory memory;
    SeedRecord(memory, 0x2000u, 0x500u);
    memory.reads.clear();
    memory.writes.clear();
    Check(pal4::CeguiCollectionHelpers::CopyStructuredRange60(
              0x3000u, 0x3078u, 0x2000u, memory) == 0x3078u &&
              memory.rect_copies ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x3004u, 0x2004u}, {0x3018u, 0x2018u},
                      {0x3040u, 0x2004u}, {0x3054u, 0x2018u}},
          "each structured record invokes two imported Rect assignments");
    CheckRecord(memory, 0x3000u, 0x500u);
    CheckRecord(memory, 0x303cu, 0x500u);
    memory.rect_copies.clear();
    Check(pal4::CeguiCollectionHelpers::CopyStructuredRange60(
              0x3000u, 0x3000u, 0x2000u, memory) == 0x3000u &&
              memory.rect_copies.empty(),
          "empty structured range does not invoke Rect assignment");
}

void StructuredBackwardMoveHandlesOverlap() {
    Memory memory;
    SeedRecord(memory, 0x2000u, 0x100u);
    SeedRecord(memory, 0x203cu, 0x200u);
    memory.rect_copies.clear();
    Check(pal4::CeguiCollectionHelpers::MoveStructuredBackward60(
              0x2000u, 0x2078u, 0x20b4u, memory) == 0x203cu,
          "backward move returns final destination start");
    CheckRecord(memory, 0x203cu, 0x100u);
    CheckRecord(memory, 0x2078u, 0x200u);
    Check(memory.rect_copies.front() ==
              std::pair<std::uint32_t, std::uint32_t>{0x207cu, 0x2040u},
          "backward move starts at highest source record");
}

void SingleGuardedCopy() {
    Memory memory;
    SeedRecord(memory, 0x2000u, 0x700u);
    memory.reads.clear();
    memory.writes.clear();
    pal4::CeguiCollectionHelpers::CopyOne60(0u, 0x2000u, memory);
    Check(memory.reads.empty() && memory.writes.empty(),
          "null single-record destination skips source reads");
    pal4::CeguiCollectionHelpers::CopyOne60(0x3000u, 0x2000u, memory);
    CheckRecord(memory, 0x3000u, 0x700u);
}

}  // namespace

int main() {
    WordCountUsesArithmeticShift();
    ElementCountUsesSignedDivision();
    EmptyCopyPreservesDestination();
    NonemptyCopyUsesFifteenDwordsPerRecord();
    NullDestinationSkipsCopyButAdvances();
    PatternFillRetainsZeroCountReturnAndNullSkip();
    StructuredCopyCallsRectAssignmentsInOrder();
    StructuredBackwardMoveHandlesOverlap();
    SingleGuardedCopy();
    std::cout << "9 CEGUI collection helper test cases passed\n";
}
