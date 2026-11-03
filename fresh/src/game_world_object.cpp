#include "pal4/game_world_object.h"

#include <array>
#include <cstdint>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 6> kDictionaryOffsets{7u, 14u, 19u,
                                                            24u, 29u, 34u};
constexpr std::array<std::uint32_t, 19> kClearedWords{
    1u, 2u, 3u, 4u, 5u, 6u, 11u, 12u, 13u, 18u, 23u, 28u, 33u,
    45u, 46u, 47u, 48u, 42u, 43u};
constexpr std::array<std::uint32_t, 6> kEnabledWords{10u, 17u, 22u,
                                                      27u, 32u, 41u};

}  // namespace

GameWorldObjectState& GameWorldObject::Construct(
    GameWorldObjectState& state, IGameWorldDictionaryRuntime& runtime) {
    for (const auto offset : kDictionaryOffsets)
        runtime.InitializeDictionary(state, offset);
    for (const auto offset : kClearedWords)
        state.words[offset] = 0u;
    state.words[0] = kVtableAddress;
    state.words[44] = kDefaultParameterBits;
    for (const auto offset : kEnabledWords)
        state.words[offset] = 1u;
    return state;
}

void GameWorldObject::Destruct(GameWorldObjectState& state,
                                IGameWorldDictionaryRuntime& runtime) {
    state.words[0] = kVtableAddress;
    for (auto it = kDictionaryOffsets.rbegin(); it != kDictionaryOffsets.rend();
         ++it)
        runtime.CleanupDictionary(state, *it);
}

}  // namespace pal4
