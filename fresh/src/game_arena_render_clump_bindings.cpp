#include "pal4/game_arena_render_clump_bindings.h"
#include <initializer_list>
namespace pal4 {
void BindGameArenaRenderClumps(std::uint32_t arena, IGameArenaRenderClumpRuntime& runtime) {
    runtime.EnableDistanceOptimization();
    for (const auto offset : {52u,72u,92u}) {
        const auto clump=runtime.ReadClump(arena+offset);
        if (!clump) continue;
        runtime.AddClump(arena,clump);
        runtime.BindAlphaQueue(clump,arena+284u);
        if (offset!=92u) runtime.BindDistanceCallbacks(clump);
    }
}
}
