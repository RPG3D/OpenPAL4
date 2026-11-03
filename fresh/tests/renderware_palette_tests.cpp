#include <windows.h>
#include <rwcore.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

static_assert(sizeof(RwPalQuant)==16392 && sizeof(_rwPalQuantOctNode)==92);
static_assert(sizeof(_rwPalQuantLeafNode)==28 && offsetof(RwPalQuant,root)==16384);
static_assert(std::is_same_v<decltype(&RwPalQuantAddImage),void (__cdecl *)(RwPalQuant*,RwImage*,RwReal)>);
static_assert(std::is_same_v<decltype(&RwPalQuantMatchImage),void (__cdecl *)(RwUInt8*,RwInt32,RwInt32,RwBool,RwPalQuant*,RwImage*)>);
static_assert(std::is_same_v<decltype(&RwPalQuantTerm),void (__cdecl *)(RwPalQuant*)>);
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Allocation{size_t size;RwUInt32 hint;};
std::map<void*,Allocation> allocations;
bool fail_quant{},invalid_free{};
void* Allocate(size_t size,RwUInt32 hint){if(fail_quant&&hint==0x30411){fail_quant=false;return nullptr;}auto* p=std::malloc(size);if(p){allocations[p]={size,hint};std::memset(p,0xcc,size);}return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t size,RwUInt32 hint){if(!p)return Allocate(size,hint);if(!size){Free(p);return nullptr;}auto* q=std::realloc(p,size);if(q){allocations.erase(p);allocations[q]={size,hint};}return q;}
void* Callocate(size_t n,size_t size,RwUInt32 hint){auto* p=Allocate(n*size,hint);if(p)std::memset(p,0,n*size);return p;}
struct Engine{Engine(bool pools){const RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};Check(RwEngineInit(&memory,pools?rwENGINEINITFREELISTS:rwENGINEINITNOFREELISTS,0),"engine memory init");}~Engine(){RwEngineTerm();}};
struct Depth{unsigned old=RwPalQuantGetMaxDepth();explicit Depth(unsigned value){RwPalQuantSetMaxDepth(value);}~Depth(){RwPalQuantSetMaxDepth(old);}};
struct Quant{
    RwPalQuant q{};bool active{};
    Quant(){Check(RwPalQuantInit(&q),"quant init");active=true;}
    void Close(){auto* pool=q.cubefreelist;RwPalQuantTerm(&q);active=false;Check(!q.root&&q.cubefreelist==pool,"Term clears root but retains stale pool pointer; not reusable without Init");}
    ~Quant(){if(active)RwPalQuantTerm(&q);}
};
struct View{
    RwImage image{};std::vector<RwUInt8> pixels;std::array<RwRGBA,256> palette{};
    View(int width,int height,int depth,int padding=0):pixels((width*(depth==32?4:1)+padding)*height,0xa9){
        image.width=width;image.height=height;image.depth=depth;image.stride=width*(depth==32?4:1)+padding;image.cpPixels=pixels.data();image.palette=depth==32?nullptr:palette.data();
    }
    void Color(int x,int y,RwRGBA c){std::memcpy(pixels.data()+y*image.stride+x*4,&c,4);}
    RwRGBA Color(int x,int y)const{RwRGBA c;std::memcpy(&c,pixels.data()+y*image.stride+x*4,4);return c;}
    void Index(int x,int y,unsigned i){pixels[y*image.stride+x]=static_cast<RwUInt8>(i);}
};
bool Equal(RwRGBA a,RwRGBA b){return std::memcmp(&a,&b,4)==0;}
using Palette=std::array<RwRGBA,256>;
Palette Sentinel(){Palette p;for(auto& c:p)c={0xee,0xee,0xee,0xee};return p;}
std::vector<RwUInt8> Match(Quant& q,View& source,int stride=0,int depth=8,int packed=0){if(!stride)stride=source.image.width+3;std::vector<RwUInt8> out(stride*source.image.height,0xd7);RwPalQuantMatchImage(out.data(),stride,depth,packed,&q.q,&source.image);return out;}
void Exact(){
    Check(RwPalQuantGetMaxDepth()==6,"SDK and original default depth6");
    for(unsigned depth:{1u,6u,8u}){Depth setting(depth);Quant q;View image(3,2,32,5);
        const std::array<RwRGBA,6> colors{{{0,0,0,0},{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,255,255},{255,0,0,0}}};
        for(int i=0;i<6;++i)image.Color(i%3,i/3,colors[i]);auto input=image.pixels;
        RwPalQuantAddImage(&q.q,&image.image,1);auto palette=Sentinel();Check(RwPalQuantResolvePalette(palette.data(),16,&q.q)==6,"RGBA distinct colors including transparent RGB retained");auto out=Match(q,image,6);
        for(int i=0;i<6;++i)Check(out[(i/3)*6+i%3]<6&&Equal(palette[out[(i/3)*6+i%3]],colors[i]),"exact color reconstruction from trained pixels");
        for(int y=0;y<2;++y)for(int x=3;x<6;++x)Check(out[y*6+x]==0xd7,"destination stride padding preserved");
        for(int i=6;i<16;++i)Check(Equal(palette[i],{0,0,0,0}),"unused requested palette slots zeroed");Check(Equal(palette[16],{0xee,0xee,0xee,0xee}),"palette outside maxcols untouched");Check(image.pixels==input,"source bytes and row padding unchanged");q.Close();
    }
}
void Indexed(){
    for(int depth:{4,8}){Quant q;View image(5,2,depth,3);const std::array<RwRGBA,3> colors{{{255,0,0,255},{0,255,0,255},{0,0,255,0}}};for(int i=0;i<3;++i)image.palette[2+i]=colors[i];
        for(int y=0;y<2;++y)for(int x=0;x<5;++x)image.Index(x,y,2+(x+y)%3);
        RwPalQuantAddImage(&q.q,&image.image,1);auto p=Sentinel();Check(RwPalQuantResolvePalette(p.data(),16,&q.q)==3,"4/8-bit image uses byte-per-pixel indexes");auto out=Match(q,image,8);
        for(int y=0;y<2;++y)for(int x=0;x<5;++x)Check(Equal(p[out[y*8+x]],colors[(x+y)%3]),"indexed source resolves through its own palette");
    }
}
void Packed(){
    for(int source_depth:{4,8,32}){Quant q;View image(3,2,source_depth,3);const std::array<RwRGBA,3> colors{{{0,0,0,255},{255,255,255,255},{255,0,0,255}}};
        for(int i=0;i<3;++i)image.palette[i]=colors[i];for(int y=0;y<2;++y)for(int x=0;x<3;++x){if(source_depth==32)image.Color(x,y,colors[(x+y)%3]);else image.Index(x,y,(x+y)%3);}
        RwPalQuantAddImage(&q.q,&image.image,1);Palette palette{};Check(RwPalQuantResolvePalette(palette.data(),16,&q.q)==3,"packed palette");const auto plain=Match(q,image,6);auto packed=Match(q,image,4,4,TRUE);
        for(int y=0;y<2;++y){Check(packed[y*4]==(plain[y*6]|(plain[y*6+1]<<4)),"first pixel low nibble, second high nibble");Check(packed[y*4+1]==(0xd0|plain[y*6+2]),"odd width preserves last high nibble");Check(packed[y*4+2]==0xd7&&packed[y*4+3]==0xd7,"packed row padding untouched");}
        Check(Match(q,image,6,4,2)==plain&&Match(q,image,6,8,TRUE)==plain,"packing requires depth4 and packed exactly1");
    }
}
void Weighted(){
    Quant q;View red(1,1,32),green(1,1,32);red.Color(0,0,{255,0,0,255});green.Color(0,0,{0,255,0,255});
    RwPalQuantAddImage(&q.q,&red.image,1);RwPalQuantAddImage(&q.q,&green.image,3);Palette palette{};
    Check(RwPalQuantResolvePalette(palette.data(),1,&q.q)==1,"two images share one weighted palette");
    std::printf("weighted representative %u,%u,%u,%u\n",palette[0].red,palette[0].green,palette[0].blue,palette[0].alpha);
    Check(Equal(palette[0],{63,191,0,255}),"weighted mean uses scaled/truncated byte reconstruction");Check(Match(q,red)[0]==0&&Match(q,green)[0]==0,"both training images map to shared palette");
}
double ReductionError(View& image,int budget){Quant q;RwPalQuantAddImage(&q.q,&image.image,1);Palette p{};Check(RwPalQuantResolvePalette(p.data(),budget,&q.q)==budget,"quantizer meets reduced color budget");auto out=Match(q,image,image.image.width);double error=0;
    for(int y=0;y<image.image.height;++y)for(int x=0;x<image.image.width;++x){auto i=out[y*image.image.width+x];Check(i<budget,"assigned index within budget");auto a=image.Color(x,y),b=p[i];for(int d:{int(a.red)-b.red,int(a.green)-b.green,int(a.blue)-b.blue,int(a.alpha)-b.alpha})error+=d*d;}return error;
}
void Reduction(){View image(8,8,32,4);for(int y=0;y<8;++y)for(int x=0;x<8;++x)image.Color(x,y,{static_cast<RwUInt8>(x*32),static_cast<RwUInt8>(y*32),static_cast<RwUInt8>(((x+y)%8)*32),255});const auto e4=ReductionError(image,4),e16=ReductionError(image,16);Check(e16<e4&&e16>0,"larger palette improves actual reconstructed pixels on fixed color grid");}
void RepeatedResolve(){Quant q;View image(1,1,32);image.Color(0,0,{100,50,20,255});RwPalQuantAddImage(&q.q,&image.image,1);Palette first{},second{};
    Check(RwPalQuantResolvePalette(first.data(),16,&q.q)==1&&Equal(first[0],{100,50,20,255}),"first resolve represents original color");
    Check(RwPalQuantResolvePalette(second.data(),16,&q.q)==1,"second resolve result count");std::printf("second representative %u,%u,%u,%u\n",second[0].red,second[0].green,second[0].blue,second[0].alpha);
    Check(Equal(second[0],{200,98,40,255}),"resolve mutates statistics; repeated call adds coarse components again");
}
// SEH probes contain no C++ objects requiring unwinding. Each diagnostic runs in
// its own CTest process; no product exception handler or allocation workaround.
DWORD ProbeInit(RwPalQuant* q){__try{(void)RwPalQuantInit(q);return 0;}__except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}}
DWORD ProbeAdd(RwPalQuant* q,RwImage* image){__try{RwPalQuantAddImage(q,image,1);return 0;}__except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}}
DWORD ProbeMatch(RwPalQuant* q,RwImage* image,RwUInt8* out){__try{RwPalQuantMatchImage(out,1,8,FALSE,q,image);return 0;}__except(EXCEPTION_EXECUTE_HANDLER){return GetExceptionCode();}}
void Untrained(){Quant q;View train(1,1,32),unseen(1,1,32);train.Color(0,0,{0,0,0,255});unseen.Color(0,0,{255,255,255,255});RwPalQuantAddImage(&q.q,&train.image,1);Palette p{};RwPalQuantResolvePalette(p.data(),16,&q.q);RwUInt8 out=0xab;
    Check(ProbeMatch(&q.q,&unseen.image,&out)==EXCEPTION_ACCESS_VIOLATION&&out==0xab,"matching untrained tree branch faults; no nearest-color fallback");
}
void InitFailure(bool root){RwPalQuant q{};fail_quant=true;Check(ProbeInit(&q)==EXCEPTION_ACCESS_VIOLATION&&!fail_quant,"unchecked quantizer allocation failure faults");
    if(root){Check(q.cubefreelist&&!q.root,"direct root failure retains pool wrapper");RwPalQuantTerm(&q);}else Check(!q.cubefreelist&&!q.root,"pool preallocation failure leaves null pool");
}
void AddFailure(){Quant q;View image(1,1,32);image.Color(0,0,{1,2,3,255});fail_quant=true;Check(ProbeAdd(&q.q,&image.image)==EXCEPTION_ACCESS_VIOLATION&&!fail_quant,"unchecked first branch allocation faults");q.Close();}
}
int main(int argc,char** argv){try{Check(argc==2,"one case required");const std::string_view c=argv[1];{Engine engine(c!="root_failure"&&c!="add_failure");
    if(c=="exact")Exact();else if(c=="indexed")Indexed();else if(c=="packed")Packed();else if(c=="weighted")Weighted();else if(c=="reduction")Reduction();else if(c=="repeated")RepeatedResolve();else if(c=="untrained")Untrained();else if(c=="init_failure")InitFailure(false);else if(c=="root_failure")InitFailure(true);else if(c=="add_failure")AddFailure();else throw std::runtime_error("unknown case");
    }Check(allocations.empty()&&!invalid_free,"quantizer/engine allocations released exactly once after diagnostic cleanup");std::printf("palette %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"palette failed: %s\n",e.what());return 1;}}
