#pragma once
#include "pal4/game_world_material_operations.h"

namespace pal4 {
class NativeWorldMaterialTextureRuntime final : public IGameWorldMaterialTextureRuntime {
public:
    std::uint32_t GeometryOfAtomic(std::uint32_t atomic) override;
    void ForAllMaterials(std::uint32_t geometry,GameWorldMaterialTraversal traversal,std::uint32_t& value) override;
    void SetMaterialTexture(std::uint32_t material,std::uint32_t texture) override;
    std::uint32_t MaterialTexture(std::uint32_t material) override;
    std::int32_t MaterialCount(std::uint32_t geometry) override;
    std::uint32_t MaterialAt(std::uint32_t geometry,std::int32_t index) override;
    std::uint32_t CopyTexturePath(std::uint32_t address,std::string& destination,std::uint32_t capacity) override;
    bool CompareTexturePath(std::string_view candidate,std::string_view actual) override;
    void CaptureFirstAtomic(std::uint32_t clump,std::uint32_t& selected) override;
    void CheckAllAtomicTextures(std::uint32_t clump,std::string_view candidate,std::uint32_t& flag) override;
    std::uint32_t SetAllAtomicTextures(std::uint32_t clump,std::uint32_t texture) override;
};
}  // namespace pal4
