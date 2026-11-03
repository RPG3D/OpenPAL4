#include "pal4/game_resource_manager.h"
#include <bit>
namespace pal4 {
bool GameResourceManager::UpdateAndRenderClump(
    const float elapsed, IGameResourceFrameRuntime& runtime,
    IRwDictSchemaUpdateRuntime& schema_runtime) {
    if (words[9] == 0) return false;
    if (words[6] != 0 && words[7] == 0) return false;
    RwDictSchema schema;
    schema.dict = words[37];
    schema.list = words[38];
    schema.time_scale = std::bit_cast<float>(words[39]);
    (void)schema.Update(elapsed, schema_runtime);
    if (words[11] != 0 && words[13] != 0 && words[12] != 0) {
        runtime.RenderCameraScene(words.data() + 14, elapsed, words.data());
    }
    return true;
}

} // namespace pal4
