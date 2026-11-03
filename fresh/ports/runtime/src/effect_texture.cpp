#include "pal4/portable/effect_texture.h"
#include "pal4/portable/image_texture.h"
#include <lodepng.h>
#include <algorithm>
#include <stdexcept>
namespace pal4::portable {
EffectTextures::EffectTextures(EffectMemberReader reader):read_(std::move(reader)){if(!read_)throw std::invalid_argument("effect texture reader is not bound");}
std::size_t EffectTextures::Size() const{std::size_t count=0;for(const auto& entry:textures_)if(!entry.second.expired())++count;return count;}
std::shared_ptr<const EffectTexture> EffectTextures::Load(std::string_view original_path){
    std::string path(original_path);std::replace(path.begin(),path.end(),'/','\\');
    constexpr std::string_view prefix="gamedata\\effect\\";
    if(!path.starts_with(prefix))throw std::invalid_argument("effect texture outside original package");
    path.erase(0,prefix.size());
    if(!path.starts_with("tex\\") || path.find("..")!=std::string::npos || path.find('\0')!=std::string::npos || path.find(':')!=std::string::npos || path.ends_with('\\'))throw std::invalid_argument("invalid effect texture member");
    for(auto it=textures_.begin();it!=textures_.end();)if(it->second.expired())it=textures_.erase(it);else ++it;
    if(const auto found=textures_.find(path);found!=textures_.end())if(auto existing=found->second.lock())return existing;
    auto asset=std::make_shared<EffectTexture>();asset->member=path;
    const auto dot=path.find_last_of('.'),slash=path.find_last_of('\\');
    const auto base=path.substr(0,dot!=std::string::npos && dot>slash?dot:path.size());const auto dds=base+".dds",png=base+".png";
    auto source=read_(dds);const bool compressed=source.has_value();
    if(compressed)asset->member=dds;else {source=read_(png);asset->member=png;}
    if(!source || source->size()>32u*1024*1024)throw std::runtime_error("original effect texture is missing or too large: "+path);
    asset->source_bytes=source->size();
    asset->image=DecodeOriginalTexture(*source,compressed);
    asset->rgba_crc32=lodepng_crc32(asset->image.rgba.data(),asset->image.rgba.size());textures_.emplace(path,asset);return asset;
}
}
