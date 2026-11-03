#pragma once
#include "pal4/rw_texture_dictionary.h"

namespace pal4 {
// Native SDK dictionary lifetime boundary. Handles belong to this Win32 host.
// The game's force-release policy stays in RwTextureDictionary::Cleanup.
class NativeTextureDictionaryRuntime final : public IRwTextureDictionaryRuntime {
public:
    std::uint32_t CreateDictionary() override;
    void ForEachTexture(std::uint32_t dictionary,
        const std::function<std::uint32_t(std::uint32_t)>& visit) override;
    std::uint32_t DestroyDictionary(std::uint32_t dictionary) override;
    std::int32_t TextureReferences(std::uint32_t texture) override;
    void SetTextureReferences(std::uint32_t texture, std::int32_t references) override;
};
}
