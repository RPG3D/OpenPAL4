#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class CeguiTextureState;

struct CeguiBitCursor final {
    std::uint32_t bit = 0u;
    std::uint32_t word_address = 0u;
};

struct CeguiBitReference final {
    std::uint32_t mask = 0u;
    std::uint32_t word_address = 0u;
};

class ICeguiBitWordMemory {
public:
    virtual ~ICeguiBitWordMemory() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class CeguiBitWordMemoryStub : public ICeguiBitWordMemory {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t address) override;
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
};

class CeguiBitArrayHelpers final {
public:
    // 0x416140, 0x416170, 0x416190.
    [[nodiscard]] static CeguiBitCursor& OffsetCursor(
        const CeguiBitCursor& source, CeguiBitCursor& output,
        std::uint32_t offset) noexcept;
    [[nodiscard]] static std::int32_t Distance(
        const CeguiBitCursor& cursor,
        const CeguiBitCursor& base) noexcept;
    [[nodiscard]] static CeguiBitCursor& Begin(
        const CeguiTextureState& state,
        CeguiBitCursor& output) noexcept;

    // 0x4161b0: bit-vector insertion, including backward tail movement.
    static void Insert(CeguiTextureState& state, std::uint32_t index,
                       std::uint32_t count, bool value);

    // 0x4163f0 through 0x4164b0: bit-reference and cursor operations.
    [[nodiscard]] static CeguiBitReference& WriteBit(
        CeguiBitReference& reference, bool value,
        ICeguiBitWordMemory& memory);
    [[nodiscard]] static std::uint32_t Decrement(
        CeguiBitCursor& cursor) noexcept;
    [[nodiscard]] static std::uint32_t Increment(
        CeguiBitCursor& cursor) noexcept;
    [[nodiscard]] static CeguiBitReference& MakeReference(
        const CeguiBitCursor& cursor,
        CeguiBitReference& output) noexcept;
    [[nodiscard]] static CeguiBitCursor& Advance(
        CeguiBitCursor& cursor, std::uint32_t offset) noexcept;
    [[nodiscard]] static bool Equal(
        const CeguiBitCursor& left,
        const CeguiBitCursor& right) noexcept;
};

}  // namespace pal4
