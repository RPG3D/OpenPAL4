#pragma once
#include <cstdint>
#include <filesystem>
#include <string_view>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatWorldResources;class NativeCombatWorldNames;
class NativeCameraManagerOwner;class INativeSkillSystemServices;
// Original53F3B0, before GOM initialization and UI/actor creation. All model
// borrowers and scene subscriptions must have retired before a replacement.
// Reuses native packed scene/pipeline loading and shared queue/cache/camera order.
void LoadNativeCombatStartWorld(NativeCombatSystemImage&,NativeCombatWorldResources&,
    NativeCombatWorldNames&,NativeCameraManagerOwner&,INativeSkillSystemServices&,
    const std::filesystem::path& game_root,std::uint32_t effect,
    std::string_view region,std::string_view scene);
}
