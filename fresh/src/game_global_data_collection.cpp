#include "pal4/game_global_data_collection.h"

namespace pal4 {

std::uint8_t GameGlobalDataCollection::InsertKey(
    const std::int32_t key, IGameGlobalDataCollectionRuntime& runtime) {
    const auto tree = runtime.GlobalDataSubsystem() + 0x84u;
    const auto nil = runtime.NilNode();
    const auto header = runtime.ReadDword(tree + 4u);
    auto parent = header;
    auto child = runtime.ReadDword(header + 4u);
    std::uint8_t comparison = 1u;
    while (child != nil) {
        parent = child;
        comparison = key < static_cast<std::int32_t>(
                               runtime.ReadDword(child + 0xcu)) ? 1u : 0u;
        child = runtime.ReadDword(child + (comparison ? 0u : 8u));
    }
    if (runtime.ReadByte(tree + 8u) != 0u)
        return runtime.InsertNode(tree, child, parent, key);

    auto candidate = parent;
    if (comparison != 0u) {
        if (parent == runtime.ReadDword(header))
            return runtime.InsertNode(tree, child, parent, key);
        comparison = runtime.PreviousNode(candidate);
    }
    if (static_cast<std::int32_t>(runtime.ReadDword(candidate + 0xcu)) >= key)
        return comparison;
    return runtime.InsertNode(tree, child, parent, key);
}

std::uint32_t GameGlobalDataCollection::WordAt90(
    IGameGlobalDataCollectionRuntime& runtime) {
    return runtime.ReadDword(runtime.GlobalDataSubsystem() + 0x90u);
}

}  // namespace pal4
