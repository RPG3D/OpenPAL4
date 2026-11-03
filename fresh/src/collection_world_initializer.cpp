#include "pal4/collection_world_initializer.h"

namespace pal4 {

int CollectionWorldInitializer::AddElementWithDetails(
    CollectionState& state, std::uint32_t object,
    std::string_view name, std::uint32_t camera,
    ICollectionWorldRuntime& runtime) {
    runtime.PrepareGeometry(state, 0xdcu);
    if (runtime.CompareName(state.world, name) == 0) return 0;

    if (runtime.AllocateName(state.world, name.size(), 1u)) {
        runtime.CopyName(state.world, name);
        state.world.field_dc = static_cast<std::uint32_t>(name.size());
    }
    runtime.ProcessScripts();
    const auto game = runtime.GameInstance();
    runtime.InitializeWorld(state, object, "UIWorld", name, game + 0x1acu);
    runtime.SetCamera(state, camera);
    runtime.AddLight(state, {0x3f800000u, 0x3f800000u,
                             0x3f800000u, 0x3f800000u});
    const auto clump = runtime.WorldClump(state);
    if (clump != 0u) {
        runtime.AddClump(state, clump);
        runtime.ProcessClumpAtomics(state, clump, 0xdcu);
    }
    return runtime.CleanupScripts();
}

}  // namespace pal4
