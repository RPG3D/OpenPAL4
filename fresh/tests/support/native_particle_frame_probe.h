#pragma once
#include "pal4/native_particle_configuration.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/particle_visual_object.h"
#include "native_particle_probe_camera.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rpptank.h>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <iostream>
#include <source_location>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeParticleFrame(NativeParticleConfiguration& owner,NativeBaseObjectMemory& memory) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native particle frame line "+std::to_string(at.line()));
    };
    auto& state=*reinterpret_cast<ParticleVisualState*>(owner.State().words[23]);
    auto* atomic=reinterpret_cast<RpAtomic*>(state.ReadWord(40u));
    check(!state.ReadWord(204u) && state.ReadByte(1u));
    (void)owner.Update(0.01);
    const auto allocated=state.ReadWord(204u),live=state.ReadWord(208u);
    check(allocated && allocated==live && allocated<=state.ReadWord(200u));
    check(RpPTankAtomicGetActiveParticlesCount(atomic)==static_cast<RwInt32>(allocated));
    const auto first=memory.ReadDword(state.ReadWord(160u));
    check(memory.ReadByte(first) && std::bit_cast<float>(memory.ReadDword(first+4u))==0.01f);
    RpPTankLockStruct positions{},colors{},sizes{};
    check(RpPTankAtomicLock(atomic,&positions,rpPTANKDFLAGPOSITION,rpPTANKLOCKREAD));
    check(RpPTankAtomicLock(atomic,&colors,rpPTANKDFLAGCOLOR,rpPTANKLOCKREAD));
    check(RpPTankAtomicLock(atomic,&sizes,rpPTANKDFLAGSIZE,rpPTANKLOCKREAD));
    RwV3d position{};std::memcpy(&position,positions.data,sizeof(position));
    RwV2d size{};std::memcpy(&size,sizes.data,sizeof(size));
    check(RpPTankAtomicUnlock(atomic));
    // H_212 starts at alpha zero: its random lifetime can leave the first
    // particle below one alpha byte at 10ms. Pixel readback below checks drawing.
    check(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z) &&
        size.x>0.0f && size.y>0.0f);
    RwV3d expected{std::bit_cast<float>(memory.ReadDword(first+68u)),
        std::bit_cast<float>(memory.ReadDword(first+72u)),
        std::bit_cast<float>(memory.ReadDword(first+76u))};
    // IDA 64660D: local-space particles are uploaded through the frame LTM.
    if(state.ReadByte(4u)) {
        const auto local=expected;
        RwV3dTransformPoint(&expected,&local,
            RwFrameGetLTM(reinterpret_cast<RwFrame*>(state.ReadWord(44u))));
    }
    check(std::abs(position.x-expected.x)<0.00001f &&
        std::abs(position.y-expected.y)<0.00001f &&
        std::abs(position.z-expected.z)<0.00001f);
    // A real SDK camera invokes the PTank render callback; state changes must
    // be balanced across the draw. It uses the existing nonactivating host.
    NativeParticleProbeCamera camera;
    camera.Begin(std::bit_cast<float>(state.ReadWord(116u)));
    constexpr std::array<RwUInt32,5> states{27u,19u,20u,7u,14u};
    std::array<RwUInt32,5> before{};
    for(unsigned i=0;i<states.size();++i) RwD3D9GetRenderState(states[i],&before[i]);
    (void)owner.Draw();
    for(unsigned i=0;i<states.size();++i){RwUInt32 after{};RwD3D9GetRenderState(states[i],&after);check(after==before[i]);}
    const auto colored=camera.EndPixels();check(colored>0u);
    bool missing_camera=false;
    try{(void)owner.Draw();}catch(const std::logic_error&){missing_camera=true;}
    check(missing_camera);
    // Skip suppresses emission, but original Update still ages particles.
    (void)owner.Update(0.1,0u,true);
    check(state.ReadWord(204u)==allocated &&
        std::abs(std::bit_cast<float>(memory.ReadDword(first+4u))-0.11f)<0.00001f);
    (void)owner.Update(3.0,0u,true);
    check(!state.ReadWord(208u) && !memory.ReadByte(first));
    check(RpPTankAtomicGetActiveParticlesCount(atomic)==static_cast<RwInt32>(allocated));
    check(RpPTankAtomicLock(atomic,&colors,rpPTANKDFLAGCOLOR,rpPTANKLOCKREAD));
    bool transparent=true;
    for(std::uint32_t i=0;i<allocated;++i) transparent=transparent && colors.data[i*colors.stride+3u]==0u;
    check(RpPTankAtomicUnlock(atomic) && transparent);
    (void)owner.Update(0.01);
    check(state.ReadWord(208u)>0u && memory.ReadDword(state.ReadWord(160u))==first);
    bool rejected=false;
    const auto prior=state.bytes;
    try{(void)owner.Update(-1.0);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected && state.bytes==prior);
    std::cout << "Native particle frame: slots=" << allocated << " colored-pixels=" << colored
              << " expiration/reuse/render-state restoration verified\n";
}
}
