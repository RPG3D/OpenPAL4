#include "pal4/game_world_initializer.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 12> kSuffixes{
    ".uva",       ".bsp",       "_trans.dff",  "_trans.uva",
    "_water.dff", "_water.uva", "_clip.dff",   "_clip.uva",
    "_clipNA.dff", "_clipNA.uva", "_sky.dff", "_sky.uva"};
constexpr std::array<std::uint32_t, 12> kFreeOrder{
    1u, 0u, 2u, 3u, 6u, 7u, 8u, 9u, 4u, 5u, 10u, 11u};

}  // namespace

bool GameWorldInitializer::Initialize(
    GameWorldObjectState& state, const std::string_view base_directory,
    const std::string_view scene_name, const std::string_view variant_name,
    const std::uint32_t context, IGameWorldInitializeRuntime& runtime) {
    static_cast<void>(GameWorldCleanup::Cleanup(state, context, runtime));

    const auto base_path = std::string(base_directory) +
                           std::string(scene_name) + "\\" +
                           std::string(variant_name) + "\\";
    const auto asset_prefix = base_path + std::string(variant_name);
    state.words[2] = runtime.Allocate(static_cast<std::uint32_t>(scene_name.size() + 1u));
    state.words[3] = runtime.Allocate(static_cast<std::uint32_t>(variant_name.size() + 1u));
    state.words[1] = runtime.Allocate(static_cast<std::uint32_t>(base_path.size() + 1u));
    if (state.words[2] == 0u || state.words[3] == 0u || state.words[1] == 0u)
        return false;

    std::array<std::uint32_t, kSuffixes.size()> paths{};
    for (std::uint32_t index = 0u; index < paths.size(); ++index) {
        paths[index] = runtime.Allocate(static_cast<std::uint32_t>(
            asset_prefix.size() + kSuffixes[index].size() + 1u));
    }
    for (const auto required : {0u, 1u, 2u, 4u, 6u, 8u, 10u}) {
        if (paths[required] == 0u)
            return false;
    }

    runtime.WriteText(state.words[2], scene_name);
    runtime.WriteText(state.words[3], variant_name);
    runtime.WriteText(state.words[1], base_path);
    for (std::uint32_t index = 0u; index < paths.size(); ++index)
        runtime.WriteText(paths[index], asset_prefix +
                                            std::string(kSuffixes[index]));

    static_cast<void>(GameWorldMaterialParams::Load(state, runtime));
    runtime.ReallocateImage(state.words[1]);
    runtime.LoadDictionary(paths[0]);
    const auto world = runtime.LoadAtomic(paths[1], context);
    state.words[4] = world;
    if (world == 0u)
        return false;
    runtime.SetCurrentDictionary(state, 7u, world);
    runtime.SetWorldMaterialFlags(world, state.words[10]);
    runtime.WriteWord(world + 8u, runtime.ReadWord(world + 8u) | 0x45u);
    static_cast<void>(GameWorldLightGather::Gather(state, runtime));

    constexpr std::array<std::uint32_t, 5> kDffIndices{2u, 6u, 8u, 4u, 10u};
    constexpr std::array<std::uint32_t, 5> kUvaIndices{3u, 7u, 9u, 5u, 11u};
    constexpr std::array<std::uint32_t, 5> kObjectWords{13u, 18u, 23u, 28u, 33u};
    constexpr std::array<std::uint32_t, 5> kDictionaryWords{14u, 19u, 24u,
                                                              29u, 34u};
    constexpr std::array<std::uint32_t, 5> kFlagWords{17u, 22u, 27u, 32u, 41u};
    for (std::uint32_t index = 0u; index < kDffIndices.size(); ++index) {
        runtime.LoadDictionary(paths[kUvaIndices[index]]);
        const auto clump = runtime.LoadClump(paths[kDffIndices[index]], context);
        state.words[kObjectWords[index]] = clump;
        if (clump == 0u)
            continue;
        runtime.SetCurrentDictionary(state, kDictionaryWords[index], clump);
        runtime.SetClumpMaterialFlags(clump, state.words[kFlagWords[index]]);
        if (index == 4u)
            runtime.ProcessSkyAtomicGeometry(clump, state, 37u);
    }
    for (const auto index : kFreeOrder)
        static_cast<void>(runtime.Free(paths[index]));
    state.words[12] = 1u;
    return true;
}

}  // namespace pal4
