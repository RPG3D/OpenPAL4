#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameWorldAssetRuntime {
public:
    virtual ~IGameWorldAssetRuntime() = default;
    virtual void ReallocateImage(std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentDictionary() = 0;
    [[nodiscard]] virtual bool TextureExists(
        std::uint32_t dictionary, std::string_view name) = 0;
    virtual void SelectTexture(std::uint32_t dictionary,
                               std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t CreateDictionary() = 0;
    virtual void SetCurrentDictionary(std::uint32_t dictionary) = 0;
    [[nodiscard]] virtual std::uint32_t OpenStream(
        std::uint32_t kind, std::uint32_t mode,
        std::string_view path) = 0;
    [[nodiscard]] virtual bool FindChunk(std::uint32_t stream,
                                          std::uint32_t chunk_id,
                                          std::uint32_t first,
                                          std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReadClump(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t ReadAtomic(std::uint32_t stream) = 0;
    virtual void DestroyDictionary(std::uint32_t dictionary) = 0;
    virtual void RemoveTexture(std::uint32_t dictionary,
                               std::string_view name) = 0;
    [[nodiscard]] virtual bool AddTexture(
        std::uint32_t dictionary, std::string_view name,
        std::uint32_t created_dictionary) = 0;
    virtual void CloseStream(std::uint32_t stream,
                             std::uint32_t flags) = 0;
};

class GameWorldAssetLoader final {
public:
    [[nodiscard]] static std::uint32_t LoadClump(
        std::string_view path, std::uint32_t image,
        std::uint32_t dictionary, IGameWorldAssetRuntime& runtime);
    [[nodiscard]] static std::uint32_t LoadAtomic(
        std::string_view path, std::uint32_t image,
        std::uint32_t dictionary, IGameWorldAssetRuntime& runtime);
};

}  // namespace pal4
