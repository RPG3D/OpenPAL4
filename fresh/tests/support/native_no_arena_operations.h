#pragma once
#include "pal4/native_arena_load_runtime.h"
#include <stdexcept>
namespace pal4::test {
struct NoArenaOperations final : pal4::INativeArenaLoadHost {
    [[noreturn]] static void Unexpected(){throw std::logic_error("System UI fixture attempted an arena operation");}
    std::uint32_t ActorControlAddress()override{Unexpected();}
    std::uint32_t InputManagerAddress()override{Unexpected();}
    std::uint32_t UiFrameManagerAddress()override{Unexpected();}
    std::uint32_t Milliseconds()override{Unexpected();}
    void ProcessActorInput(bool)override{Unexpected();}
    void InitializeInputState()override{Unexpected();}
    void ActivateDefaultCamera()override{Unexpected();}
    void ShowLoadingWindow(bool)override{Unexpected();}
    void SetDefaultCursor()override{Unexpected();}
    void LogStateExchange(std::string_view,std::string_view,std::uint32_t)override{Unexpected();}
    std::uint32_t RenderLoadingUi(std::uint32_t)override{Unexpected();}
    void LoadSave(std::uint32_t)override{Unexpected();}
    void LeaveSceneState(std::uint32_t)override{Unexpected();}
    bool EnterSceneState(std::uint32_t,std::uint32_t)override{Unexpected();}
    bool EnterSaveState(std::uint32_t,std::uint32_t)override{Unexpected();}
    void LeaveSaveState(std::uint32_t)override{Unexpected();}
    void ProcessSaveWindowEvents()override{Unexpected();}
    std::uint32_t SaveFrameAddress()override{Unexpected();}
    bool SaveFrameVisible(std::uint32_t)override{Unexpected();}
    void LoadSceneResources(std::string_view,std::string_view)override{Unexpected();}
    void LoadSceneRuntime(std::string_view)override{Unexpected();}
    void InitializePlayerScene()override{Unexpected();}
};
}
