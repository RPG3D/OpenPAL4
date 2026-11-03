#include "pal4/game_data_static_paths.h"

#include <array>
#include <cstddef>

namespace pal4 {
namespace {

constexpr std::array<GameDataPathSpec, 13> kSpecs{{
    {0x66dfe0u, 0x66dff0u, 0x66e010u, 0x94d560u, "gamedata\\"},
    {0x66e020u, 0x66e030u, 0x66e050u, 0x94d554u, "PALWorld\\"},
    {0x66e060u, 0x66e070u, 0x66e090u, 0x94d548u, "PALActor\\"},
    {0x66e0a0u, 0x66e0b0u, 0x66e0d0u, 0x94d53cu, "palObject\\"},
    {0x66e0e0u, 0x66e0f0u, 0x66e110u, 0x94d530u, "PALWeapon\\"},
    {0x66e120u, 0x66e130u, 0x66e150u, 0x94d524u, "PALSound\\"},
    {0x66e160u, 0x66e170u, 0x66e190u, 0x94d518u, "gamedata\\Music\\"},
    {0x66e1a0u, 0x66e1b0u, 0x66e1d0u, 0x94d50cu,
     "gamedata\\EditData\\scene\\"},
    {0x66e1e0u, 0x66e1f0u, 0x66e210u, 0x94d500u,
     "gamedata\\EditData\\script\\"},
    {0x66e220u, 0x66e230u, 0x66e250u, 0x94d4f4u,
     "gamedata\\EffData\\"},
    {0x66e260u, 0x66e270u, 0x66e290u, 0x94d4e8u,
     "gamedata\\MatFX\\NorTexture\\"},
    {0x66e2a0u, 0x66e2b0u, 0x66e2d0u, 0x94d4dcu,
     "gamedata\\MatFX\\ReflectionTexture\\"},
    {0x66e2e0u, 0x66e2f0u, 0x66e310u, 0x94d4d0u,
     "gamedata\\MatFX\\DualTexture\\"},
}};

}  // namespace

const GameDataPathSpec& GameDataStaticPaths::Spec(const GameDataPath path) {
    return kSpecs.at(static_cast<std::size_t>(path));
}

std::uint32_t GameDataStaticPaths::Initialize(
    const GameDataPath path, IGameDataStaticPathsRuntime& runtime) {
    const auto& spec = Spec(path);
    return runtime.InitializeString(spec.global_address, spec.path);
}

int GameDataStaticPaths::Register(
    const GameDataPath path, IGameDataStaticPathsRuntime& runtime) {
    const auto& spec = Spec(path);
    (void)Initialize(path, runtime);
    return runtime.RegisterAtexit(spec.cleanup_entry);
}

void GameDataStaticPaths::Cleanup(
    const GameDataPath path, IGameDataStaticPathsRuntime& runtime) {
    runtime.CleanupString(Spec(path).global_address);
}

}  // namespace pal4
