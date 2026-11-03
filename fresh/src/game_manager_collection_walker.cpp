#include "pal4/game_manager_collection_walker.h"

namespace pal4 {

std::uint32_t GameManagerCollectionRuntimeStub::ReadDword(
    const std::uint32_t address) {
    return address;
}
void GameManagerCollectionRuntimeStub::Invoke(
    std::uint32_t /*object*/, std::uint32_t /*slot_offset*/) {}

std::uint32_t GameManagerCollectionWalker::NotifySlot40(
    const ScreenEffectDispatcherState& state,
    IGameManagerCollectionRuntime& runtime) {
    return Walk(state, 0x28u, runtime);
}

std::uint32_t GameManagerCollectionWalker::NotifySlot44(
    const ScreenEffectDispatcherState& state,
    IGameManagerCollectionRuntime& runtime) {
    return Walk(state, 0x2cu, runtime);
}

std::uint32_t GameManagerCollectionWalker::Walk(
    const ScreenEffectDispatcherState& state,
    const std::uint32_t slot_offset,
    IGameManagerCollectionRuntime& runtime) {
    const auto sentinel = state.tree_sentinel;
    auto result = sentinel;
    auto node = runtime.ReadDword(sentinel);
    while (node != sentinel) {
        runtime.Invoke(runtime.ReadDword(node + 16u), slot_offset);
        result = runtime.ReadDword(node + 8u);
        if (result != kNilNode) {
            node = result;
            result = runtime.ReadDword(node);
            while (result != kNilNode) {
                node = result;
                result = runtime.ReadDword(node);
            }
        } else {
            result = runtime.ReadDword(node + 4u);
            while (node == runtime.ReadDword(result + 8u)) {
                node = result;
                result = runtime.ReadDword(result + 4u);
            }
            if (runtime.ReadDword(node + 8u) != result) {
                node = result;
            }
        }
    }
    return result;
}

}  // namespace pal4
