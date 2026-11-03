#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
struct NativeCombatSystemImage;
class NativeEventDocumentRuntime;class NativeCombatTaskQueue;class INativeSkillSystemServices;
// Required actual owner operations. Host construction can occur within its
// dependency phase; no operation may silently acknowledge an unbound service.
// Call synchronously; all captures/owners must outlive RunNativeCombatStart.
struct NativeCombatStartOperations {
    std::function<void()> initialize_cold,reset_collections,clear_rosters,reset_console;
    std::function<void(std::string_view,std::string_view)> load_world;
    std::function<void()> initialize_game_manager,show_ui,create_players;
    std::function<void(std::uint32_t,std::int32_t)> create_monsters;
    std::function<bool()> has_players;
    std::function<bool(std::int32_t)> commit_ready;
};
// Native XML/image/task/skill boundary for the actual shared5756D0 sequence.
// Does not register a state dispatcher, enter Scene/Combat or change mode choices.
bool RunNativeCombatStart(NativeCombatSystemImage&,NativeEventDocumentRuntime&,
    NativeCombatTaskQueue&,INativeSkillSystemServices&,std::uint32_t config,
    std::uint32_t mode,const NativeCombatStartOperations&);
}
