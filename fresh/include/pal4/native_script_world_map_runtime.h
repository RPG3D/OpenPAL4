#pragma once

#include "pal4/native_script_tutorial_runtime.h"
#include "pal4/script_world_map_wait.h"

namespace pal4 {
class IGameArenaLoadRuntime;

// The original WorldMap uses the same 84966C visibility wait as Tutorial.
// Retain its context owner while borrowing the map UI and XML state services.
class NativeScriptWorldMapRuntime final : public IScriptWorldMapWaitRuntime,
                                         public INativeScriptHandlerWaitRuntime {
public:
    NativeScriptWorldMapRuntime(asIScriptContext&, IGameArenaLoadRuntime&,
                                NativeTutorialServices);
    bool BlocksScript() const override { return wait_.BlocksScript(); }
    bool Tick(float elapsed) override { return wait_.Tick(elapsed); }
    void ClearHandlers();

    std::uint32_t UiFrameManager() override;
    void ProcessWorldMapEvent(std::uint32_t, std::uint32_t, std::uint32_t) override;
    std::uint32_t FindFrame(std::uint32_t, std::string_view) override;
    TutorialWaitState* AllocateWait(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructWait(TutorialWaitState&, std::uint8_t, std::uint32_t) override;
    void FlushWait(TutorialWaitState&) override;
    void AddHandler(std::uint32_t, TutorialWaitState*) override;
    std::uint32_t CreateDocument() override;
    void EnterState(std::uint32_t, std::uint32_t) override;
    std::uint32_t DestroyDocument(std::uint32_t) override;

private:
    asIScriptContext& context_;
    IGameArenaLoadRuntime& xml_;
    NativeScriptTutorialRuntime wait_;
    std::uint32_t document_{};
};
} // namespace pal4
