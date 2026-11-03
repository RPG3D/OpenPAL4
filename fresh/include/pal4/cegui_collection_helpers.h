#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

struct CeguiCollectionHeader {
    std::uint32_t unused = 0u;
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
};
static_assert(sizeof(CeguiCollectionHeader) == 12u);

class ICeguiCollectionMemory {
public:
    virtual ~ICeguiCollectionMemory() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void CopyRect(std::uint32_t destination,
                          std::uint32_t source) = 0;
};

class CeguiCollectionMemoryStub : public ICeguiCollectionMemory {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    void CopyRect(std::uint32_t destination,
                  std::uint32_t source) override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
};

class CeguiCollectionHelpers final {
public:
    // 0x4156b0: arithmetic shift of the 32-bit byte difference.
    [[nodiscard]] static std::int32_t CountWords(
        const CeguiCollectionHeader& header) noexcept;
    // 0x4156d0: copies fifteen DWORDs per 60-byte record.
    [[nodiscard]] static std::uint32_t CopyRecords60(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination, ICeguiCollectionMemory& memory);
    // 0x415710: signed division of the byte difference by 28.
    [[nodiscard]] static std::int32_t CountElements28(
        const CeguiCollectionHeader& header) noexcept;
    // 0x415ab0: count zero returns zero, not the destination.
    [[nodiscard]] static std::uint32_t FillRecords60(
        std::uint32_t destination, std::uint32_t count,
        std::uint32_t pattern, ICeguiCollectionMemory& memory);
    // 0x415ae0: two Rect copies per 60-byte record.
    [[nodiscard]] static std::uint32_t CopyStructuredRange60(
        std::uint32_t begin, std::uint32_t end,
        std::uint32_t pattern, ICeguiCollectionMemory& memory);
    // 0x415b50: moves records backwards from source end.
    [[nodiscard]] static std::uint32_t MoveStructuredBackward60(
        std::uint32_t source_begin, std::uint32_t source_end,
        std::uint32_t destination_end, ICeguiCollectionMemory& memory);
    // 0x415bc0: one guarded 60-byte copy.
    static void CopyOne60(std::uint32_t destination,
                          std::uint32_t source,
                          ICeguiCollectionMemory& memory);
};

}  // namespace pal4
