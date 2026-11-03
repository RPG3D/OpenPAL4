#pragma once
#include "pal4/native_screen_effect_script_runtime.h"
#include "native_camera_effect_probe.h"
#include "pal4/native_screen_effect_camera.h"
#include "pal4/native_screen_effect_session.h"
#include "pal4/timed_effect_state.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include <rwcore.h>
#include <rpworld.h>
#include <iostream>
#include <cstring>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeScreenEffectScript(NativePkFileManagerRuntime& packages, std::uint32_t game_camera) {
    VerifyNativeCameraEffect();
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native screen effect script probe line="+std::to_string(at.line()));
    };
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());check(device!=nullptr);
    const auto allocations=packages.AllocationCount();
    const auto archives=packages.ArchiveCount();
    const auto stack=packages.Scripts().ReadWord(packages.ManagerAddress()+120u);
    NativeScreenEffectScriptRuntime runtime(packages,device);
    ScreenEffectState state{};state.vtable=0x842240u;
    auto* borrowed=reinterpret_cast<RwCamera*>(game_camera);check(borrowed!=nullptr);
    auto* original_world=RwCameraGetWorld(borrowed);
    auto* original_frame=RwCameraGetFrame(borrowed);
    auto* original_raster=RwCameraGetRaster(borrowed);
    // Small real camera textures make GPU readback bounded; the game path will
    // supply configured dimensions. Neither probe owner presents a window.
    NativeScreenEffectCamera capture(64,64,borrowed->nearPlane,borrowed->farPlane);
    NativeScreenEffectCamera destination(64,64,borrowed->nearPlane,borrowed->farPlane);
    capture.Bind(game_camera);destination.Bind(game_camera);
    check(capture.Initialize() && destination.Initialize());
    const auto sample=[&](bool shader) {
        IDirect3DSurface9* surface{};IDirect3DSurface9* staging{};
        bool begun{};
        try {
            destination.Begin();begun=true;
            check(SUCCEEDED(device->GetRenderTarget(0u,&surface)) && surface);
            if(shader) {
                (void)ScreenEffectControl::Enable(state);
                std::uint8_t active=1u;
                const auto passes=ScreenEffectScript::ApplyOldTV(state,reinterpret_cast<std::uint32_t>(&active),runtime);
                check(passes>0u);
                auto* effect=reinterpret_cast<ID3DXEffect*>(state.script);
                for(unsigned pass=0u;pass<passes;++pass) {
                    check(SUCCEEDED(effect->BeginPass(pass)));
                    capture.Draw(0,0,64,64);
                    (void)ScreenEffectControl::StopRendering(state,runtime);
                }
                (void)ScreenEffectControl::Render(state,runtime);
                (void)ScreenEffectControl::Disable(state);
            } else capture.Draw(0,0,64,64);
            destination.End();begun=false;
            D3DSURFACE_DESC desc{};check(SUCCEEDED(surface->GetDesc(&desc)));
            check(desc.Format==D3DFMT_A8R8G8B8 || desc.Format==D3DFMT_X8R8G8B8);
            check(SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr)));
            check(SUCCEEDED(device->GetRenderTargetData(surface,staging)));
            D3DLOCKED_RECT locked{};check(SUCCEEDED(staging->LockRect(&locked,nullptr,D3DLOCK_READONLY)));
            std::uint32_t pixel{};
            std::memcpy(&pixel,static_cast<const char*>(locked.pBits)+32u*locked.Pitch+32u*4u,4u);
            staging->UnlockRect();staging->Release();surface->Release();return pixel;
        } catch(...) {
            if(begun) destination.End();
            if(staging) staging->Release();if(surface) surface->Release();throw;
        }
    };
    try {
        for(unsigned cycle=0u;cycle<2u;++cycle) {
            check(ScreenEffectScript::LoadOldTV(state,0u,0u,runtime));
            check(state.loaded && !state.enabled && state.script && !runtime.LoadScopeActive());
            auto* effect=reinterpret_cast<ID3DXEffect*>(state.script);
            D3DXEFFECT_DESC description{};check(SUCCEEDED(effect->GetDesc(&description)));
            auto technique=effect->GetTechniqueByName("techOldTV");check(technique!=nullptr);
            check(SUCCEEDED(effect->ValidateTechnique(technique)));
            D3DXTECHNIQUE_DESC detail{};check(SUCCEEDED(effect->GetTechniqueDesc(technique,&detail)) && detail.Passes);
            (void)ScreenEffectControl::Enable(state);
            std::uint8_t active=1u;
            const auto count=ScreenEffectScript::ApplyOldTV(state,reinterpret_cast<std::uint32_t>(&active),runtime);
            check(count==detail.Passes);
            for(unsigned pass=0u;pass<count;++pass) {
                check(SUCCEEDED(effect->BeginPass(pass)));
                (void)ScreenEffectControl::StopRendering(state,runtime);
            }
            (void)ScreenEffectControl::Render(state,runtime);
            (void)ScreenEffectControl::Disable(state);
            check(!state.enabled);
            capture.Begin();
            RwRGBA color{200u,40u,20u,255u};
            check(RwCameraClear(reinterpret_cast<RwCamera*>(capture.State().camera),&color,3u)!=nullptr);
            capture.End();
            const auto* captured_frame=RwCameraGetFrame(reinterpret_cast<RwCamera*>(capture.State().camera));
            check(std::memcmp(RwFrameGetLTM(const_cast<RwFrame*>(captured_frame)),RwFrameGetLTM(original_frame),sizeof(RwMatrix))==0);
            const auto plain=sample(false);
            check((plain&0x00ffffffu)==0x00c82814u);
            const auto processed=sample(true);
            check((processed&0x00ffffffu)!=(plain&0x00ffffffu));
            check(RwCameraGetWorld(reinterpret_cast<RwCamera*>(capture.State().camera))==original_world);
            // Recreate, detach and rebind while the borrowed world still lives.
            capture.Unbind();
            check(!RwCameraGetWorld(reinterpret_cast<RwCamera*>(capture.State().camera)));
            capture.Bind(game_camera);check(capture.Initialize());
            // The next iteration deliberately loads over an existing effect;
            // shared Reset must release its old COM owner before replacement.
        }
        (void)ScreenEffectScript::CleanupSingleScript(state,runtime);
        (void)ScreenEffectScript::CleanupSingleScript(state,runtime);
        check(!state.loaded && !state.enabled && !state.script);
        runtime.ProcessPackageScript();
        check(!runtime.OpenFile("gamedata\\2D\\ScreenEffect\\missing.fx","r"));
        std::uint32_t environment{};runtime.CleanupScriptEnvironment(environment);
        check(!runtime.LoadScopeActive());
        runtime.ProcessPackageScript();
        constexpr char invalid[]="this is deliberately invalid effect source";
        const auto buffer=runtime.AllocateBuffer(sizeof(invalid));
        std::memcpy(reinterpret_cast<void*>(buffer),invalid,sizeof(invalid));
        std::uint32_t output{},error{};
        check(runtime.LoadScript(runtime.CurrentD3DDevice(),buffer,sizeof(invalid),0u,0u,0x105u,0u,output,error)!=0u && !output && error);
        bool failed{};
        try {runtime.ShowError(runtime.ErrorText(error,"Error"),0u);}
        catch(const std::runtime_error&) {failed=true;}
        check(failed);
        // Same path the shared loader invokes when ShowError throws: release
        // diagnostics and still-owned source buffers without a modal dialog.
        runtime.CleanupScriptEnvironment(environment);
        check(!runtime.LoadScopeActive());
    } catch(...) {
        (void)ScreenEffectScript::CleanupSingleScript(state,runtime);throw;
    }
    check(packages.AllocationCount()==allocations && packages.ArchiveCount()==archives &&
        packages.Scripts().ReadWord(packages.ManagerAddress()+120u)==stack);
    capture.Cleanup();capture.Cleanup();destination.Cleanup();
    check(!capture.State().active && !capture.State().camera && !capture.State().texture);
    check(RwCameraGetWorld(borrowed)==original_world && RwCameraGetFrame(borrowed)==original_frame && RwCameraGetRaster(borrowed)==original_raster);
    NativeBaseObjectMemory memory;
    ScreenEffectDispatcherState dispatcher{};dispatcher.tree_sentinel=0xfffffff0u;
    {
        NativeScreenEffectSession session(memory,packages,dispatcher);
        session.BindCamera(game_camera);check(session.Initialize());
        check(memory.OutstandingAllocations()==16u && !dispatcher.active);
        check(destination.Initialize());
        for(const auto key : {12, 2, 2, 12}) {
            session.HandleScript(key);check(dispatcher.active==1 && dispatcher.current_key==key);
            const auto camera=session.BeginFrame();check(camera!=game_camera);
            session.EndFrame();
            destination.Begin();session.RenderPasses(0u);destination.End();
            GameManagerReset::StopActive(dispatcher,session);
            check(!dispatcher.active && !dispatcher.current_key && !dispatcher.current_handler);
            // Capture storage remains available after Stop. This local adapter
            // probe explicitly calls its plain fallback; Scene frame selection
            // separately follows the shared callback swap/restore.
            check(session.CapturingEnabled());
            (void)session.BeginFrame();session.EndFrame();
            destination.Begin();session.RenderPasses(0u);destination.End();
        }
        const auto before_trails=memory.OutstandingAllocations();
        for(unsigned cycle=0;cycle<3;++cycle) {
            const auto starting_capture=dispatcher.frame_state;
            session.HandleScript(6); // giCGEffPlay(5) maps to catalog key6.
            auto& trails=*reinterpret_cast<TimedEffectState*>(dispatcher.current_handler);
            check(trails.loaded && trails.enabled && trails.camera_count==10 && !trails.completed_cycles);
            check(memory.OutstandingAllocations()==before_trails+11u);
            for(unsigned frame=1;frame<=24u+cycle;++frame) {
                session.UpdateRenderParameters(0.017);
                const auto used=dispatcher.frame_state;
                const auto frame_camera=session.BeginFrame();
                RwRGBA color{static_cast<RwUInt8>(frame*8u),40u,20u,255u};
                check(RwCameraClear(reinterpret_cast<RwCamera*>(frame_camera),&color,3u)!=nullptr);
                session.EndFrame();
                check(trails.completed_cycles==std::min(frame/2u,10u));
                check((dispatcher.frame_state!=used)==(frame%2u==0u));
                destination.Begin();session.RenderPasses(0u);destination.End();
            }
            check(dispatcher.frame_state!=starting_capture);
            const auto retained=*reinterpret_cast<const std::uint32_t*>(trails.cameras_begin);
            GameManagerReset::StopActive(dispatcher,session);
            check(!trails.loaded && !trails.enabled && !trails.active_camera && dispatcher.frame_state==retained);
            check(memory.OutstandingAllocations()==before_trails+1u);
            // Odd/even rotations retain a different owner; the remaining ten records are retired.
        }
        bool unsupported{};
        try {session.HandleScript(11);}catch(const std::logic_error&) {unsupported=true;}
        check(unsupported && !dispatcher.active);
        session.UnbindCamera();check(!session.CapturingEnabled());
        session.BindCamera(game_camera);check(session.CapturingEnabled());
        (void)GameManagerReset::Reset(dispatcher,0u,session);
        (void)GameManagerReset::Reset(dispatcher,0u,session);
        check(!dispatcher.enabled && !dispatcher.frame_state && memory.OutstandingAllocations()==0u);
        destination.Cleanup();
    }
    check(memory.OutstandingAllocations()==0u && packages.AllocationCount()==allocations && packages.ArchiveCount()==archives &&
        packages.Scripts().ReadWord(packages.ManagerAddress()+120u)==stack);
    check(RwCameraGetWorld(borrowed)==original_world && RwCameraGetFrame(borrowed)==original_frame && RwCameraGetRaster(borrowed)==original_raster);
    std::cout<<"Screen effect OldTV: original2D FX, real capture/quad GPU pixel readback, changed shader output, LTM/world sync, recreation and cleanup verified\n";
}
}
