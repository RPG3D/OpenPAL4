#include "pal4/magic_window_skill_catalog.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

void TestEmptyAndNoMatch() {
    const std::array<pal4::MagicDbSkillRecord, 0> empty{};
    const auto empty_selection = pal4::MagicWindowSkillCatalog::Select(empty, 3u);
    Check(empty_selection.matching_indices.empty() &&
              empty_selection.VisibleCount() == 0u, "empty table");
    const std::array<pal4::MagicDbSkillRecord, 2> other{{
        {.category = 1u}, {.category = 2u}}};
    const auto other_selection = pal4::MagicWindowSkillCatalog::Select(other, 3u);
    Check(other_selection.matching_indices.empty(), "no matching category");
}

void TestOrderAndLimit() {
    std::array<pal4::MagicDbSkillRecord, 12> records{};
    for (std::size_t index = 0; index < records.size(); ++index) {
        records[index].key = static_cast<std::uint32_t>(100u + index);
        records[index].category = index == 2u || index == 8u ? 9u : 3u;
    }
    const auto selection = pal4::MagicWindowSkillCatalog::Select(records, 3u);
    Check(selection.matching_indices ==
              std::vector<std::size_t>{0u, 1u, 3u, 4u, 5u, 6u, 7u, 9u, 10u, 11u},
          "all matches retain original table order");
    Check(selection.VisibleCount() == 6u &&
              records[selection.matching_indices[5]].key == 106u &&
              records[selection.matching_indices[6]].key == 107u,
          "first six displayed while remaining matches stay collected");
}

void TestExactField() {
    std::array<pal4::MagicDbSkillRecord, 3> records{};
    records[0].key = 7u;
    records[0].category = 4u;
    records[0].kind = 9u;
    records[1].category = 4u;
    records[1].name_address = 4u;
    records[2].category = 5u;
    const auto selection = pal4::MagicWindowSkillCatalog::Select(records, 4u);
    Check(selection.matching_indices == std::vector<std::size_t>{0u, 1u} &&
              selection.VisibleCount() == 2u,
          "field plus four alone controls the predicate");
}

void TestTypeIcons() {
    using Catalog = pal4::MagicWindowSkillCatalog;
    Check(Catalog::IconForKind(0u) == "skillattack" &&
              Catalog::IconForKind(2u) == "skillcure" &&
              Catalog::IconForKind(1u) == "skillbuff" &&
              Catalog::IconForKind(3u) == "skillbuff" &&
              Catalog::IconForKind(0xffffffffu) == "skillbuff",
          "record field plus eight maps to three gy0 skill icons");
}

void TestButtonImages() {
    using Catalog = pal4::MagicWindowSkillCatalog;
    const auto learned = Catalog::ButtonImages(true);
    Check(learned.normal == "bottonskill" &&
              learned.highlight == "bottonskill1" &&
              learned.disabled == "bottonskill3" &&
              learned.pushed == "bottonskill2",
          "threshold-met button states");
    const auto unlearned = Catalog::ButtonImages(false);
    Check(unlearned.normal == "bottonskilllearn" &&
              unlearned.highlight == "bottonskilllearn1" &&
              unlearned.disabled == "bottonskilllearn3" &&
              unlearned.pushed == "bottonskilllearn2",
          "below-threshold button states");
}

void TestSignedThreshold() {
    pal4::MagicDbSkillRecord record;
    record.threshold_3c = 5u;
    Check(!pal4::MagicWindowSkillCatalog::MeetsThreshold(4, record) &&
              pal4::MagicWindowSkillCatalog::MeetsThreshold(5, record) &&
              pal4::MagicWindowSkillCatalog::MeetsThreshold(6, record),
          "signed inclusive threshold");
    record.threshold_3c = 0xffffffffu;
    Check(!pal4::MagicWindowSkillCatalog::MeetsThreshold(-2, record) &&
              pal4::MagicWindowSkillCatalog::MeetsThreshold(-1, record) &&
              pal4::MagicWindowSkillCatalog::MeetsThreshold(0, record),
          "negative threshold follows jge signed comparison");
}

void TestActionPlan() {
    pal4::MagicDbSkillRecord record;
    record.threshold_3c = 5u;
    record.minimum_total_40 = 8u;
    using Catalog = pal4::MagicWindowSkillCatalog;

    const auto learned_usable = Catalog::ChooseAction(record, 5, 0, 0, true);
    Check(learned_usable.trigger_id == 0x16u && learned_usable.enabled &&
              !learned_usable.format_value,
          "learned usable skill uses trigger 0x16");
    const auto learned_blocked = Catalog::ChooseAction(record, 6, 100, 100, false);
    Check(learned_blocked.trigger_id == 0x107u && !learned_blocked.enabled &&
              !learned_blocked.format_value,
          "learned blocked skill uses trigger 0x107");

    const auto learnable = Catalog::ChooseAction(record, 4, 8, 5, false);
    Check(learnable.trigger_id == 0xbcu && learnable.enabled &&
              learnable.format_value == 5,
          "both inclusive resource gates permit learning");
    Check(!Catalog::ChooseAction(record, 4, 7, 5, true).enabled &&
              !Catalog::ChooseAction(record, 4, 8, 4, true).enabled,
          "either missing resource disables learning");

    record.threshold_3c = 0xffffffffu;
    record.minimum_total_40 = 0xfffffffeu;
    Check(Catalog::ChooseAction(record, -2, -2, -1, false).enabled &&
              !Catalog::ChooseAction(record, -2, -3, -1, false).enabled &&
              !Catalog::ChooseAction(record, -2, -2, -2, false).enabled,
          "both resource comparisons are signed");
}

}  // namespace

int main() {
    TestEmptyAndNoMatch();
    TestOrderAndLimit();
    TestExactField();
    TestTypeIcons();
    TestButtonImages();
    TestSignedThreshold();
    TestActionPlan();
}
