#pragma once
#include "pal4/script_player_garb.h"
#include <functional>
namespace pal4 {
class NativePlayerSlotCopyRuntime;
class IScriptPlayerActionsRuntime;
class IScriptProcessRuntime;
// Borrows the existing player text, scene-membership and model reload owners.
class NativeScriptPlayerGarbRuntime final : public IScriptPlayerGarbRuntime {
public:
    NativeScriptPlayerGarbRuntime(IScriptPlayerActionsRuntime&,NativePlayerSlotCopyRuntime&,
        IScriptProcessRuntime&,std::function<std::uint32_t()> control,
        std::function<void(std::uint32_t)> bind);
    std::uint32_t PlayerManager() override;
    std::uint32_t PlayerByIndex(std::uint32_t,std::int32_t) override;
    void AssignModelText(std::uint32_t,std::string_view) override;
    bool AllocateModelText(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void WriteModelText(std::uint32_t,std::string_view) override;
    bool NeedsModelReload(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t ArenaManager() override;
    void ScriptOnRemove(std::uint32_t,std::uint32_t) override;
    void ProcessAndCleanup(std::uint32_t) override;
    void SetStateFlag(std::uint32_t,std::uint32_t) override;
    void ScriptOnAdd(std::uint32_t,std::uint32_t) override;
    std::uint32_t ActorControl() override;
    void SetActor(std::uint32_t,std::uint32_t) override;
private:
    void RequirePlayer(std::uint32_t);
    IScriptPlayerActionsRuntime& scene_;
    NativePlayerSlotCopyRuntime& text_;
    IScriptProcessRuntime& reload_;
    std::function<std::uint32_t()> control_;
    std::function<void(std::uint32_t)> bind_;
};
}
