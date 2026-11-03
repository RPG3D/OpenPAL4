#pragma once
#include "pal4/script_npc_material.h"
#include "pal4/base_object_lifecycle.h"
#include <array>
#include <string>

namespace pal4 {
class NativeAnimatedActorAsset;
class NativeArenaNpcSceneLoader;
class IBaseObjectMaterialRuntime;

// Scoped borrowed material metadata; must retire before the NPC image/clump.
class NativeNpcMaterialBinding final {
public:
    NativeNpcMaterialBinding(BaseObjectState&, const NativeAnimatedActorAsset&,
                             std::string_view resource);
    ~NativeNpcMaterialBinding();
    NativeNpcMaterialBinding(const NativeNpcMaterialBinding&) = delete;
    NativeNpcMaterialBinding& operator=(const NativeNpcMaterialBinding&) = delete;
private:
    void Restore() noexcept;
    BaseObjectState& state_;
    std::string directory_, face_, leg_;
    std::array<std::uint32_t, 9> saved_{};
};

class NativeScriptNpcMaterialRuntime final : public IScriptNpcMaterialRuntime {
public:
    NativeScriptNpcMaterialRuntime(NativeArenaNpcSceneLoader& npcs,
                                  IBaseObjectMaterialRuntime& textures)
        : npcs_(npcs), textures_(textures) {}
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t NpcManager() override;
    std::uint32_t FindNpc(std::uint32_t, std::uint32_t) override;
    std::uint32_t SetMaterial(std::uint32_t, std::uint32_t) override;
    std::uint32_t ApplyMaterial(std::uint32_t) override;
private:
    BaseObjectState& Object(std::uint32_t);
    NativeArenaNpcSceneLoader& npcs_;
    IBaseObjectMaterialRuntime& textures_;
};
}
