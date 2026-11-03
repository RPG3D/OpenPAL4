#pragma once
#include "pal4/native_particle_effect_factory.h"
#include "pal4/native_base_object_memory.h"
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
namespace pal4::test {
inline void VerifyNativeNestedEffectFacing(NativeBaseObjectMemory& memory,
    NativeParticleEffectFactory& objects,std::uint32_t object) {
    const auto check=[](bool ok){if(!ok)throw std::runtime_error("nested facing SDK frame regression");};
    const auto position=objects.ResolveFacet(object,"PositionTask");
    const auto rotation=objects.ResolveFacet(object,"RotateTask");
    const auto scale=objects.ResolveFacet(object,"ScaleTask");
    const auto facing=objects.ResolveFacet(object,"MoveToTargetTask");
    check(facing==object+48u);
    const auto old_position=objects.ReadPosition(position);
    std::array<float,3> old_rotation{},old_scale{};
    objects.ReadRotation(rotation,old_rotation);objects.ReadScale(scale,old_scale);
    std::array<std::uint32_t,7> old_cache{};
    for(unsigned i=0;i<old_cache.size();++i)old_cache[i]=memory.ReadDword(object+104u+4u*i);
    const auto before=memory.OutstandingAllocations();
    objects.Move(position,{4.f,5.f,6.f},false);
    objects.SetScale(scale,{2.f,3.f,4.f},0u);
    objects.Face(facing,{104.f,5.f,6.f});
    objects.SetFaceRotation(facing,{0.f,0.f,0.f});
    objects.ActivateFace(facing);
    const auto* m=RwFrameGetMatrix(reinterpret_cast<RwFrame*>(objects.Frame(object)));
    const auto close=[](float a,float b){return std::abs(a-b)<0.002f;};
    check(close(m->at.x,4.f)&&close(m->at.y,0.f)&&close(m->at.z,0.f));
    check(close(m->right.x,0.f)&&close(m->right.z,-2.f)&&close(m->up.y,3.f));
    check(close(m->pos.x,4.f)&&close(m->pos.y,5.f)&&close(m->pos.z,6.f));
    check(memory.ReadDword(object+104u)==std::bit_cast<std::uint32_t>(104.f) &&
        memory.ReadDword(object+108u)==std::bit_cast<std::uint32_t>(5.f) &&
        memory.ReadDword(object+112u)==std::bit_cast<std::uint32_t>(6.f) &&
        memory.ReadByte(object+128u)==1u && memory.OutstandingAllocations()==before);
    objects.SetFaceRotation(facing,{0.f,15.f,0.f});objects.ActivateFace(facing);
    check(memory.ReadDword(object+120u)==std::bit_cast<std::uint32_t>(15.f));
    check(close(m->pos.x,4.f)&&close(m->pos.y,5.f)&&close(m->pos.z,6.f));
    objects.Move(position,old_position,false);objects.ApplyRotation(rotation,old_rotation,false);
    objects.SetScale(scale,old_scale,0u);
    for(unsigned i=0;i<old_cache.size();++i)memory.WriteDword(object+104u+4u*i,old_cache[i]);
}
}
