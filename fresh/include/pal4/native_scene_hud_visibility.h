#pragma once

#include <cstdint>

namespace pal4 {

struct PalActorControlState;
struct PlayerManagerObjectState;
class IScriptPlayerActionsRuntime;
class NativePalGameIVTextureStorage;
class NativeGameTimerQueue;
class NativeEventHandlerArray;

// Borrowed services for a single complete HUD visibility transition.
struct NativeSceneHudVisibilityServices {
    PalActorControlState& control;
    PlayerManagerObjectState& players;
    IScriptPlayerActionsRuntime& player_actions;
    NativePalGameIVTextureStorage& game;
    NativeGameTimerQueue& timers;
    NativeEventHandlerArray& active;
    std::uint32_t frame_manager;
};

}  // namespace pal4
