#include "pal4/game_resource_manager.h"

#include <array>

namespace pal4 {
namespace {

std::uint32_t ReadEffectName(std::uint32_t stream,
                             IGameResourceManagerRuntime& runtime) {
    std::uint32_t length = 0;
    runtime.ReadStreamWord(stream, &length);
    if (static_cast<std::int32_t>(length) <= 0) return 0;
    const auto address = runtime.Allocate(length);
    runtime.ReadStreamBuffer(stream, address, length);
    runtime.TerminateBuffer(address, length - 1);
    return address;
}

}  // namespace

void GameResourceManager::LoadEffectData(IGameResourceManagerRuntime& runtime) {
    if (words[5] == 0 || words[4] == 0) return;
    const auto path = runtime.ReadString(words[2]) +
                      runtime.ReadString(words[3]) + "_" +
                      runtime.ReadString(words[4]) + ".eff";
    const auto temporary = runtime.Allocate(
        static_cast<std::uint32_t>(path.size() + 1));
    runtime.StoreString(temporary, path);
    const auto stream = runtime.OpenDataStream(2, 1, path);
    (void)runtime.Free(temporary);
    if (stream == 0) return;

    constexpr std::array<std::size_t, 8> fields{
        81, 89, 77, 76, 78, 82, 83, 84};
    for (const auto index : fields) {
        runtime.ReadStreamWord(stream, &words[index]);
    }
    const auto first = ReadEffectName(stream, runtime);
    const auto second = ReadEffectName(stream, runtime);
    const auto third = ReadEffectName(stream, runtime);
    (void)runtime.CloseDataStream(stream);
    runtime.AddAndProcessScript8();

    if (words[81] != 0 && third != 0) {
        (void)LoadTexture3(words[89], runtime.ReadString(third),
                           5, 6, runtime);
    }
    if (words[77] != 0 && first != 0 && second != 0) {
        (void)LoadTextures(runtime.ReadString(first), runtime.ReadString(second),
                           0, words[84], 1, words[82], 0x3f800000u, runtime);
    }
    if (words[76] != 0 && first != 0) {
        (void)LoadTexture(runtime.ReadString(first), 0x3f800000u, runtime);
    }
    if (words[78] != 0 && second != 0) {
        (void)LoadTexture2(runtime.ReadString(second), words[83], 0, runtime);
    }
    runtime.CleanupAllScripts();
    if (first != 0) (void)runtime.Free(first);
    if (second != 0) (void)runtime.Free(second);
    if (third != 0) (void)runtime.Free(third);
}


}  // namespace pal4
