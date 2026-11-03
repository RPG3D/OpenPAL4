#include "pal4/d3dx_shader_intrinsic_table.h"

#include <algorithm>
#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::uint8_t, 0x4018> kImage = {
#include "d3dx_shader_intrinsic_table.inc"
};

constexpr std::uint32_t RuleAddress(std::size_t rule) {
    return D3DXShaderIntrinsicTable::kTableBase +
           static_cast<std::uint32_t>(rule * 164);
}

}  // namespace

std::uint32_t D3DXShaderIntrinsicTable::Word(std::uint32_t address) {
    if (address < kImageBase || address - kImageBase > kImage.size() - 4)
        return 0;
    const auto offset = static_cast<std::size_t>(address - kImageBase);
    return static_cast<std::uint32_t>(kImage[offset]) |
           static_cast<std::uint32_t>(kImage[offset + 1]) << 8 |
           static_cast<std::uint32_t>(kImage[offset + 2]) << 16 |
           static_cast<std::uint32_t>(kImage[offset + 3]) << 24;
}

std::string_view D3DXShaderIntrinsicTable::CString(
    std::uint32_t address) {
    if (address < kImageBase || address - kImageBase >= kImage.size())
        return {};
    const auto begin = static_cast<std::size_t>(address - kImageBase);
    const auto end = std::find(kImage.begin() + begin, kImage.end(), 0);
    return {reinterpret_cast<const char*>(kImage.data() + begin),
            static_cast<std::size_t>(end - (kImage.begin() + begin))};
}

std::uint32_t D3DXShaderIntrinsicTable::Opcode(std::size_t rule) {
    return rule < kRuleCount ? Word(RuleAddress(rule)) : 0;
}

std::string_view D3DXShaderIntrinsicTable::Name(std::size_t rule) {
    return rule < kRuleCount ? CString(Word(RuleAddress(rule) + 4)) :
                               std::string_view{};
}

std::uint32_t D3DXShaderIntrinsicTable::SlotWord(
    std::size_t rule, std::size_t slot, std::size_t field) {
    if (rule >= kRuleCount || slot >= kSlotsPerRule ||
        field >= kWordsPerSlot)
        return 0;
    return Word(RuleAddress(rule) + 4 +
                static_cast<std::uint32_t>(slot * 32 + field * 4));
}

std::pair<std::size_t, std::size_t> D3DXShaderIntrinsicTable::FindRange(
    std::string_view name) {
    std::size_t first = 0;
    std::size_t end = kRuleCount;
    while (first < end) {
        const auto middle = (first + end) / 2;
        if (Name(middle) < name) first = middle + 1;
        else end = middle;
    }
    const auto lower = first;
    end = kRuleCount;
    while (first < end) {
        const auto middle = (first + end) / 2;
        if (Name(middle) <= name) first = middle + 1;
        else end = middle;
    }
    return {lower, first};
}

}  // namespace pal4
