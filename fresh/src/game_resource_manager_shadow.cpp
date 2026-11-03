#include "pal4/game_resource_manager.h"

namespace pal4 {

bool GameResourceManager::LoadShadowClump(
    const std::uint32_t texture_context, IGameResourceManagerRuntime& runtime) {
    if (words[9] == 0) return false;
    (void)CleanupShadowClump(texture_context, runtime);
    const auto path = runtime.ReadString(words[2]) +
                      runtime.ReadString(words[3]) + "_shadow.dff";
    const auto temporary = runtime.Allocate(
        static_cast<std::uint32_t>(path.size() + 1));
    runtime.StoreString(temporary, path);
    words[12] = runtime.LoadWorldClump(path, 0, texture_context);
    if (temporary != 0) (void)runtime.Free(temporary);
    if (words[12] == 0) return false;
    return InitializeShadowLight(runtime);
}

bool GameResourceManager::InitializeShadowLight(IGameResourceManagerRuntime& runtime) {
    if (words[12] == 0 || words[13] != 0) return false;
    words[13] = runtime.CreateLight(1);
    if (words[13] == 0) return false;
    const auto frame = runtime.CreateFrame();
    if (frame == 0) {
        runtime.DestroyLight(words[13]);
        words[13] = 0;
        return false;
    }
    runtime.FrameRotate(frame, 0x8c6ff0u, 86.900002f, 0);
    runtime.FrameRotate(frame, 0x8c6ffcu, 10.0f, 1);
    runtime.FrameRotate(frame, 0x8c7008u, 0.0f, 1);
    runtime.SetLightFrame(words[13], frame);
    runtime.AddFrameChild(runtime.ClumpFrame(words[12]),
                          runtime.LightFrame(words[13]));
    return true;
}

bool GameResourceManager::CleanupShadowClump(
    const std::uint32_t texture_context, IGameResourceManagerRuntime& runtime) {
    (void)CleanupCamera(runtime);
    if (words[13] != 0) {
        const auto frame = runtime.LightFrame(words[13]);
        runtime.SetLightFrame(words[13], 0);
        if (frame != 0) runtime.DestroyFrame(frame);
        runtime.DestroyLight(words[13]);
        words[13] = 0;
    }
    if (words[12] != 0) {
        const auto clump = words[12];
        const auto world = runtime.ClumpWorld(clump);
        if (world != 0) runtime.RemoveClump(world, clump);
        runtime.DestroyClump(clump);
        words[12] = 0;
        if (words[2] != 0 && words[3] != 0) {
            const auto path = runtime.ReadString(words[2]) +
                              runtime.ReadString(words[3]) + "_shadow.dff";
            const auto temporary = runtime.Allocate(
                static_cast<std::uint32_t>(path.size() + 1));
            runtime.StoreString(temporary, path);
            runtime.RemoveTextureByName(texture_context, path);
            if (temporary != 0) (void)runtime.Free(temporary);
        }
    }
    words[11] = 0;
    return true;
}

}  // namespace pal4
