#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace pal4 {

struct MagicDbSkillRecord {
    std::uint32_t key{};
    std::uint32_t category{};
    std::uint32_t kind{};
    std::uint32_t unknown_0c{};
    std::uint32_t name_address{};
    std::array<std::byte, 0x3cu - 0x14u> unknown_14{};
    std::uint32_t threshold_3c{};
    std::uint32_t minimum_total_40{};
    std::uint32_t unknown_44{};
    std::uint32_t effect_id{};
    std::int32_t cast_cost{};
    std::array<std::byte, 0x98u - 0x50u> remaining{};
};

static_assert(sizeof(MagicDbSkillRecord) == 0x98u);
static_assert(offsetof(MagicDbSkillRecord, category) == 4u);
static_assert(offsetof(MagicDbSkillRecord, name_address) == 0x10u);
static_assert(offsetof(MagicDbSkillRecord, threshold_3c) == 0x3cu);
static_assert(offsetof(MagicDbSkillRecord, minimum_total_40) == 0x40u);
static_assert(offsetof(MagicDbSkillRecord, effect_id) == 0x48u);
static_assert(offsetof(MagicDbSkillRecord, cast_cost) == 0x4cu);

struct MagicSkillButtonImages {
    std::string_view normal;
    std::string_view highlight;
    std::string_view disabled;
    std::string_view pushed;
};

struct MagicSkillActionPlan {
    std::uint32_t trigger_id{};
    bool enabled{};
    std::optional<std::int32_t> format_value;
};

struct MagicSkillSelection {
    std::vector<std::size_t> matching_indices;

    std::size_t VisibleCount() const noexcept;
};

class MagicWindowSkillCatalog final {
public:
    static constexpr std::size_t kVisibleLimit = 6u;

    static MagicSkillSelection Select(std::span<const MagicDbSkillRecord> records,
                                       std::uint32_t category);
    static std::string_view IconForKind(std::uint32_t kind) noexcept;
    static bool MeetsThreshold(std::int32_t current_value,
                               const MagicDbSkillRecord& record) noexcept;
    static MagicSkillButtonImages ButtonImages(bool threshold_met) noexcept;
    static MagicSkillActionPlan ChooseAction(
        const MagicDbSkillRecord& record, std::int32_t current_value,
        std::int32_t total_value, std::int32_t remaining_value,
        bool skill_state_is_three) noexcept;
};

}  // namespace pal4
