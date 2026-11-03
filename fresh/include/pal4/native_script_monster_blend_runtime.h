#pragma once
#include "pal4/script_monster_blend.h"
#include "pal4/native_script_frame_wait_runtime.h"
#include <memory>
#include <vector>
class asIScriptContext;
namespace pal4 {
class NativeMonsterManagerOwner;
class NativeScriptMonsterVisibilityRuntime;
// Context-owned handlers borrow the scene's existing monster/entry services.
class NativeScriptMonsterBlendRuntime final : public IScriptMonsterBlendRuntime,
                                             public INativeScriptFrameWaitRuntime {
public:
    NativeScriptMonsterBlendRuntime(NativeMonsterManagerOwner&,
        NativeScriptMonsterVisibilityRuntime&,asIScriptContext&);
    void ClearHandlers();
    bool Tick(float elapsed) override;
    bool BlocksScript() const override;
    std::uint32_t MonsterManager() override;
    std::uint32_t FindMonster(std::uint32_t,std::string_view) override;
    ObjectBlendWaitState* AllocateWait(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructWait(ObjectBlendWaitState&,std::uint8_t,std::uint32_t) override;
    void FlushWait(ObjectBlendWaitState&) override;
    std::uint32_t AddHandler(std::uint32_t,ObjectBlendWaitState*) override;
    void SetMaterialFlags(std::uint32_t,std::uint32_t) override;
    void LeaveCharacter(std::uint32_t) override;
    void EnterCharacter(std::uint32_t) override;
    void SetStateFlag(std::uint32_t,std::uint32_t) override;
    std::string_view MonsterName(std::uint32_t) override;
    std::uint32_t FindMonsterInfo(std::uint32_t,std::string_view) override;
    bool SetAction(std::uint32_t,std::string_view,float) override;
    void ReplaceComponent(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint8_t InterpolateAlpha(float,float,float) override;
    void CloseWait(ObjectBlendWaitState&) override;
private:
    void RequireMonster(std::uint32_t) const;
    std::uint32_t Queue() const noexcept;
    NativeMonsterManagerOwner& monsters_;
    NativeScriptMonsterVisibilityRuntime& visibility_;
    asIScriptContext& context_;
    struct Handler {std::unique_ptr<ObjectBlendWaitState> state;std::uint8_t mode{};bool closed{};};
    std::unique_ptr<ObjectBlendWaitState> pending_;
    std::uint8_t pending_mode_{};
    std::vector<Handler> handlers_;
};
}
