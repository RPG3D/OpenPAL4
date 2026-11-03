#pragma once
#include "pal4/native_game_script_callbacks.h"
#include <cstdint>
#include <functional>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;
class NativeGameArenaManagerOwner;
class NativeArenaNpcSceneEntry;
class NativeNewGameScriptSession;
class NativePkFileManagerRuntime;
class GameMathTables;
class IMonsterProximityRuntime;
class IMonsterInteractionRuntime;

struct NativeMonsterInteractionServices {
    NativeBaseObjectMemory& memory;
    NativeMonsterManagerOwner& monsters;
    NativeGameArenaManagerOwner& arena;
    NativeArenaNpcSceneEntry& scene_entries;
    NativeNewGameScriptSession& scripts;
    NativePkFileManagerRuntime& packages;
    const GameMathTables& math;
    IMonsterProximityRuntime& proximity;
    std::uint32_t actor_control;
    NativeGameScriptServices callbacks;
    std::function<void()> clear_handlers;
    std::function<void()> missing_script;
};

// Native433EA0 services for the existing shared interaction processor. Creating
// scene/script/package owners must outlive this adapter and its queued contexts.
class NativeMonsterInteraction final {
public:
    explicit NativeMonsterInteraction(NativeMonsterInteractionServices services);
    ~NativeMonsterInteraction();
    NativeMonsterInteraction(const NativeMonsterInteraction&) = delete;
    NativeMonsterInteraction& operator=(const NativeMonsterInteraction&) = delete;
    IMonsterInteractionRuntime& Runtime();
    bool Process(std::uint32_t monster, std::uint32_t target);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
