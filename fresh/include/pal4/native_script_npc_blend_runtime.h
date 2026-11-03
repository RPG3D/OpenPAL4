#pragma once
#include "pal4/script_player_blend.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include <memory>
#include <vector>

class asIScriptContext;
namespace pal4 {
class NativeArenaNpcSceneLoader;
class IScriptPlayerActionsRuntime;

// One context's NPC fade-in and fade-out handlers. Clear before the borrowed scene is
// released. Shared NPC entry/material algorithms retain model ownership.
class NativeScriptNpcBlendRuntime final : public IScriptPlayerBlendRuntime,
                                         public INativeScriptFrameWaitRuntime {
public:
    NativeScriptNpcBlendRuntime(NativeArenaNpcSceneLoader&,std::uint32_t arena,asIScriptContext&);
    void BindPlayers(IScriptPlayerActionsRuntime& players) noexcept { players_ = &players; }
    void ClearHandlers();
    bool Tick(float elapsed) override;
    bool BlocksScript() const override;
    bool Waiting() const noexcept { return !handlers_.empty(); }
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t,std::int32_t) override;
    std::uint32_t NpcManager() override;
    std::uint32_t NpcByName(std::uint32_t,std::string_view) override;
    std::string_view ActorName(std::uint32_t) override;
    std::uint32_t NpcInfoByName(std::uint32_t,std::string_view) override;
    void ProcessNpcEvents(std::uint32_t,std::uint32_t,std::uint32_t) override;
    PlayerBlendWaitState* Allocate(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void Construct(PlayerBlendWaitState&,std::uint8_t,std::uint32_t) override;
    void Flush(PlayerBlendWaitState&) override;
    void SetMaterialFlags(std::uint32_t,std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void RemoveFromArena(std::uint32_t,std::uint32_t) override;
    void AddToArena(std::uint32_t,std::uint32_t) override;
    void SetStateFlag(std::uint32_t,std::uint32_t) override;
    float Interpolate(float,float,float,std::uint32_t) override;
    std::int32_t ConvertAlpha(float) override;
    void Close(PlayerBlendWaitState&) override;
    std::uint32_t AddHandler(std::uint32_t,PlayerBlendWaitState*) override;
private:
    void RequireActor(std::uint32_t) const;
    bool IsPlayer(std::uint32_t) const;
    std::uint32_t QueueAddress() const noexcept;
    NativeArenaNpcSceneLoader& npcs_;
    const std::uint32_t arena_;
    asIScriptContext& context_;
    IScriptPlayerActionsRuntime* players_{};
    struct Handler {
        std::unique_ptr<PlayerBlendWaitState> state;
        std::uint8_t mode{};
        bool closed{};
    };
    std::unique_ptr<PlayerBlendWaitState> pending_;
    std::uint8_t pending_mode_{};
    std::vector<Handler> handlers_;
};
}
