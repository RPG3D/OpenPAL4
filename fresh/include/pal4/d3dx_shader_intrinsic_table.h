#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace pal4 {

class D3DXShaderIntrinsicTable final {
public:
    static constexpr std::uint32_t kImageBase = 0x850f98;
    static constexpr std::uint32_t kTableBase = 0x851078;
    static constexpr std::size_t kRuleCount = 94;
    static constexpr std::size_t kSlotsPerRule = 5;
    static constexpr std::size_t kWordsPerSlot = 8;

    [[nodiscard]] static std::uint32_t Word(std::uint32_t address);
    [[nodiscard]] static std::string_view CString(std::uint32_t address);
    [[nodiscard]] static std::uint32_t Opcode(std::size_t rule);
    [[nodiscard]] static std::string_view Name(std::size_t rule);
    [[nodiscard]] static std::uint32_t SlotWord(
        std::size_t rule, std::size_t slot, std::size_t field);
    [[nodiscard]] static std::pair<std::size_t, std::size_t> FindRange(
        std::string_view name);
};

}  // namespace pal4
