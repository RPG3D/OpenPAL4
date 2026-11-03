#pragma once

#include <cstdint>

namespace pal4 {

class IMonsterManagerTreeLookupRuntime {
public:
    virtual ~IMonsterManagerTreeLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) const = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextData() const = 0;
    // DialogTextCheck(node+0x0c, input).
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t node_text, std::uint32_t input) = 0;
    // StringBuffer_CompareRange(node+0x0c, input).
    [[nodiscard]] virtual std::int32_t CompareRange(
        std::uint32_t node_text, std::uint32_t input) = 0;
};

class MonsterManagerTreeLookup final {
public:
    // IDA 0x438930: result is the candidate node or the tree header.
    [[nodiscard]] static std::uint32_t Search(
        std::uint32_t tree, std::uint32_t input,
        IMonsterManagerTreeLookupRuntime& runtime);
    // IDA 0x438a30: returns the lower-bound candidate without a final check.
    [[nodiscard]] static std::uint32_t LowerBoundAlternative(
        std::uint32_t tree, std::uint32_t input,
        IMonsterManagerTreeLookupRuntime& runtime);
};

}  // namespace pal4
