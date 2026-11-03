#pragma once
#include "pal4/native_arena_active_player_scene_binding.h"
#include "pal4/player_manager_object.h"
#include "pal4/script_player_actions.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/script_current_player_visibility.h"
#include <memory>
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePlayerVisibility(PlayerManagerObjectState& players,
    IScriptPlayerActionsRuntime& actions, IScriptCurrentPlayerVisibilityRuntime& current,
    std::uint32_t collision,
    NativeArenaActivePlayerSceneBinding& active) {
    const auto leader = players.Word(0x2c50u / 4u);
    if (leader != 0u || !active.Entered())
        throw std::logic_error("visibility probe requires original cave leader");
    const auto verify = [&](std::size_t index, bool visible) {
        const auto& player = players.slots[index].base;
        if (player.Read(0x1f0u) != (visible ? 1u : 0u) ||
            player.Read(0x568u) != (visible ? collision : 0u) ||
            (index == leader && active.Entered() != visible))
            throw std::runtime_error("player visibility state/collision/owner mismatch");
        // The real Role models have shadow clump/light bindings. Script Add
        // must use the same embedded camera lifecycle as current-player entry.
        if (player.Read(0x30u) && player.Read(0x34u)) {
            const auto camera = player.Read(0x50u); // CameraRenderState+24 at actor+56.
            const auto raster = player.Read(0x58u); // CameraRenderState+32.
            if ((camera != 0u) != visible || (raster != 0u) != visible)
                throw std::runtime_error("player visibility shadow camera/raster ownership mismatch");
        }
    };
    for (const auto index : {1u, 0u}) {
        const auto original_visible = players.slots[index].base.Read(0x1f0u) != 0u;
        for (unsigned repeat = 0u; repeat < 2u; ++repeat) {
            (void)ScriptPlayerActions::SetVisible(index, false, actions);
            verify(index, false);
            (void)ScriptPlayerActions::SetVisible(index, true, actions);
            verify(index, true);
            // Repeated show must execute remove/add and preserve collision ownership.
            (void)ScriptPlayerActions::SetVisible(index, true, actions);
            verify(index, true);
        }
        (void)ScriptPlayerActions::SetVisible(index, original_visible, actions);
        verify(index, original_visible);
    }
    NativeAngelScriptEngine engine;
    NativeGameScriptServices services{};services.current_player_visibility=&current;
    NativeGameScriptCallbacks callbacks(services);engine.SetCallbacks(&callbacks);
    constexpr char script[]="void Hide() {giPlayerCurrentSetVisible(false);} "
        "void Show() {giPlayerCurrentSetVisible(true);}";
    if(engine.Raw()->AddScriptSection("visibility","visibility.cs",script,sizeof(script)-1)<0 ||
       engine.Raw()->Build("visibility")<0)throw std::runtime_error("compile current visibility native probe");
    asIScriptContext* raw=nullptr;
    if(engine.Raw()->CreateContext(&raw)<0 || !raw)throw std::runtime_error("current visibility context");
    const std::unique_ptr<asIScriptContext,void(*)(asIScriptContext*)> context(raw,[](auto* p){p->Release();});
    const auto run=[&](const char* entry,bool visible){
        if(raw->Prepare(engine.FunctionId("visibility",entry))<0 || raw->Execute()!=asEXECUTION_FINISHED)
            throw std::runtime_error("compiled current visibility callback failed");
        verify(leader,visible);
    };
    run("Hide",false);run("Hide",false);run("Show",true);run("Show",true);
    std::cout << "Current visibility: compiled -113 hide/show preserves native collision, cameras and active owner\n";
    if (players.Word(0x2c50u / 4u) != leader)
        throw std::runtime_error("visibility changed the party leader");
}
}  // namespace pal4::test
