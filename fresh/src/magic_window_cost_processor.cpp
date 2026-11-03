#include "pal4/magic_window_cost_processor.h"

#include <bit>

namespace pal4 {

void MagicWindowCostProcessor::Apply(const MagicDbStuntRecord& record,
                                     IMagicWindowCostRuntime& runtime) {
    if (record.counter_cost > 0) {
        const auto delta = 0u - static_cast<std::uint32_t>(record.counter_cost);
        runtime.AdjustCounter(std::bit_cast<std::int32_t>(delta));
    }

    for (std::size_t index = 0; index < record.item_ids.size(); ++index) {
        if (static_cast<std::int32_t>(record.item_ids[index]) > 0)
            runtime.RemoveItem(record.item_ids[index], record.item_counts[index]);
    }
}

}  // namespace pal4
