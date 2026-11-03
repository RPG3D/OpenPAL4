#include "pal4/pal_game_iv_state_registry.h"

#include <new>

namespace pal4 {
namespace {

constexpr std::uint32_t kFirstEntryOffset = 0x188u;

}  // namespace

bool PalGameIVStateRegistry::AddEntry(
    std::uint32_t game, std::uint32_t entry,
    IPalGameIVStateRegistryRuntime& runtime) {
    if (ContainsEntry(game, entry, runtime)) return false;
    const auto node = runtime.Allocate(8u);
    if (node == 0u) throw std::bad_alloc();
    runtime.WriteWord(node, entry);
    runtime.WriteWord(node + 4u, 0u);
    auto cursor = runtime.ReadWord(game + kFirstEntryOffset);
    if (cursor == 0u) {
        runtime.WriteWord(game + kFirstEntryOffset, node);
    } else {
        while (runtime.ReadWord(cursor + 4u) != 0u)
            cursor = runtime.ReadWord(cursor + 4u);
        runtime.WriteWord(cursor + 4u, node);
    }
    return true;
}

bool PalGameIVStateRegistry::ContainsEntry(
    std::uint32_t game, std::uint32_t entry,
    IPalGameIVStateRegistryRuntime& runtime) {
    auto cursor = runtime.ReadWord(game + kFirstEntryOffset);
    while (cursor != 0u) {
        if (runtime.StateId(runtime.ReadWord(cursor)) ==
            runtime.StateId(entry))
            return true;
        cursor = runtime.ReadWord(cursor + 4u);
    }
    return false;
}

std::uint32_t PalGameIVStateRegistry::FindEntryById(
    std::uint32_t game, std::uint32_t id,
    IPalGameIVStateRegistryRuntime& runtime) {
    auto cursor = runtime.ReadWord(game + kFirstEntryOffset);
    while (cursor != 0u) {
        const auto entry = runtime.ReadWord(cursor);
        if (runtime.StateId(entry) == id) return entry;
        cursor = runtime.ReadWord(cursor + 4u);
    }
    return 0u;
}

std::uint32_t PalGameIVStateRegistry::Cleanup(
    std::uint32_t game, IPalGameIVStateRegistryRuntime& runtime) {
    auto result = runtime.EmptyCleanupResult();
    auto node = runtime.ReadWord(game + kFirstEntryOffset);
    while (node != 0u) {
        runtime.WriteWord(game + kFirstEntryOffset,
                          runtime.ReadWord(node + 4u));
        const auto entry = runtime.ReadWord(node);
        if (entry != 0u) runtime.DeleteState(entry, 1u);
        result = runtime.Free(node);
        node = runtime.ReadWord(game + kFirstEntryOffset);
    }
    return result;
}

}  // namespace pal4
