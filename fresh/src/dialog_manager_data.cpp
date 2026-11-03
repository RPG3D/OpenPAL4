#include "pal4/dialog_manager_data.h"

#include <bit>

namespace pal4 {

double DialogManagerData::NormalizedStat(const DialogManagerState& state) {
    constexpr auto factor = std::bit_cast<float>(0x3c053408u);
    return static_cast<double>(state.Word(128)) * factor;
}

std::uint8_t DialogManagerData::InsertEvent(
    DialogManagerState& state, const std::uint32_t key,
    IDialogManagerEventRuntime& runtime) {
    const auto nil = runtime.NilNode();
    const auto head = state.Word(120);
    auto parent = head;
    auto leaf = runtime.NodeWord(head, 4);
    bool less = true;
    while (leaf != nil) {
        parent = leaf;
        less = key < runtime.NodeWord(leaf, 12);
        leaf = runtime.NodeWord(leaf, less ? 0 : 8);
    }
    if (state.Byte(124) != 0)
        return runtime.Insert(state, leaf, parent, key);

    std::uint8_t result = less ? 1 : 0;
    auto candidate = parent;
    if (less) {
        if (parent == runtime.NodeWord(head, 0))
            return runtime.Insert(state, leaf, parent, key);
        result = runtime.FindPredecessor(candidate);
    }
    if (runtime.NodeWord(candidate, 12) >= key) return result;
    return runtime.Insert(state, leaf, parent, key);
}

std::uint32_t DialogManagerData::MergeStoryMax(
    const DialogManagerState& state,
    IDialogManagerStoryMergeRuntime& runtime) {
    const auto list = runtime.GameInstance() + 0x154;
    for (std::uint32_t index = 0;; ++index) {
        const auto begin = runtime.GameStoryBegin(list);
        if (begin == 0) break;
        const auto count = (runtime.GameStoryEnd(list) - begin) / 4;
        if (index >= count) break;
        const auto object = runtime.GameStoryObject(begin + index * 4);
        const auto value = runtime.ObjectStat(object);
        const auto address = state.Word(184) + index * 4;
        if (value > runtime.StoryStat(address))
            runtime.SetStoryStat(address, value);
    }
    return list;
}

double DialogManagerData::CalculateStatValue(
    const DialogManagerState& state, IDialogManagerStatRuntime& runtime) {
    static_cast<void>(MergeStoryMax(state, runtime));
    const auto candidates = runtime.SortedCandidatesFromGameDb();
    const auto first = state.Word(184);
    const auto end = state.Word(188);
    const auto count = first == 0 || end < first ? 0u : (end - first) / 4;
    std::vector<const DialogStatCandidate*> selected(count, nullptr);
    for (const auto& candidate : candidates) {
        if (candidate.story_index < 0 ||
            static_cast<std::uint32_t>(candidate.story_index) >= count)
            continue;
        const auto index = static_cast<std::size_t>(candidate.story_index);
        if (candidate.threshold >
            runtime.StoryStat(first + static_cast<std::uint32_t>(index) * 4))
            continue;
        if (selected[index] == nullptr ||
            candidate.threshold > selected[index]->threshold)
            selected[index] = &candidate;
    }
    double total = 0.0;
    for (const auto* candidate : selected)
        if (candidate != nullptr) total += candidate->score;
    return static_cast<float>(total);
}

}  // namespace pal4
