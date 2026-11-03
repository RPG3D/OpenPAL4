#pragma once
#include <filesystem>
namespace pal4 {
class NativePlayerManagerSession;class NativeGameDatabase;class INativeSkillSystemServices;class NativeEffectSceneSession;
namespace test {void VerifyNativeCombatActorExit(const std::filesystem::path&,NativePlayerManagerSession&,
    const NativeGameDatabase&,INativeSkillSystemServices&,NativeEffectSceneSession&);}
}
