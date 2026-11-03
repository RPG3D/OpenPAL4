#pragma once
#include "pal4/native_combat_prerequisites.h"
#include "pal4/native_combat_cold_initialization.h"
#include "pal4/native_combat_start_console.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_combat_console.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_event_router.h"
#include "pal4/native_skill_system_services.h"
#include "pal4/native_pk_file_manager_runtime.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/d3dx9_game_runtime.h"
#include <rwcore.h>
#include <bit>
#include <iostream>
#include <stdexcept>
#include <cstring>

namespace pal4::test {
inline void VerifyNativeCombatColdInitialization(const std::filesystem::path& root,
    NativePkFileManagerRuntime& packages,NativeCameraManagerOwner& cameras,
    INativeSkillSystemServices& skills) {
    const auto check=[](bool v){if(!v)throw std::runtime_error("native Combat cold initialization probe");};
    const auto reject=[&](auto f){bool threw=false;try{f();}catch(const std::exception&){threw=true;}check(threw);};
    const auto allocations=packages.AllocationCount(),archives=packages.ArchiveCount();
    const auto stack=packages.Scripts().ReadWord(packages.ManagerAddress()+120u);
    const auto active=cameras.ActiveCameraPointer();
    NativeCombatSystemImage image;
    NativeCombatTranslator translator(image.translator);
    NativeCombatTranslator foreign;
    NativeBaseObjectMemory memory;
    NativeScriptEventRouter router;
    NativeCombatConsole console(memory,router);(void)console.Construct();
    NativeMatFxScriptSession shader(packages,static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice()));
    const auto address=reinterpret_cast<std::uint32_t>(&image);
    auto* bytes=reinterpret_cast<std::uint8_t*>(&image);
    bytes[518]=0xa5u;bytes[519]=0x5au;image.target_selection.words[17]=0x1234u;
    unsigned begins{},ends{};bool fail_end=true;
    const auto logger=[&](bool beginning){
        if(beginning)++begins;else {++ends;if(fail_end)throw std::runtime_error("intentional final cold log failure");}
    };
    reject([&]{InitializeNativeCombatCold(image,foreign,console,shader,cameras,skills,root,logger);});
    check(!begins && !translator.Loaded() && !console.SubscriptionCount() && !shader.Effect() && !bytes[517]);
    // Fails after actual database, subscriptions, original shader compilation,
    // package cleanup and camera setup; the final shared commit must not occur.
    reject([&]{InitializeNativeCombatCold(image,translator,console,shader,cameras,skills,root,logger);});
    check(begins==1u && ends==1u && !bytes[517] && !translator.Loaded() && !shader.Effect() &&
          !shader.LoadScopeActive() && !console.SubscriptionCount() && !router.SubscriptionCount() &&
          image.target_selection.words[17]==0u && bytes[518]==0xa5u && bytes[519]==0x5au);
    fail_end=false;
    InitializeNativeCombatCold(image,translator,console,shader,cameras,skills,root,logger);
    check(begins==2u && ends==2u && bytes[517]==1u && translator.Address()==address+380u &&
          translator.Loaded() && shader.Effect() && !shader.LoadScopeActive() &&
          console.SubscriptionCount()==23u && router.SubscriptionCount()==23u);
    const auto effect=shader.Effect();
    reject([&]{InitializeNativeCombatCold(image,translator,console,shader,cameras,skills,root,logger);});
    check(begins==2u && shader.Effect()==effect && bytes[517]==1u && console.SubscriptionCount()==23u);
    unsigned resets{};
    const auto observer=router.Subscribe(router.Key("combat_reset"),1u,[&](std::uint32_t payload){
        check(payload && *reinterpret_cast<const std::int32_t*>(payload)==0);
        check(console.SubscriptionCount()==(image.Mode()==0u?23u:0u));
        check(cameras.ActiveCameraPointer()==active);++resets;
    });
    for(const auto mode:{0u,1u,2u,0u}) {
        image.Mode()=static_cast<std::uint8_t>(mode);
        ResetNativeCombatStartConsole(image,console,cameras,router);
        const auto& camera=cameras.CameraDataForObject(cameras.FindByName("CombatCamera"));
        check(std::bit_cast<float>(camera.At(0x48u))==452.0f &&
              std::bit_cast<float>(camera.At(0x3cu))==178.0f &&
              std::bit_cast<float>(camera.At(0x74u))==13.0f && router.Contains(observer));
    }
    check(resets==4u);(void)router.Unsubscribe(observer);
    std::cout<<"Native combat Start console modes: 0/1/2/0 subscriptions23/0/0/23 synchronous reset0, camera defaults and Scene camera preserved pass\n";
    console.Destroy();shader.Reset();translator.Reset();
    check(!router.SubscriptionCount() && !memory.OutstandingAllocations() &&
          packages.AllocationCount()==allocations && packages.ArchiveCount()==archives &&
          packages.Scripts().ReadWord(packages.ManagerAddress()+120u)==stack && cameras.ActiveCameraPointer()==active);
    std::cout<<"Native combat cold initialization: original DB/MatFX/camera, final-log failure before517, retry, identity/duplicate guards,23 subscriptions and balanced resource cleanup pass\n";
}
inline void VerifyNativeCombatPrerequisites(NativePkFileManagerRuntime& packages,
                                           NativeCameraManagerOwner& cameras) {
    const auto check = [](bool ok) {
        if (!ok) throw std::runtime_error("native combat prerequisite probe");
    };
    const auto allocations = packages.AllocationCount();
    const auto archives = packages.ArchiveCount();
    const auto stack = packages.Scripts().ReadWord(packages.ManagerAddress() + 120u);
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    NativeMatFxScriptSession shader(packages, device);
    check(!shader.Effect() && !shader.LoadScopeActive());
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        check(shader.Load() && shader.Effect() && !shader.LoadScopeActive());
        auto* effect = reinterpret_cast<ID3DXEffect*>(shader.Effect());
        D3DXEFFECT_DESC desc{};
        check(SUCCEEDED(effect->GetDesc(&desc)) && desc.Techniques > 0u);
        auto technique = effect->GetTechniqueByName("RtkWorldLtMap");
        check(technique && SUCCEEDED(effect->ValidateTechnique(technique)));
        D3DXTECHNIQUE_DESC detail{};
        check(SUCCEEDED(effect->GetTechniqueDesc(technique, &detail)) && detail.Passes > 0u);
        check(SUCCEEDED(effect->SetTechnique(technique)));
        UINT passes{};
        check(SUCCEEDED(effect->Begin(&passes, 0u)) && passes == detail.Passes);
        for (UINT pass = 0u; pass < passes; ++pass) {
            const auto begin = effect->BeginPass(pass);
            if (FAILED(begin)) { (void)effect->End(); check(false); }
            const auto end = effect->EndPass();
            if (FAILED(end)) { (void)effect->End(); check(false); }
        }
        check(SUCCEEDED(effect->End()));
        std::cout << "Native combat MatFX: technique=" << detail.Name << " passes=" << passes << '\n';
    }
    check(SUCCEEDED(static_cast<HRESULT>(shader.OnLostDevice())));
    check(SUCCEEDED(static_cast<HRESULT>(shader.OnResetDevice())));
    check(!shader.Load("gamedata\\MatFX\\Shader\\missing.fx") && !shader.Effect() && !shader.LoadScopeActive());
    check(shader.Load());
    bool rejected{};
    try { (void)shader.Load(std::string_view("bad\0path", 8u)); }
    catch (const std::invalid_argument&) { rejected = true; }
    check(rejected && !shader.Effect() && !shader.LoadScopeActive());
    check(shader.Load());
    shader.Reset(); shader.Reset();
    check(!shader.Effect() && !shader.LoadScopeActive());
    check(packages.AllocationCount() == allocations && packages.ArchiveCount() == archives &&
          packages.Scripts().ReadWord(packages.ManagerAddress() + 120u) == stack);
    const auto active = cameras.ActiveCameraPointer();
    const auto main = cameras.CameraDataForObject(cameras.FindByName("MainCamera"));
    (void)SetupNativeCombatCamera(cameras);
    const auto& combat = cameras.CameraDataForObject(cameras.FindByName("CombatCamera"));
    check(std::bit_cast<float>(combat.At(0x3cu)) == 178.0f &&
          std::bit_cast<float>(combat.At(0x40u)) == 14.0f &&
          std::bit_cast<float>(combat.At(0x44u)) == 0.0f &&
          std::bit_cast<float>(combat.At(0x74u)) == 13.0f &&
          std::bit_cast<float>(combat.At(0x78u)) == 20.0f &&
          std::bit_cast<float>(combat.At(0x7cu)) == -2.0f &&
          std::bit_cast<float>(combat.At(0x48u)) == 452.0f);
    check(cameras.ActiveCameraPointer() == active &&
          cameras.CameraDataForObject(cameras.FindByName("MainCamera")).words == main.words);
    std::cout << "Native combat prerequisites: original MatFX compile/pass/reset/reload, package balance, "
                 "CombatCamera defaults and unchanged active Scene camera verified\n";
}
}
