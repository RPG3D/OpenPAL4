#include "pal4/combat_ready_target_scan.h"

namespace pal4 {
namespace {

class ScopedIterator final {
public:
    ScopedIterator(ICombatReadyTargetRuntime& runtime,
                   const std::uint32_t iterator)
        : runtime_(runtime), iterator_(iterator) {}
    ~ScopedIterator() {
        if (iterator_ != 0u) runtime_.DestroyIterator(iterator_, 1u);
    }
    ScopedIterator(const ScopedIterator&) = delete;
    ScopedIterator& operator=(const ScopedIterator&) = delete;

    [[nodiscard]] std::uint32_t get() const { return iterator_; }

private:
    ICombatReadyTargetRuntime& runtime_;
    std::uint32_t iterator_;
};

}  // namespace

std::uint32_t CombatReadyTargetScan::FirstReadyOther(
    ICombatReadyTargetRuntime& runtime, const std::uint32_t actor,
    const std::uint32_t vtable_offset,
    const std::uint32_t excluded) {
    ScopedIterator iterator(runtime, runtime.OpenIterator(actor,
                                                          vtable_offset));
    if (iterator.get() == 0u) return 0u;
    while (!runtime.IteratorAtEnd(iterator.get())) {
        const auto candidate = runtime.IteratorCurrent(iterator.get());
        if (runtime.IsActorReady(candidate) && candidate != excluded)
            return candidate;
        runtime.IteratorNext(iterator.get());
    }
    return 0u;
}

}  // namespace pal4
