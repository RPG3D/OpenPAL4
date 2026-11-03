#include "pal4/game_resource_manager.h"

namespace pal4 {

std::uint32_t GameResourceManager::LoadExtSizeScl(
    IGameResourceManagerRuntime& runtime) {
    if (words[5] == 0) return words[5];
    if (words[4] == 0) return words[4];

    const auto path = runtime.ReadString(words[2]) + "extSize.scl";
    const auto temporary = runtime.Allocate(
        static_cast<std::uint32_t>(path.size() + 1));
    runtime.StoreString(temporary, path);
    const auto stream = runtime.OpenDataStream(2, 1, path);
    const auto free_result = runtime.Free(temporary);
    if (stream == 0) return free_result;

    runtime.ReadStreamWords(stream, &words[49], 3);
    return runtime.CloseDataStream(stream);
}


}  // namespace pal4
