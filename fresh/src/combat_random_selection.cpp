#include "pal4/combat_random_selection.h"

#include <bit>
#include <stdexcept>

namespace pal4 {
namespace {

class CollectionRelease final {
public:
    CollectionRelease(const std::vector<std::int32_t>& values,
                      ICombatRandomSelectionRuntime& runtime)
        : values_(values), runtime_(runtime) {}
    ~CollectionRelease() {
        if (!released_)
            static_cast<void>(runtime_.ReleaseCollection(values_));
    }
    std::int32_t Release() {
        released_ = true;
        return runtime_.ReleaseCollection(values_);
    }

private:
    const std::vector<std::int32_t>& values_;
    ICombatRandomSelectionRuntime& runtime_;
    bool released_ = false;
};

std::int32_t Select(std::uint32_t action, std::uint32_t context,
                    std::uint32_t target, bool actor_event,
                    ICombatRandomSelectionRuntime& runtime) {
    const auto roll = static_cast<double>(runtime.RandomNumber() % 1000u) *
                      static_cast<double>(0.001f);
    const auto threshold = std::bit_cast<float>(
        runtime.ReadWord(action + 20u));
    if (roll > threshold) return 0;

    std::vector<std::int32_t> values;
    CollectionRelease release(values, runtime);
    for (std::uint32_t i = 0u; i < 8u; ++i) {
        const auto value = runtime.FloatToInt(std::bit_cast<float>(
            runtime.ReadWord(action + 24u + i * 4u)));
        if (value != 0) values.push_back(value);
    }
    const auto random = runtime.RandomNumber();
    if (values.empty())
        throw std::domain_error("combat random selection has no candidates");
    const auto selected = values[random % values.size()];
    if (actor_event)
        runtime.ApplySelectedActorEvent(target, selected, context);
    else
        runtime.ApplySelected(target, selected, context);
    return release.Release();
}

}  // namespace

std::int32_t CombatRandomSelection::Process(
    std::uint32_t action, std::uint32_t context,
    std::uint32_t target, ICombatRandomSelectionRuntime& runtime) {
    return Select(action, context, target, false, runtime);
}

std::int32_t CombatRandomSelection::ProcessActorEvent(
    std::uint32_t action, std::uint32_t target,
    std::uint32_t context, ICombatRandomSelectionRuntime& runtime) {
    return Select(action, context, target, true, runtime);
}

}  // namespace pal4
