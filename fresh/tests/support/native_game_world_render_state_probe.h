#pragma once
#include "pal4/native_game_world_render_state.h"
#include "pal4/native_game_arena_manager_owner.h"
#include <rwcore.h>
#include <d3d9.h>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeGameWorldRenderState(NativeGameArenaManagerOwner& arena){
    const auto check=[](bool ok,const char* m){if(!ok)throw std::runtime_error(m);};
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());check(device,"actual scene fog device");
    constexpr std::array<unsigned,9> keys{28,35,140,48,34,36,37,38,27};std::array<DWORD,9> before{};
    for(std::size_t i=0;i<keys.size();++i)check(SUCCEEDED(device->GetRenderState(static_cast<D3DRENDERSTATETYPE>(keys[i]),&before[i])),"read actual scene render states");
    auto* slot=reinterpret_cast<std::uint32_t*>(arena.Address()+504u);const auto original=*slot;
    struct Restore{IDirect3DDevice9* device;std::uint32_t* slot;std::uint32_t original;const std::array<DWORD,9>& before;
        ~Restore(){*slot=original;constexpr std::array<unsigned,9> keys{28,35,140,48,34,36,37,38,27};for(std::size_t i=0;i<keys.size();++i)(void)device->SetRenderState(static_cast<D3DRENDERSTATETYPE>(keys[i]),before[i]);}} restore{device,slot,original,before};
    const auto get=[&](unsigned key){DWORD value{};check(SUCCEEDED(device->GetRenderState(static_cast<D3DRENDERSTATETYPE>(key),&value)),"read applied scene fog state");return value;};
    std::array<std::uint32_t,6> settings{0x101u,0xff123456u,3u,std::bit_cast<std::uint32_t>(5.f),std::bit_cast<std::uint32_t>(25.f),std::bit_cast<std::uint32_t>(0.25f)};
    *slot=reinterpret_cast<std::uint32_t>(settings.data());
    NativeGameWorldRenderState::Apply(arena);
    check(get(28)==1u && get(35)==3u && get(140)==0u && get(48)==0u && get(34)==settings[1] && get(36)==settings[3] && get(37)==settings[4],"original linear fog writes actual D3D9 values");
    NativeGameWorldRenderState::Clear(arena);
    check(!get(28) && !get(35) && !get(140) && !get(48),"original scene fog clear writes actual D3D9 values");
    settings[2]=1u;NativeGameWorldRenderState::Apply(arena);
    check(get(28)==1u && get(35)==1u && get(38)==settings[5],"original exponential fog density writes actual device");
    settings[0]=1u;NativeGameWorldRenderState::Apply(arena);check(!get(28),"original enabled record can disable fog");
    settings[0]=0u;const auto mode=get(35);NativeGameWorldRenderState::Apply(arena);NativeGameWorldRenderState::Clear(arena);check(get(35)==mode,"disabled record leaves device unchanged");
    *slot=0u;NativeGameWorldRenderState::Apply(arena);NativeGameWorldRenderState::Clear(arena);check(get(35)==mode,"absent record leaves device unchanged");
    NativeGameArenaManagerOwner absent;bool rejected{};try{NativeGameWorldRenderState::Apply(absent);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"retired arena cannot draw scene fog");
    std::cout<<"Native scene render state: actual D3D9 linear/exponential/disable/clear, null and disabled records, retired owner, borrowed state/device restoration pass\n";
}
}
