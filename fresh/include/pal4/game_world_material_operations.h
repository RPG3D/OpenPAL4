#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameWorldMaterialFlagRuntime {
public:
    virtual ~IGameWorldMaterialFlagRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GeometryOfAtomic(
        std::uint32_t atomic) = 0;
    [[nodiscard]] virtual std::uint32_t GeometryFlags(
        std::uint32_t geometry) = 0;
    virtual void SetGeometryFlags(std::uint32_t geometry,
                                  std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::int32_t MaterialCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialAt(
        std::uint32_t geometry, std::int32_t index) = 0;
    virtual void SetMaterialFlags(std::uint32_t material,
                                  std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllClumpAtomics(
        std::uint32_t clump, std::uint32_t flags) = 0;
};

enum class GameWorldMaterialTraversal : std::uint8_t {
    SetTexture,
    GetTexture,
};

class IGameWorldMaterialTextureRuntime {
public:
    virtual ~IGameWorldMaterialTextureRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GeometryOfAtomic(
        std::uint32_t atomic) = 0;
    virtual void ForAllMaterials(
        std::uint32_t geometry, GameWorldMaterialTraversal traversal,
        std::uint32_t& value) = 0;
    virtual void SetMaterialTexture(std::uint32_t material,
                                    std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialTexture(
        std::uint32_t material) = 0;
    [[nodiscard]] virtual std::int32_t MaterialCount(
        std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t MaterialAt(
        std::uint32_t geometry, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t CopyTexturePath(
        std::uint32_t text_address, std::string& destination,
        std::uint32_t capacity) = 0;
    [[nodiscard]] virtual bool CompareTexturePath(
        std::string_view candidate, std::string_view actual) = 0;
    virtual void CaptureFirstAtomic(std::uint32_t clump,
                                    std::uint32_t& selected_atomic) = 0;
    virtual void CheckAllAtomicTextures(
        std::uint32_t clump, std::string_view candidate,
        std::uint32_t& match_flag) = 0;
    [[nodiscard]] virtual std::uint32_t SetAllAtomicTextures(
        std::uint32_t clump, std::uint32_t texture) = 0;
};

class GameWorldMaterialOperations final {
public:
    [[nodiscard]] static std::uint32_t SetAtomicFlags(
        std::uint32_t atomic, std::uint32_t flags,
        IGameWorldMaterialFlagRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetClumpFlags(
        std::uint32_t clump, std::uint32_t flags,
        IGameWorldMaterialFlagRuntime& runtime);
    [[nodiscard]] static std::uint32_t CaptureAtomic(
        std::uint32_t atomic, std::uint32_t& selected_atomic);
    [[nodiscard]] static std::uint32_t SetTexture(
        std::uint32_t material, std::uint32_t texture,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAtomicTexture(
        std::uint32_t atomic, std::uint32_t texture,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t CaptureTexture(
        std::uint32_t material, std::uint32_t& texture,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAtomicTexturePath(
        std::uint32_t atomic, std::string& destination,
        std::uint32_t capacity,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckAtomicTexturePath(
        std::uint32_t atomic, std::uint32_t& match_flag,
        std::string_view candidate,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckClumpTexturePath(
        std::uint32_t clump, std::uint32_t texture,
        IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t CheckClumpTexturePathOnly(
        std::uint32_t clump, IGameWorldMaterialTextureRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetClumpTexture(
        std::uint32_t clump, std::uint32_t texture,
        IGameWorldMaterialTextureRuntime& runtime);
};

}  // namespace pal4
