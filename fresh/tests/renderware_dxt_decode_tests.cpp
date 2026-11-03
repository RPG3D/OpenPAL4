#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>

// Test-only probes of pinned d3d9dxttex.obj. These are not public SDK APIs.
// Four stack arguments/caller cleanup and DXT1's consumed EAX are verified in
// original code. DXT3/5 return values are deliberately ignored: their original
// C source return types are not established by the incidental register value.
extern "C" int __cdecl _rwD3D9DecompressDXT1(int,int,void*,const void*);
extern "C" void __cdecl _rwD3D9DecompressDXT3(int,int,void*,const void*);
extern "C" void __cdecl _rwD3D9DecompressDXT5(int,int,void*,const void*);
namespace {
using Byte=std::uint8_t;
using Block=std::array<Byte,16>;
struct Pixel {Byte r,g,b,a; bool operator==(const Pixel&)const=default;};
static_assert(sizeof(Pixel)==4);
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Color(Byte* p,unsigned c0,unsigned c1,Byte indices){
    p[0]=Byte(c0);p[1]=Byte(c0>>8);p[2]=Byte(c1);p[3]=Byte(c1>>8);
    for(int i=4;i<8;++i)p[i]=indices;
}
void Palette(){
    Block b{};std::array<Pixel,16> out{};
    Color(b.data(),0xf800,0x001f,0xe4);
    Check(!_rwD3D9DecompressDXT1(4,4,out.data(),b.data()),"opaque block reports no transparency");
    const Pixel expected[4]={{248,0,0,255},{0,0,248,255},{165,0,82,255},{82,0,165,255}};
    for(int i=0;i<16;++i)Check(out[i]==expected[i%4],"565 shifts and full-width interpolation");
    Color(b.data(),0x001f,0xf800,0xe4);
    Check(_rwD3D9DecompressDXT1(4,4,out.data(),b.data())==1,"used transparent selector sets flag");
    const Pixel three[4]={{0,0,248,255},{248,0,0,255},{124,0,124,255},{0,0,0,0}};
    for(int i=0;i<16;++i)Check(out[i]==three[i%4],"three-color palette and transparent black");
    Color(b.data(),0x001f,0xf800,0xaa);
    Check(!_rwD3D9DecompressDXT1(4,4,out.data(),b.data()),"endpoint ordering alone does not set alpha flag");
    Color(b.data(),0xffff,0xffff,0xff);
    Check(_rwD3D9DecompressDXT1(4,4,out.data(),b.data())==1&&out[0]==Pixel{0,0,0,0},"equal endpoints use transparent branch");
}
void Layout(){
    std::array<Byte,32> blocks{};std::array<Pixel,64> out{};
    const unsigned colors[4]={0xf800,0x07e0,0x001f,0xffff};
    const Pixel expected[4]={{248,0,0,255},{0,252,0,255},{0,0,248,255},{248,252,248,255}};
    for(int i=0;i<4;++i)Color(blocks.data()+8*i,colors[i],0,0);
    Check(!_rwD3D9DecompressDXT1(8,8,out.data(),blocks.data()),"multi-block alpha flag");
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)Check(out[y*8+x]==expected[(y/4)*2+x/4],"block row and pixel stride");
    Color(blocks.data()+24,0,0,0xff);
    Check(_rwD3D9DecompressDXT1(8,8,out.data(),blocks.data())==1,"alpha flag accumulates across block rows");
}
void Alpha3(){
    Block b{};std::array<Pixel,16> out{};
    for(int i=0;i<8;++i)b[i]=Byte(2*i|((2*i+1)<<4));
    Color(b.data()+8,0xf800,0x001f,0);
    _rwD3D9DecompressDXT3(4,4,out.data(),b.data());
    for(int i=0;i<16;++i)Check(out[i]==Pixel{248,0,0,Byte(i*17)},"DXT3 nibbles expand to full alpha range");
    Color(b.data()+8,0x001f,0xf800,0xff);
    _rwD3D9DecompressDXT3(4,4,out.data(),b.data());
    for(int i=0;i<16;++i)Check(out[i]==Pixel{0,255,255,Byte(i*17)},"legacy reverse-endpoint selector3 produces cyan with explicit alpha");
}
void Alpha5(){
    Block b{};std::array<Pixel,16> out{};std::uint64_t indices=0;
    for(int i=0;i<16;++i)indices|=std::uint64_t(i%8)<<(3*i);
    for(int i=0;i<6;++i)b[2+i]=Byte(indices>>(8*i));
    Color(b.data()+8,0xf800,0x001f,0);
    const Byte expected[3][8]={{210,0,180,150,120,90,60,30},{10,210,50,90,130,170,0,255},{90,90,90,90,90,90,0,255}};
    for(int branch=0;branch<3;++branch){
        b[0]=expected[branch][0];b[1]=expected[branch][1];
        _rwD3D9DecompressDXT5(4,4,out.data(),b.data());
        for(int i=0;i<16;++i)Check(out[i]==Pixel{248,0,0,expected[branch][i%8]},"DXT5 alpha tables and packed indices across byte boundaries");
    }
    Color(b.data()+8,0x001f,0xf800,0xff);
    _rwD3D9DecompressDXT5(4,4,out.data(),b.data());
    Check(out[0]==Pixel{0,255,255,90},"DXT5 reverse-endpoint selector3 also uses cyan");
}
void Partial(){
    Block b{};Color(b.data(),0xf800,0,0);
    const Pixel sentinel{11,22,33,44};std::array<Pixel,32> out;out.fill(sentinel);
    Check(!_rwD3D9DecompressDXT1(6,5,out.data()+1,b.data()),"partial dimensions report opaque");
    for(int y=0;y<5;++y)for(int x=0;x<6;++x)Check(out[1+y*6+x]==(y<4&&x<4?Pixel{248,0,0,255}:sentinel),"partial block rows and columns remain untouched");
    Check(out.front()==sentinel&&out.back()==sentinel,"output guards retained");
    Check(!_rwD3D9DecompressDXT1(4,0,nullptr,nullptr),"zero block rows do not dereference pointers");
    _rwD3D9DecompressDXT3(0,0,nullptr,nullptr);_rwD3D9DecompressDXT5(0,0,nullptr,nullptr);
}
}
int main(int argc,char** argv){
    try {Check(argc==2,"expected case");const std::string_view name=argv[1];
        if(name=="palette")Palette();else if(name=="layout")Layout();else if(name=="alpha3")Alpha3();else if(name=="alpha5")Alpha5();else if(name=="partial")Partial();else throw std::runtime_error("unknown DXT decode case");
        std::puts("pinned SDK DXT decode probe passed");return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
