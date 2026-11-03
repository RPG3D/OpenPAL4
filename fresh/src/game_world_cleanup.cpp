#include "pal4/game_world_cleanup.h"
#include "pal4/game_world_resource_name.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

std::int32_t GameWorldCleanup::Cleanup(
    GameWorldObjectState& state, const std::uint32_t context,
    IGameWorldCleanupRuntime& runtime) {
    for (const auto offset : {45u, 46u, 47u, 48u})
        state.words[offset] = 0u;
    for (const auto offset : {42u, 43u}) {
        if (state.words[offset] != 0u) {
            runtime.FreeExternal(state.words[offset]);
            state.words[offset] = 0u;
        }
    }
    for (const auto offset : {7u, 14u, 19u, 24u, 29u, 34u})
        runtime.CleanupDictionary(state, offset);

    if (state.words[11] != 0u) {
        if (state.words[4] != 0u)
            runtime.RemoveCamera(state.words[4], state.words[11]);
        state.words[11] = 0u;
    }
    for (const auto offset : {5u, 6u}) {
        if (state.words[offset] != 0u) {
            runtime.DestroyLight(state.words[offset]);
            state.words[offset] = 0u;
        }
    }
    for (const auto offset : {13u, 18u, 23u, 28u, 33u}) {
        const auto clump = state.words[offset];
        if (clump == 0u)
            continue;
        const auto world = runtime.GetClumpWorld(clump);
        if (world != 0u)
            runtime.RemoveClump(world, clump);
        runtime.DestroyClump(clump);
        state.words[offset] = 0u;
    }

    if (state.words[4] != 0u) {
        runtime.DestroyWorld(state.words[4]);
        state.words[4] = 0u;
        const auto prefix = runtime.ReadText(state.words[1]) +
                            runtime.ReadText(state.words[3]);
        constexpr std::array<std::string_view, 6> kSuffixes{
            ".bsp", "_trans.dff", "_water.dff", "_clip.dff",
            "_clipNA.dff", "_sky.dff"};
        for (const auto suffix : kSuffixes) {
            const auto path = prefix + std::string(suffix);
            const auto buffer = runtime.Allocate(
                static_cast<std::uint32_t>(path.size() + 1u));
            runtime.WriteText(buffer, path);
            runtime.RemoveTextureByName(
                context, GameWorldResourceName::Build(path));
            static_cast<void>(runtime.Free(buffer));
        }
    }

    for (const auto offset : {1u, 2u}) {
        if (state.words[offset] != 0u) {
            static_cast<void>(runtime.Free(state.words[offset]));
            state.words[offset] = 0u;
        }
    }
    std::int32_t result = static_cast<std::int32_t>(state.words[3]);
    if (state.words[3] != 0u) {
        result = runtime.Free(state.words[3]);
        state.words[3] = 0u;
    }
    state.words[12] = 0u;
    return result;
}

}  // namespace pal4
