#include "pal4/portable/dds_texture.h"
#include <iostream>
#include <stdexcept>
using namespace pal4::portable;
namespace {
void Check(bool v){if(!v)throw std::runtime_error("DDS texture contract failed");}
void Word(std::vector<std::uint8_t>& b,unsigned at,unsigned value){for(unsigned i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(value>>(8*i));}
std::vector<std::uint8_t> Dds(unsigned format,unsigned bytes,unsigned w=2,unsigned h=2){std::vector<std::uint8_t> b(128+bytes);Word(b,0,0x20534444);Word(b,4,124);Word(b,12,h);Word(b,16,w);Word(b,76,32);Word(b,80,4);Word(b,84,format);return b;}
template<class F>void Reject(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}Check(failed);}
}
int main(){try{
    auto bc1=Dds(0x31545844,8);bc1[128]=0;bc1[129]=0xf8;auto image=DecodeDdsTexture(bc1);Check(image.width==2 && image.height==2 && !image.alpha);for(unsigned i=0;i<4;++i)Check(image.rgba[i*4]==255 && image.rgba[i*4+1]==0 && image.rgba[i*4+3]==255);
    auto bc2=Dds(0x33545844,16);for(unsigned i=0;i<8;++i)bc2[128+i]=0x88;bc2[137]=0xf8;image=DecodeDdsTexture(bc2);Check(image.alpha && image.rgba[0]==255 && image.rgba[3]==136);
    auto bc3=Dds(0x35545844,16);bc3[128]=255;bc3[137]=0xf8;image=DecodeDdsTexture(bc3);Check(!image.alpha && image.rgba[0]==255 && image.rgba[3]==255);
    auto partial=Dds(0x31545844,16,5,3);partial[129]=0xf8;partial[136]=0xe0;partial[137]=7;image=DecodeDdsTexture(partial);Check(image.rgba.size()==60 && image.rgba[16]==0 && image.rgba[17]==255 && image.rgba[56]==0 && image.rgba[57]==255);
    auto raw=Dds(0,4,1,1);Word(raw,80,0x41);Word(raw,88,32);Word(raw,92,0xff);Word(raw,96,0xff00);Word(raw,100,0xff0000);Word(raw,104,0xff000000);Word(raw,128,0x80402010);image=DecodeDdsTexture(raw);Check(image.rgba==std::vector<std::uint8_t>{16,32,64,128});
    auto truncated=bc3;truncated.pop_back();Reject([&]{DecodeDdsTexture(truncated);});Word(bc3,84,0);Reject([&]{DecodeDdsTexture(bc3);});Word(bc1,112,0x200);Reject([&]{DecodeDdsTexture(bc1);});Word(raw,16,0);Reject([&]{DecodeDdsTexture(raw);});
    std::cout<<"DDS BC1/BC2/BC3, partial blocks, RGBA masks, alpha and truncation passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
