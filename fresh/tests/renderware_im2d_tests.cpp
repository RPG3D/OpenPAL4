#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <wrl/client.h>
#include "pal4/camera_scene_frame_native.h"
#include "pal4/native_cegui_vertex_runtime.h"
#include "pal4/native_effect_render_runtime.h"
#include "pal4/native_bink_decoder.h"
#include "pal4/native_bink_package_source.h"
#include "pal4/native_bink_frame_presenter.h"
#include "pal4/native_miles_digital_driver.h"
#include "pal4/native_script_movie_player.h"
#include "pal4/native_angelscript_engine.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/native_script_flash_runtime.h"
#include "pal4/pal_screen_effect.h"
#include <array>
#include <bit>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

void VerifyNativeFontConsole(RwCamera* camera);

static_assert(sizeof(RwIm2DVertex)==28 && sizeof(RwImVertexIndex)==2);
static_assert(offsetof(RwIm2DVertex,emissiveColor)==16 && offsetof(RwIm2DVertex,u)==20);
namespace {
using Microsoft::WRL::ComPtr;
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::unordered_set<void*> allocations;
bool invalid_free{};
void* Allocate(size_t n,RwUInt32){auto* p=std::malloc(n);if(p)allocations.insert(p);return p;}
void Free(void* p){if(p&&allocations.erase(p)!=1)invalid_free=true;std::free(p);}
void* Reallocate(void* p,size_t n,RwUInt32 hint){if(!p)return Allocate(n,hint);if(!n){Free(p);return nullptr;}auto* q=std::realloc(p,n);if(q){allocations.erase(p);allocations.insert(q);}return q;}
void* Callocate(size_t n,size_t s,RwUInt32 hint){auto* p=Allocate(n*s,hint);if(p)std::memset(p,0,n*s);return p;}
struct Session {
    HWND window{};bool initialized{},opened{},started{};
    Session(){const RwMemoryFunctions memory{Allocate,Free,Reallocate,Callocate};Check(RwEngineInit(&memory,0,0),"engine init");initialized=true;Check(RpWorldPluginAttach(),"world plugin");
        window=pal4::test::CreateTestWindow("STATIC","PAL4 Im2D",WS_OVERLAPPEDWINDOW,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);Check(window,"hidden window");
        RwEngineOpenParams p{window};Check(RwEngineOpen(&p),"engine open");opened=true;Start();}
    void Start(){Check(RwEngineStart(),"engine start");started=true;}
    void Stop(){Check(RwEngineStop(),"engine stop");started=false;}
    ~Session(){if(started)RwEngineStop();if(opened)RwEngineClose();if(initialized)RwEngineTerm();if(window)DestroyWindow(window);}
};
void Set(RwRenderState state,RwUInt32 value){Check(RwRenderStateSet(state,reinterpret_cast<void*>(value)),"set RW state");}
void Prepare(){
    Set(rwRENDERSTATETEXTURERASTER,0);Set(rwRENDERSTATEZTESTENABLE,FALSE);Set(rwRENDERSTATEZWRITEENABLE,FALSE);
    Set(rwRENDERSTATECULLMODE,rwCULLMODECULLNONE);Set(rwRENDERSTATEVERTEXALPHAENABLE,FALSE);Set(rwRENDERSTATEFOGENABLE,FALSE);
    RwD3D9SetRenderState(D3DRS_STENCILENABLE,FALSE);RwD3D9SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
}
struct Camera {
    RwCamera* value=RwCameraCreate();RwFrame* frame=RwFrameCreate();RwRaster *color{},*depth{},*child{};
    explicit Camera(HWND window,bool sub=false,int size=32){Check(value&&frame,"camera/frame");RwCameraSetFrame(value,frame);
        color=RwRasterCreate(size,size,32,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);depth=RwRasterCreate(size,size,0,rwRASTERTYPEZBUFFER);Check(color&&depth,"camera targets");
        RwCameraSetRaster(value,color);RwCameraSetZRaster(value,depth);RwCameraSetNearClipPlane(value,1);RwCameraSetFarClipPlane(value,10);const RwV2d view{1,1};RwCameraSetViewWindow(value,&view);
        Check(RwCameraBeginUpdate(value)&&RwCameraEndUpdate(value)&&RwCameraShowRaster(value,window,0),"initial presentation policy");
        if(sub){child=RwRasterCreate(0,0,0,rwRASTERTYPECAMERATEXTURE|rwRASTERDONTALLOCATE);Check(child,"child raster");RwRect rect{8,6,16,16};Check(RwRasterSubRaster(child,color,&rect)==child,"subraster");RwCameraSetRaster(value,child);}
    }
    ~Camera(){RwCameraSetFrame(value,nullptr);RwCameraSetRaster(value,nullptr);RwCameraSetZRaster(value,nullptr);RwCameraDestroy(value);RwFrameDestroy(frame);if(child)RwRasterDestroy(child);RwRasterDestroy(color);RwRasterDestroy(depth);}
    void Begin(bool white=false){
        // Clear the root first so pixels outside a child remain observable.
        auto* target=RwCameraGetRaster(value);RwCameraSetRaster(value,color);RwRGBA clear{static_cast<RwUInt8>(white?255:0),static_cast<RwUInt8>(white?255:0),static_cast<RwUInt8>(white?255:0),255};Check(RwCameraClear(value,&clear,rwCAMERACLEARIMAGE|rwCAMERACLEARZ),"clear root");RwCameraSetRaster(value,target);
        Check(RwCameraBeginUpdate(value),"camera begin");Prepare();
    }
    std::vector<RwRGBA> Read(HWND window,bool end=true){if(end)Check(RwCameraEndUpdate(value),"camera end");Check(RwCameraShowRaster(value,window,0),"present");
        auto* image=RwImageCreate(32,32,32);Check(image&&RwImageAllocatePixels(image)&&RwImageSetFromRaster(image,color),"image readback");std::vector<RwRGBA> pixels(1024);
        for(int y=0;y<32;++y)std::memcpy(pixels.data()+32*y,RwImageGetPixels(image)+y*RwImageGetStride(image),128);RwImageDestroy(image);return pixels;
    }
};
constexpr DWORD red=0xffd04020;
std::array<RwIm2DVertex,4> Quad(float size=28,DWORD color=red){return {{{2,2,0.5f,1,color,0,0},{2,size,0.5f,1,color,0,1},{size,2,0.5f,1,color,1,0},{size,size,0.5f,1,color,1,1}}};}
void Color(const std::vector<RwRGBA>& pixels,int x,int y,DWORD expected,int tolerance=0){auto c=pixels[y*32+x];for(auto [a,b]:{std::pair{int(c.red),int((expected>>16)&255)},std::pair{int(c.green),int((expected>>8)&255)},std::pair{int(c.blue),int(expected&255)}})Check(std::abs(a-b)<=tolerance,"pixel RGB");}
IDirect3DDevice9* Device(){return static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());}
void Primitives(HWND window){Camera camera(window);auto q=Quad();auto original=q;
    for(auto primitive:{rwPRIMTYPELINELIST,rwPRIMTYPEPOLYLINE,rwPRIMTYPETRILIST,rwPRIMTYPETRISTRIP,rwPRIMTYPETRIFAN}){
        camera.Begin();auto vertices=q;if(primitive==rwPRIMTYPETRIFAN)vertices={q[0],q[1],q[3],q[2]};
        const int count=primitive==rwPRIMTYPETRILIST?3:4;Check(RwIm2DRenderPrimitive(primitive,vertices.data(),count),"primitive draw");auto out=camera.Read(window);
        if(primitive==rwPRIMTYPELINELIST||primitive==rwPRIMTYPEPOLYLINE)Color(out,2,8,red);else Color(out,8,8,red);
        Color(out,30,30,0);Check(std::memcmp(&q,&original,sizeof(q))==0,"input unchanged");
    }
    camera.Begin();Check(RwIm2DRenderLine(q.data(),0,0,1),"line selects indexes; numVertices unused");auto out=camera.Read(window);Color(out,2,8,red);
    camera.Begin();Check(RwIm2DRenderTriangle(q.data(),0,0,1,2),"triangle selects indexes; numVertices unused");out=camera.Read(window);Color(out,8,8,red);Color(out,25,25,0);
}
void Indexed(HWND window){Camera camera(window);auto* device=Device();auto q=Quad();const auto original=q;
    std::array<RwImVertexIndex,3> expand{2,0,1};camera.Begin();RwD3D9SetIndices(nullptr);Check(RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,expand.data(),3),"expanded indexed draw");
    ComPtr<IDirect3DIndexBuffer9> bound;Check(SUCCEEDED(device->GetIndices(&bound))&&!bound,"small index list expands vertices without binding IB");auto out=camera.Read(window);Color(out,8,8,red);Color(out,25,25,0);
    std::array<RwImVertexIndex,6> indexes{0,1,2,2,1,3};camera.Begin();Check(RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,indexes.data(),6),"true indexed draw");
    Check(SUCCEEDED(device->GetIndices(&bound))&&bound,"longer index list binds IB");D3DINDEXBUFFER_DESC desc{};Check(SUCCEEDED(bound->GetDesc(&desc))&&desc.Size==20000&&desc.Format==D3DFMT_INDEX16,"SDK10000-index capacity");bound.Reset();
    out=camera.Read(window);Color(out,8,8,red);Color(out,25,25,red);Check(std::memcmp(&q,&original,sizeof(q))==0,"indexed input unchanged");
}
void Subraster(HWND window){Camera camera(window,true);auto q=Quad(12);std::array<RwImVertexIndex,6> idx{0,1,2,2,1,3};
    for(int mode=0;mode<4;++mode){camera.Begin();if(mode==0)Check(RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP,q.data(),4),"child primitive");
        else if(mode==1)Check(RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,idx.data(),6),"child indexed");
        else if(mode==2)Check(RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,idx.data(),3),"child expanded");
        else Check(RwIm2DRenderTriangle(q.data(),4,0,1,2),"child triangle");
        auto out=camera.Read(window);Color(out,12,10,red);Color(out,4,4,0);Color(out,22,20,0);
    }
}
void State(HWND window){Camera camera(window);auto* device=Device();auto q=Quad();
    auto* texture=RwRasterCreate(1,1,32,rwRASTERTYPETEXTURE|rwRASTERFORMAT8888);Check(texture,"texture");
    auto* texels=RwRasterLock(texture,0,rwRASTERLOCKWRITE);Check(texels,"texture lock");*reinterpret_cast<DWORD*>(texels)=0xff804020;RwRasterUnlock(texture);
    for(bool textured:{false,true}){camera.Begin();RwD3D9SetRenderState(D3DRS_LIGHTING,TRUE);RwD3D9SetRenderState(D3DRS_CLIPPING,TRUE);Set(rwRENDERSTATETEXTURERASTER,reinterpret_cast<RwUInt32>(textured?texture:nullptr));
        Check(RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP,q.data(),4),"flush draw");DWORD lighting=1,clipping=1,op=0;device->GetRenderState(D3DRS_LIGHTING,&lighting);device->GetRenderState(D3DRS_CLIPPING,&clipping);device->GetTextureStageState(0,D3DTSS_COLOROP,&op);
        Check(!lighting&&!clipping&&op==(textured?D3DTOP_MODULATE:D3DTOP_SELECTARG2),"Im2D establishes fixed function state");
        ComPtr<IDirect3DVertexDeclaration9> declaration;Check(SUCCEEDED(device->GetVertexDeclaration(&declaration))&&declaration,"vertex declaration");std::array<D3DVERTEXELEMENT9,5> elements{};UINT count=5;Check(SUCCEEDED(declaration->GetDeclaration(elements.data(),&count))&&count==4,"three elements plus end");
        Check(elements[0].Offset==0&&elements[0].Type==D3DDECLTYPE_FLOAT4&&elements[0].Usage==D3DDECLUSAGE_POSITIONT&&elements[1].Offset==16&&elements[1].Type==D3DDECLTYPE_D3DCOLOR&&elements[2].Offset==20&&elements[2].Type==D3DDECLTYPE_FLOAT2,"28-byte vertex ABI");
        auto out=camera.Read(window);Color(out,8,8,textured?0xff681004:red,1);
    }
    Set(rwRENDERSTATETEXTURERASTER,0);RwRasterDestroy(texture);
}
void Ring(HWND window){Camera camera(window);auto q=Quad();camera.Begin();
    for(int i=0;i<3500;++i)Check(RwIm2DRenderTriangle(q.data(),4,0,1,2),"vertex ring repeated draw");
    std::vector<RwImVertexIndex> indexes(6000);for(int i=0;i<6000;++i)indexes[i]=i%3;
    for(int i=0;i<3;++i)Check(RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,indexes.data(),int(indexes.size())),"index ring wrap draw");
    auto out=camera.Read(window);Color(out,8,8,red);Color(out,25,25,0);
}
void Cegui(HWND window){
    Camera camera(window);
    pal4::CeguiRectangleRecord rectangle;
    auto coord=[](float value){return std::bit_cast<std::uint32_t>(value);};
    rectangle.WriteDword(0,0);
    rectangle.WriteDword(4,coord(2.0f));
    rectangle.WriteDword(8,coord(28.0f));
    rectangle.WriteDword(12,coord(2.0f));
    rectangle.WriteDword(16,coord(28.0f));
    rectangle.WriteDword(20,coord(0.5f));
    rectangle.WriteDword(24,coord(0.0f));
    rectangle.WriteDword(28,coord(1.0f));
    rectangle.WriteDword(32,coord(0.0f));
    rectangle.WriteDword(36,coord(1.0f));
    for(auto offset:{40u,44u,48u,52u})rectangle.WriteDword(offset,0xff2040d0u);
    std::array<pal4::CeguiRenderVertex,4> vertices{};
    std::array<RwImVertexIndex,6> indices{0,1,2,3,0,2};
    pal4::CeguiRendererState renderer;
    const auto address=[](const void* value){return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(value));};
    renderer.WriteDword(188,address(&rectangle));
    renderer.WriteDword(192,address(&rectangle)+sizeof(rectangle));
    renderer.WriteDword(204,address(vertices.data()));
    renderer.WriteDword(220,address(indices.data()));
    renderer.WriteDword(264,0);
    camera.Begin();
    pal4::NativeCeguiVertexRuntime runtime;
    Check(pal4::CeguiVertexProcessor::ProcessNative(renderer,runtime)!=0,
          "CEGUI rectangle submits through native RenderWare");
    Check(renderer.ReadDword(264)==0&&vertices[0].x==1.5f&&vertices[0].y==1.5f,
          "CEGUI vertex expansion and flush");
    auto out=camera.Read(window);
    Color(out,8,8,red);
    Color(out,30,30,0);
}
void Effect(HWND window){Camera camera(window);pal4::NativeEffectRenderRuntime runtime;pal4::PalScreenEffectState effect{};
    (void)pal4::EffectRenderControl::Initialize(effect.render,32,32);pal4::EffectRenderControl::InitializeTexture(effect.render,runtime);
    Check(effect.render.texture!=0,"create native screen-effect texture");
    pal4::NativeAngelScriptEngine engine;
    constexpr char source[]="void Fade(){giFlashOutBlack(0.1f,true,true);}";
    Check(engine.Raw()->AddScriptSection("effect","effect.cs",source,sizeof(source)-1)>=0&&engine.Raw()->Build("effect")>=0,"build GPU fade script");
    asIScriptContext* context=nullptr;Check(engine.Raw()->CreateContext(&context)>=0&&context,"create GPU fade context");
    pal4::NativeScriptFlashRuntime flash(effect,runtime,*context);
    pal4::NativeGameScriptCallbacks callbacks({&flash.Flash()});engine.SetCallbacks(&callbacks);
    const int function=engine.FunctionId("effect","Fade");
    Check(function>=0&&context->Prepare(function)>=0&&context->Execute()==asEXECUTION_SUSPENDED,"script starts a blocking native fade");
    for(auto [advance,expected]:{std::pair{0.0f,0xffffffu},std::pair{0.05f,0x7f7f7fu},std::pair{0.05f,0x000000u}}){
        if(advance>0.0f)(void)flash.Tick(advance);
        camera.Begin(true);auto* slot=camera.value;
        Check(pal4::EffectRenderControl::Draw(effect.render,reinterpret_cast<std::uint32_t>(&slot),runtime)!=0,"draw scripted native screen effect");
        auto pixels=camera.Read(window);Color(pixels,16,16,expected,2);
    }
    Check(!flash.Waiting()&&context->Execute()==asEXECUTION_FINISHED,"script resumes after GPU fade completes");
    engine.SetCallbacks(nullptr);(void)context->Release();
    Check(pal4::EffectRenderControl::DestroyTexture(effect.render,runtime)!=0&&effect.render.texture==0,"release native screen-effect texture");
}
void Bink(HWND window, const char* root){
    Camera camera(window);
    const std::string installation(root);
    pal4::NativeBinkDecoder decoder((installation+"\\binkw32.dll").c_str());
    pal4::NativeBinkPackageSource source(decoder);
    Check(source.Open((installation+"\\gamedata\\VideoA.cpk").c_str(),"1A.bik"),"open original movie for GPU upload");
    const auto info=decoder.FrameInfo();
    Check(info.width==800&&info.height==600,"original Bink dimensions");
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(info.width)*info.height*4u);
    pal4::NativeBinkFramePresenter presenter;
    Check(presenter.Upload(pixels,info.width,info.height),"upload black control frame");
    camera.Begin();
    auto* black_slot=camera.value;
    Check(presenter.Draw(reinterpret_cast<std::uint32_t>(&black_slot),32u,32u),"draw black control frame");
    for(const auto& pixel:camera.Read(window))
        Check(pixel.red==0u&&pixel.green==0u&&pixel.blue==0u,"black texture stays black on GPU");
    bool visible=false;
    for(int frame=0;frame<40&&!visible;++frame){
        Check(decoder.DecodeFrame(pixels),"decode original movie for GPU");
        Check(presenter.Upload(pixels,info.width,info.height),"upload original Bink pixels to RW texture");
        camera.Begin();
        auto* slot=camera.value;
        Check(presenter.Draw(reinterpret_cast<std::uint32_t>(&slot),32u,32u),"draw original Bink texture");
        const auto output=camera.Read(window);
        for(const auto& pixel:output){
            if(pixel.red!=0u||pixel.green!=0u||pixel.blue!=0u){visible=true;break;}
        }
        if(!visible)Check(decoder.NextFrame(),"advance movie before next GPU frame");
    }
    Check(visible,"original movie reaches nonblack GPU pixels");
    source.Close();
}
void ScriptBink(HWND window, const char* root){
    struct Host final : pal4::INativeMovieSessionHost {
        explicit Host(pal4::NativeMilesDigitalDriver& driver) : miles(driver) {}
        void* DigitalDriver() override { return miles.Handle(); }
        int state_entries{},restores{};
        float MuteGameAudio() override { return 0.75f; }
        void RestoreGameAudio(float volume) override { Check(volume==0.75f,"restore scripted movie audio");++restores; }
        void EnterState(std::uint32_t state) override { Check(state==12u,"scripted movie state 12");++state_entries; }
        void ResetScreenEffect() override {}
        pal4::NativeMilesDigitalDriver& miles;
    };
    pal4::NativeMilesDigitalDriver miles(root);
    Check(miles.Open(),"open shared Miles driver for scripted GPU movie");
    Host host(miles);
    Camera camera(window);
    pal4::NativeBinkFramePresenter presenter;
    pal4::NativeAngelScriptEngine engine;
    constexpr char script[]="void Movie(){giPlayMovie(\"1A.bik\");}";
    Check(engine.Raw()->AddScriptSection("movie_gpu","movie_gpu.cs",script,sizeof(script)-1)>=0&&
          engine.Raw()->Build("movie_gpu")>=0,"compile scripted movie GPU test");
    asIScriptContext* context=nullptr;
    Check(engine.Raw()->CreateContext(&context)>=0&&context,"create scripted movie context");
    pal4::NativeScriptMoviePlayer movie(root,*context,presenter,host);
    pal4::NativeGameScriptCallbacks callbacks({nullptr,nullptr,&movie});
    engine.SetCallbacks(&callbacks);
    const int function=engine.FunctionId("movie_gpu","Movie");
    Check(function>=0&&context->Prepare(function)>=0&&
          context->Execute()==asEXECUTION_SUSPENDED&&movie.Waiting()&&host.state_entries==1,
          "giPlayMovie opens original movie and suspends script");
    bool visible=false;
    for(int frame=0;frame<40&&!visible;++frame){
        Check(!movie.Tick(false)&&movie.FrameReady(),"scripted movie frame tick");
        camera.Begin();auto* slot=camera.value;
        Check(presenter.Draw(reinterpret_cast<std::uint32_t>(&slot),32u,32u),"draw scripted original movie");
        for(const auto& pixel:camera.Read(window)){
            if(pixel.red||pixel.green||pixel.blue){visible=true;break;}
        }
    }
    Check(visible&&movie.Stop()&&host.restores==1&&
          context->Execute()==asEXECUTION_FINISHED,
          "scripted original movie reaches GPU and resumes AngelScript");
    engine.SetCallbacks(nullptr);
    (void)context->Release();
}
void Limit(HWND window){Camera camera(window);auto q=Quad();camera.Begin();std::vector<RwImVertexIndex> indexes(10001,0);
    Check(!RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST,q.data(),4,indexes.data(),int(indexes.size())),"over10000 true indexes rejected");
    Check(RwIm2DRenderTriangle(q.data(),4,0,1,2),"valid draw after index limit rejection");auto out=camera.Read(window);Color(out,8,8,red);Color(out,25,25,0);
}
struct CallbackContext { RwCamera* camera;unsigned calls; };
void __cdecl SceneCallback(std::uint32_t value){auto& context=*reinterpret_cast<CallbackContext*>(value);Check(RwCameraGetCurrentCamera()==context.camera,"game callback runs inside camera update");++context.calls;Prepare();auto q=Quad(28,0xff204060);Check(RwIm2DRenderTriangle(q.data(),4,0,1,2),"game callback draws scene color");}
void Game(HWND window){Camera camera(window);auto* clump=RpClumpCreate();Check(clump,"empty clump");pal4::NativeCameraSceneFrameRuntime runtime;CallbackContext context{camera.value,0};
    for(bool invert:{false,true}){const auto result=pal4::CameraSceneFrame::RenderInternal(reinterpret_cast<std::uint32_t>(camera.value),reinterpret_cast<std::uint32_t>(clump),invert,reinterpret_cast<std::uint32_t>(&SceneCallback),reinterpret_cast<std::uint32_t>(&context),runtime);
        Check(result==reinterpret_cast<std::uint32_t>(camera.value),"game returns input camera");auto out=camera.Read(window,false);Color(out,8,8,invert?0xffdfbf9f:0xff204060,1);Color(out,25,25,invert?0:0xffffff,1);
        if(invert){RwUInt32 src{},dst{},z{},write{};RwRenderStateGet(rwRENDERSTATESRCBLEND,&src);RwRenderStateGet(rwRENDERSTATEDESTBLEND,&dst);RwRenderStateGet(rwRENDERSTATEZTESTENABLE,&z);RwRenderStateGet(rwRENDERSTATEZWRITEENABLE,&write);Check(src==rwBLENDSRCALPHA&&dst==rwBLENDINVSRCALPHA&&z&&write,"game applies fixed post-draw preferences");}
    }
    Check(context.calls==2,"game callback count");RpClumpDestroy(clump);
}
}
int main(int argc,char** argv){try{Check(argc>=2,"one case required");const std::string_view c=argv[1];Check(argc==((c=="bink"||c=="bink_script")?3:2),"case arguments");{Session s;
    if(c=="font_console"){Camera camera(s.window,false,800);VerifyNativeFontConsole(camera.value);}else if(c=="primitives")Primitives(s.window);else if(c=="indexed")Indexed(s.window);else if(c=="subraster")Subraster(s.window);else if(c=="state")State(s.window);else if(c=="ring")Ring(s.window);else if(c=="limit")Limit(s.window);else if(c=="game")Game(s.window);else if(c=="cegui")Cegui(s.window);else if(c=="effect")Effect(s.window);else if(c=="bink")Bink(s.window,argv[2]);else if(c=="bink_script")ScriptBink(s.window,argv[2]);else if(c=="restart"){for(int i=0;i<2;++i){Indexed(s.window);s.Stop();s.Start();}}else throw std::runtime_error("unknown case");
    }Check(allocations.empty()&&!invalid_free,"engine memory released exactly once");std::printf("im2d %s passed\n",argv[1]);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"im2d failed: %s\n",e.what());return 1;}}
