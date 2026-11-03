#include "pal4/portable/image_texture.h"
#include "pal4/portable/dds_texture.h"
#include <lodepng.h>
#include <stdexcept>
namespace pal4::portable {
Texture DecodeOriginalTexture(std::span<const std::uint8_t> bytes,bool dds){
    if(bytes.empty() || bytes.size()>32u*1024*1024)throw std::runtime_error("invalid original texture size");
    if(dds)return DecodeDdsTexture(bytes);
    lodepng::State state;unsigned width{},height{};const auto inspect=lodepng_inspect(&width,&height,&state,bytes.data(),bytes.size());
    if(inspect || !width || !height || width>4096 || height>4096 || std::uint64_t(width)*height*4>64u*1024*1024)throw std::runtime_error("invalid original PNG dimensions");
    Texture image;const auto error=lodepng::decode(image.rgba,width,height,state,bytes.data(),bytes.size());
    if(error)throw std::runtime_error(std::string("original PNG decode failed: ")+lodepng_error_text(error));
    image.width=width;image.height=height;for(std::size_t i=3;i<image.rgba.size();i+=4)if(image.rgba[i]!=255){image.alpha=true;break;}return image;
}
}
