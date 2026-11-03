#include "pal4/game_arena_collision_tree.h"

namespace pal4 {

void GameArenaCollisionTree::BuildFromClump(
    std::uint32_t arena, std::uint32_t clump,
    IGameArenaCollisionRuntime& runtime) {
    if (clump == 0u) return;

    const auto cached = GameArenaObjectList::FindByName(arena, runtime);
    runtime.WriteWord(arena + 0x194u, cached);
    if (cached != 0u) return;

    const auto memory = runtime.AllocateTree(0x40u);
    const auto tree = memory == 0u ? 0u : runtime.InitializeTree(memory);
    runtime.WriteWord(arena + 0x194u, tree);
    runtime.ForAllAtomics(clump, arena, &GameArenaCollisionTree::AddTriangles);
    runtime.SetupRenderStates(runtime.ReadWord(arena + 0x194u));
    runtime.SetDebugRenderer(runtime.ReadWord(arena + 0x194u));
}

std::uint32_t GameArenaCollisionTree::AddTriangles(
    std::uint32_t atomic, std::uint32_t arena,
    IGameArenaCollisionRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic + 24u);
    if (geometry == 0u) return atomic;

    const auto triangleCount =
        static_cast<std::int32_t>(runtime.ReadWord(geometry + 16u));
    const auto indices = runtime.ReadWord(geometry + 44u);
    const auto morph = runtime.ReadWord(geometry + 92u);
    const auto vertices = runtime.ReadWord(morph + 20u);
    if (triangleCount <= 0) return atomic;

    for (std::int32_t index = 0; index < triangleCount; ++index) {
        CollisionRawVector transformed[3];
        CollisionVector positions[3];
        for (std::uint32_t vertex = 0; vertex < 3u; ++vertex) {
            const auto frame = runtime.ReadWord(atomic + 4u);
            const auto transform = runtime.GetObjectTransform(frame);
            const auto vertexIndex = runtime.ReadHalfWord(
                indices + static_cast<std::uint32_t>(index) * 8u + vertex * 2u);
            transformed[vertex] = runtime.TransformVertex(
                vertices + static_cast<std::uint32_t>(vertexIndex) * 12u,
                transform);
        }
        for (std::uint32_t vertex = 0; vertex < 3u; ++vertex)
            positions[vertex] = runtime.ConvertCoordinates(transformed[vertex]);
        if (runtime.VectorsDiffer(positions[0], positions[1]) &&
            runtime.VectorsDiffer(positions[1], positions[2]) &&
            runtime.VectorsDiffer(positions[2], positions[0])) {
            runtime.AddTriangle(runtime.ReadWord(arena + 0x194u),
                                positions[0], positions[1], positions[2]);
        }
    }
    return atomic;
}

}  // namespace pal4
