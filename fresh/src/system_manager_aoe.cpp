#include "pal4/system_manager_aoe.h"

namespace pal4 {

std::uint32_t SystemManagerAoE::Distribute(
    const std::uint32_t manager, const std::uint32_t group_argument,
    ISystemManagerAoERuntime& runtime) {
    float total_rate = 0.0f;
    for (auto slot = runtime.ReadWord(manager + 112u);
         slot != runtime.ReadWord(manager + 116u); slot += 4u) {
        total_rate += runtime.ObjectRate(runtime.ReadWord(slot));
    }

    std::vector<std::uint32_t> ordered;
    for (auto slot = runtime.ReadWord(manager + 112u);
         slot != runtime.ReadWord(manager + 116u); slot += 4u) {
        const auto object = runtime.ReadWord(slot);
        auto position = ordered.begin();
        for (; position != ordered.end(); ++position) {
            const auto candidate_rate = runtime.ObjectRate(object);
            const auto existing_rate = runtime.ObjectRate(*position);
            if (existing_rate < candidate_rate) break;
        }
        ordered.insert(position, object);
    }

    const auto first = runtime.GameDbFloat(24u);
    const auto second = runtime.GameDbFloat(25u);
    const auto lower = first > second ? second : first;
    const auto upper = first > second ? first : second;
    const auto base_random = runtime.RandomNumber() % 1000u;
    const auto base = static_cast<float>(base_random) * 0.001f *
                      (upper - lower) + lower;
    auto current = base;
    if (!ordered.empty()) {
        const auto inverse_total = 1.0f / total_rate;
        for (const auto object : ordered) {
            const auto proposed = current -
                                  runtime.ObjectRate(object) * inverse_total * base;
            const auto interval_low = proposed > current ? current : proposed;
            const auto interval_high = proposed > current ? proposed : current;
            const auto draw = runtime.RandomNumber() % 1000u;
            current = static_cast<float>(draw) * 0.001f *
                      (interval_high - interval_low) + interval_low;
            runtime.SetObjectProperty(object, current * 64.0f);
        }
    }

    const auto group = runtime.ReadWord(group_argument);
    if (group == 1u || group == 2u) {
        for (auto slot = runtime.ReadWord(manager + 112u);
             slot != runtime.ReadWord(manager + 116u); slot += 4u) {
            const auto object = runtime.ReadWord(slot);
            if (runtime.ObjectGroup(object) == group)
                runtime.SetObjectProperty(object, 64.0f);
        }
    }
    runtime.FinalizeManager(manager);
    return runtime.ReleaseOrdered(ordered);
}

std::uint32_t SystemManagerAoE::InvokeManager(
    const std::uint32_t manager, ISystemManagerAoERuntime& runtime) {
    return runtime.InvokeManagerSlot4(manager);
}

}  // namespace pal4
