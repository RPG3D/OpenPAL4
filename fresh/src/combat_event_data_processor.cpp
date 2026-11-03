#include "pal4/combat_event_data_processor.h"

#include <bit>

namespace pal4 {

bool CombatEventDataProcessor::Matches(
    std::uint32_t entry, std::uint32_t first_key,
    std::uint32_t second_key, ICombatEventDataRuntime& runtime) {
    return runtime.ReadWord(entry + 152u) == second_key &&
           runtime.ReadWord(entry + 148u) == first_key;
}

int CombatEventDataProcessor::Process(
    std::uint32_t actor, std::uint32_t first_key,
    std::uint32_t second_key, ICombatEventDataRuntime& runtime) {
    const auto database = runtime.GameDatabase();
    const auto event_table = runtime.ReadWord(database + 44u);
    const auto matches = runtime.InitializeMatches(
        static_cast<std::uint8_t>(second_key));
    try {
        std::uint32_t count = runtime.UpgradeRecordCount(event_table + 92u);
        for (std::uint32_t index = 0u; index < count; ++index) {
            const auto entry = runtime.ReadWord(event_table + 96u) +
                               index * 160u;
            if (Matches(entry, first_key, second_key, runtime)) {
                const auto matching_entry =
                    runtime.ReadWord(event_table + 96u) + index * 160u;
                runtime.InsertMatch(matches, matching_entry);
            }
            count = runtime.UpgradeRecordCount(event_table + 92u);
        }

        if (runtime.MatchCount(matches) != 0u) {
            const auto first = runtime.FirstMatch(matches);
            const auto lower_database = runtime.GameDatabase();
            const auto lower = runtime.LowerBound(
                lower_database + 32u, first_key, second_key);
            const auto upper_database = runtime.GameDatabase();
            const auto upper = runtime.UpperBound(
                upper_database + 32u, first_key, second_key);

            const auto current = std::bit_cast<std::int32_t>(
                runtime.ReadWord(actor + 224u));
            if (current < lower)
                runtime.WriteWord(actor + 224u,
                                  std::bit_cast<std::uint32_t>(lower));
            const auto adjusted = std::bit_cast<std::int32_t>(
                runtime.ReadWord(actor + 224u));
            if (adjusted > upper)
                runtime.SetValidatedProperty(actor, 5u, upper);

            const auto vtable = runtime.ReadWord(actor);
            runtime.CopyBytes(actor, first, 144u);
            runtime.WriteWord(actor, vtable);
            runtime.CleanupMatchRange(matches);
        }
    } catch (...) {
        (void)runtime.FreeMatches(matches);
        throw;
    }
    return runtime.FreeMatches(matches);
}

}  // namespace pal4
