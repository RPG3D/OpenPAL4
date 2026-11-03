#include "pal4/portable/dds_texture.h"
#include <bcdec.h>
#include <array>
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace pal4::portable {
namespace {
unsigned Word(std::span<const std::uint8_t> b,std::size_t offset){unsigned v{};for(unsigned i=0;i<4;++i)v|=unsigned(b[offset+i])<<(8*i);return v;}
void Require(bool v,const char* error){if(!v)throw std::runtime_error(error);}
}
Texture DecodeDdsTexture(std::span<const std::uint8_t> bytes){
    Require(bytes.size()>=128 && Word(bytes,0)==0x20534444 && Word(bytes,4)==124 && Word(bytes,76)==32,"invalid original DDS header");
    Texture image;image.height=Word(bytes,12);image.width=Word(bytes,16);
    Require(image.width && image.height && image.width<=4096 && image.height<=4096 && Word(bytes,24)<=1 && (Word(bytes,112)&0x20fe00)==0,"unsupported DDS dimensions or cube/volume texture");
    const unsigned format=Word(bytes,84),flags=Word(bytes,80);image.rgba.resize(std::size_t(image.width)*image.height*4);
    if(flags&4){
        const unsigned block_bytes=format==0x31545844?8u:16u;
        Require(format==0x31545844 || format==0x33545844 || format==0x35545844,"unsupported original DDS compression");
        const unsigned columns=(image.width+3)/4,rows=(image.height+3)/4;
        Require(std::uint64_t(columns)*rows*block_bytes<=bytes.size()-128,"truncated DDS blocks");
        for(unsigned y=0;y<rows;++y)for(unsigned x=0;x<columns;++x){
            alignas(8) std::array<std::uint8_t,16> compressed{};std::memcpy(compressed.data(),bytes.data()+128+(std::size_t(y)*columns+x)*block_bytes,block_bytes);
            std::array<std::uint32_t,16> decoded{};
            if(format==0x31545844)bcdec_bc1(compressed.data(),decoded.data(),16);else if(format==0x33545844)bcdec_bc2(compressed.data(),decoded.data(),16);else bcdec_bc3(compressed.data(),decoded.data(),16);
            const auto* pixels=reinterpret_cast<const std::uint8_t*>(decoded.data());
            for(unsigned row=0;row<4 && y*4+row<image.height;++row)for(unsigned col=0;col<4 && x*4+col<image.width;++col)
                std::memcpy(image.rgba.data()+(std::size_t(y*4+row)*image.width+x*4+col)*4,pixels+(row*4+col)*4,4);
        }
    }else{
        Require((flags&0x40) && Word(bytes,88)==32,"unsupported uncompressed DDS pixels");
        const std::array<unsigned,4> masks{Word(bytes,92),Word(bytes,96),Word(bytes,100),Word(bytes,104)};
        std::array<unsigned,4> shifts{};
        for(unsigned c=0;c<4;++c){if(!masks[c]){Require(c==3,"missing DDS color mask");continue;}while(((masks[c]>>shifts[c])&1u)==0)++shifts[c];Require((masks[c]>>shifts[c])==255,"unsupported DDS color mask");}
        const std::size_t stride=(Word(bytes,8)&8)?Word(bytes,20):std::size_t(image.width)*4;
        Require(stride>=std::size_t(image.width)*4 && stride<=bytes.size()-128 && std::uint64_t(stride)*image.height<=bytes.size()-128,"truncated DDS rows");
        for(unsigned y=0;y<image.height;++y)for(unsigned x=0;x<image.width;++x){const auto word=Word(bytes,128+y*stride+x*4);for(unsigned c=0;c<4;++c)image.rgba[(std::size_t(y)*image.width+x)*4+c]=masks[c]?std::uint8_t((word&masks[c])>>shifts[c]):255;}
    }
    for(std::size_t i=3;i<image.rgba.size();i+=4)if(image.rgba[i]!=255){image.alpha=true;break;}
    return image;
}
}
