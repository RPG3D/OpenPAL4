#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeCombatTranslator;
class NativeCombatConsole;
class NativeMatFxScriptSession;
class NativeCameraManagerOwner;
class INativeSkillSystemServices;

// Executes shared5754C0 against fresh native resource owners. All dependencies
// are borrowed. Call before world pipeline/actor consumers borrow these resources.
// Failure releases only resources loaded here and leaves cold517=0; camera
// defaults already applied by the original sequence are not rolled back.
void InitializeNativeCombatCold(NativeCombatSystemImage&, NativeCombatTranslator&,
    NativeCombatConsole&, NativeMatFxScriptSession&, NativeCameraManagerOwner&,
    INativeSkillSystemServices&, const std::filesystem::path& game_root,
    std::function<void(bool beginning)> log);
}
