#include "pal4/quick_cast_magic_sequence_task.h"

namespace pal4 {
namespace {

class ScopedTargetIterator final {
public:
    ScopedTargetIterator(IQuickCastMagicCompletionRuntime& runtime,
                         std::uint32_t iterator)
        : runtime_(runtime), iterator_(iterator) {}
    ~ScopedTargetIterator() {
        if (iterator_ != 0u) runtime_.DestroyIterator(iterator_, 1u);
    }
    ScopedTargetIterator(const ScopedTargetIterator&) = delete;
    ScopedTargetIterator& operator=(const ScopedTargetIterator&) = delete;
    [[nodiscard]] std::uint32_t get() const { return iterator_; }

private:
    IQuickCastMagicCompletionRuntime& runtime_;
    std::uint32_t iterator_;
};

}  // namespace

std::uint32_t QuickCastMagicSequenceTask::Complete(
    const std::uint32_t task, IQuickCastMagicCompletionRuntime& runtime) {
    if (!runtime.CombatActive()) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.SourceStateThree(source)) {
        const auto target = runtime.ReadWord(task + 48u);
        const auto current_source = runtime.ReadWord(task + 44u);
        const auto dead = runtime.ProcessDead(current_source, target);
        const auto end = runtime.EndAction(runtime.ReadWord(task + 44u));
        if (dead == 0u) return 0u;
        runtime.WriteWord(dead + 36u, end);
        return dead;
    }

    const auto target = runtime.ReadWord(task + 48u);
    ScopedTargetIterator iterator(runtime, runtime.OpenTargetIterator(target));
    if (iterator.get() != 0u) {
        while (!runtime.IteratorAtEnd(iterator.get())) {
            const auto actor = runtime.IteratorCurrent(iterator.get());
            if (!runtime.IsActorReady(actor))
                runtime.NotifyUnreadyActor(actor,
                                           runtime.ReadWord(task + 44u));
            runtime.IteratorNext(iterator.get());
        }
    }
    return runtime.EndAction(runtime.ReadWord(task + 44u));
}

}  // namespace pal4
