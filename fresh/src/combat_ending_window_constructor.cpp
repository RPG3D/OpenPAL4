#include "pal4/combat_ending_window_constructor.h"
#include "pal4/combat_ending_window_prize_events.h"

#include <array>
#include <string>

namespace pal4 {

std::uint32_t CombatEndingWindowConstructor::Construct(
    const std::uint32_t window, const std::string_view name,
    ICombatEndingWindowConstructorRuntime& runtime) {
    runtime.LoadLayout(window, name);
    runtime.WriteWord(window + 40u, 0x8435a4u);
    // The original reads an indeterminate stack byte for this flag.
    runtime.WriteByte(window + 44u, 0u);
    runtime.WriteWord(window + 48u, runtime.CreateListNode());
    runtime.WriteWord(window + 52u, 0u);
    runtime.WriteWord(window + 56u, 0u);
    runtime.WriteWord(window + 60u, 0u);
    runtime.ConstructStream(window + 420u);
    for (const auto offset : {700u, 704u, 708u, 712u}) {
        runtime.WriteWord(window + offset, 0u);
    }
    runtime.WriteWord(window, kVtable);
    runtime.WriteWord(window + 40u, kEmbeddedVtable);

    constexpr std::array<std::string_view, 4> portraits{
        "yuntianheheng", "hanlingshaheng", "liumengliheng",
        "murongziyinheng"};
    for (std::uint32_t i = 0; i < portraits.size(); ++i) {
        runtime.WriteWord(window + 648u + 4u * i,
                          runtime.FindImage("zi0", portraits[i]));
    }

    for (std::uint32_t i = 0; i < 4u; ++i) {
        runtime.WriteWord(window + 632u + 4u * i,
                          runtime.FindIcon(i, 0u, 0u));
        const auto hint_name = "CombatEndingLevelUpHint" + std::to_string(i);
        const auto memory = runtime.AllocateHint(kHintSize);
        std::uint32_t hint = 0u;
        if (memory != 0u) {
            try {
                hint = runtime.ConstructHint(memory, hint_name);
            } catch (...) {
                runtime.FreeHint(memory);
                throw;
            }
        }
        runtime.WriteWord(window + 684u + 4u * i, hint);
    }

    constexpr std::array<std::string_view, 5> rank_images{
        "tedeng", "yideng", "erdeng", "sandeng", "sideng"};
    for (std::uint32_t i = 0; i < rank_images.size(); ++i) {
        runtime.WriteWord(window + 664u + 4u * i,
                          runtime.FindImage("zi0", rank_images[i]));
    }

    (void)CombatEndingWindowPrizeEvents::Initialize(window, runtime);
    runtime.SetupElements(window);
    return window;
}

}  // namespace pal4
