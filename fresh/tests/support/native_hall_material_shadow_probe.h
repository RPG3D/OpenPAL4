#pragma once
#include "pal4/native_script_npc_material_runtime.h"
#include "pal4/native_script_shadow_runtime.h"
#include "pal4/native_actor_shadow_binding.h"
#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_player_manager_session.h"
#include "pal4/base_object_material_texture.h"
#include <rpworld.h>
#include <array>
#include <string>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeHallMaterialShadow(const NativeAnimatedActorAsset& actor,
    IBaseObjectMaterialRuntime& textures, NativePlayerManagerSession& players,
    NativeScriptShadowRuntime& shadow) {
    auto* material=actor.FaceMaterial();
    auto* original=material ? RpMaterialGetTexture(material) : nullptr;
    if (!original || std::string(RwTextureGetName(original))=="101_by")
        throw std::runtime_error("hall material fixture requires real101 default FACE");
    const std::string original_name=RwTextureGetName(original);
    (void)RwTextureAddRef(original);
    struct RestoreTexture {
        RpMaterial* material; RwTexture* original;
        ~RestoreTexture() { (void)RpMaterialSetTexture(material,original); (void)RwTextureDestroy(original); }
    } restore{material,original};
    const auto resolve = [&](std::string_view path) {
        textures.AddAndProcessScripts();
        std::uint32_t texture{};
        try { texture=textures.ResolveTexture(textures.TextureDictionary(),path); }
        catch (...) { textures.CleanupScripts(); throw; }
        textures.CleanupScripts();
        return reinterpret_cast<RwTexture*>(texture);
    };
    // The original PalTextureDict renames these SDK textures to decimal CRCs.
    // Compare actual resource identities, not the post-cache debug name.
    auto* expected_sleep=resolve("PALActor\\101\\101_by");
    auto* expected_awake=resolve("PALActor\\101\\"+original_name);
    if (!expected_sleep || !expected_awake || expected_sleep==expected_awake)
        throw std::runtime_error("actual NPC expression/default resources are not distinct");
    BaseObjectState image{};
    {
        NativeNpcMaterialBinding binding(image,actor,"101");
        if (!image.Read(8u) || !image.Read(0x544u) ||
            image.Read(0x550u)!=reinterpret_cast<std::uint32_t>(material))
            throw std::runtime_error("NPC default material metadata missing");
        if (BaseObjectMaterialTexture::SetFace(image,"101_by",textures)!=1u ||
            RpMaterialGetTexture(material)!=expected_sleep)
            throw std::runtime_error("actual NPC emotion texture did not change");
        if (BaseObjectMaterialTexture::ApplyFace(image,textures)!=1u ||
            RpMaterialGetTexture(material)!=expected_awake)
            throw std::runtime_error("actual NPC default texture was not restored");
    }
    for (const auto field : {8u,0x544u,0x548u,0x54cu,0x550u,0x554u,0x558u,0x55cu,0x560u})
        if (image.Read(field)) throw std::runtime_error("NPC material binding left borrowed fields");
    std::array<std::uint32_t,4> enabled{};
    for (unsigned i=0;i<4;++i) enabled[i]=players.State().slots[i].base.Read(44u);
    const auto world=shadow.ReadWord(shadow.ArenaManager()+0x10u);
    struct RestoreShadow {
        NativePlayerManagerSession& players; std::array<std::uint32_t,4> enabled; std::uint32_t world;
        ~RestoreShadow() {
            for (unsigned i=0;i<4;++i)
                (void)NativeActorShadowBinding::Enable(
                    reinterpret_cast<std::uint32_t>(&players.State().slots[i].base),enabled[i],world);
        }
    } restore_shadow{players,enabled,world};
    for (const auto flag : {0u,1u}) {
        (void)ScriptShadowControl::Enable(static_cast<std::uint8_t>(flag),shadow);
        unsigned bound{};
        for (const auto& slot : players.State().slots) {
            if (!slot.base.Read(0x30u) || !slot.base.Read(0x34u)) continue;
            ++bound;
            if (slot.base.Read(44u)!=flag) throw std::runtime_error("real player shadow toggle mismatch");
        }
        if (!bound) throw std::runtime_error("shadow fixture has no actual resource binding");
    }
}
}
