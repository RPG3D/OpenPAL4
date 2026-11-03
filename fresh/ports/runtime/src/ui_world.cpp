#include "pal4/portable/ui_world.h"
#include "pal4/portable/bsp_scene.h"
#include "pal4/portable/dff_collision.h"
#include "pal4/portable/image_texture.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <string>

namespace pal4::portable {
UiWorld ReadUiWorld(std::string_view name,const UiWorldMemberReader& read,bool with_animations) {
    if(!read || name.empty() || name.size()>64 || name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")!=name.npos)
        throw std::invalid_argument("invalid UI world request");
    const std::string prefix="uiWorld\\"+std::string(name)+"\\";
    auto texture=[&](std::string_view texture_name){
        if(texture_name.empty() || texture_name.size()>255 || texture_name.find_first_of("\\/:")!=texture_name.npos || texture_name.find("..")!=texture_name.npos)
            throw std::runtime_error("invalid UI world texture name");
        const auto base=prefix+std::string(texture_name);
        auto data=read(base+".dds");if(!data)data=read(base+".png");if(!data)data=read(base);
        if(!data)throw std::runtime_error("missing UI world texture: "+std::string(texture_name));
        const bool dds=data->size()>=4 && (*data)[0]=='D' && (*data)[1]=='D' && (*data)[2]=='S' && (*data)[3]==' ';
        return DecodeOriginalTexture(*data,dds);
    };
    // UI 世界使用环境光；BSP 解析接口需要四个光照参数。
    std::array<std::uint8_t,16> neutral{};
    const auto one=std::bit_cast<std::array<std::uint8_t,4>>(1.0f);
    for(unsigned i=0;i<4;++i)std::copy(one.begin(),one.end(),neutral.begin()+4*i);
    UiWorld result;
    {
        auto bsp=read(prefix+std::string(name)+".bsp");
        if(!bsp)throw std::runtime_error("missing UI world BSP");
        result.world=ReadBspScene(*bsp,neutral,texture);
    }
    if(auto trans=read(prefix+std::string(name)+"_trans.dff"))result.transparent=ReadDffEnvironment(*trans,texture,true);
    if(with_animations){
        if(auto bytes=read(prefix+std::string(name)+".uva"))result.world_animations=std::make_shared<UvAnimationDictionary>(*bytes);
        if(result.transparent)if(auto bytes=read(prefix+std::string(name)+"_trans.uva"))result.transparent_animations=std::make_shared<UvAnimationDictionary>(*bytes);
    }
    return result;
}
}
