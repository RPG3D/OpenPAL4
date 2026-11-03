#pragma once
#include "pal4/portable/effect_document.h"
#include "pal4/portable/scene.h"
namespace pal4::portable {
struct EffectTexture {
    std::string member;
    std::size_t source_bytes{};
    Texture image;
    std::uint32_t rgba_crc32{};
};
class EffectTextures {
public:
    explicit EffectTextures(EffectMemberReader reader);
    std::shared_ptr<const EffectTexture> Load(std::string_view original_path);
    std::size_t Size() const;
private:EffectMemberReader read_;std::map<std::string,std::weak_ptr<const EffectTexture>> textures_;
};
}
