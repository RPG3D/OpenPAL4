#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>
#include "pal4/rw_plugin_8450_native.h"
#include "pal4/native_rw_plugin_8450_processor.h"
#include "pal4/native_game_vector_math.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>

namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
int observed_destructors{};
void* ObserveAtomicDestructor(void* p,int,int){
    std::array<unsigned,3> words;std::memcpy(words.data(),static_cast<char*>(p)+pal4::NativeRwPlugin8450Lifecycle::State().atomic_offset,12);
    Check(words==std::array<unsigned,3>{0,0,1},"game destructor restores all three words before earlier observer destructor");
    ++observed_destructors;return p;
}
struct Session {
    bool initialized{},opened{},started{};HWND window{};
    ~Session(){if(started)RwEngineStop();if(opened)RwEngineClose();if(initialized)RwEngineTerm();if(window)DestroyWindow(window);}
    void Open(bool pools){
        Check(RwEngineInit(nullptr,pools?rwENGINEINITFREELISTS:rwENGINEINITNOFREELISTS,0),"engine init");initialized=true;
        Check(RpWorldPluginAttach(),"world plugin");
        Check(RpAtomicRegisterPlugin(0,0x12349999,nullptr,ObserveAtomicDestructor,nullptr)>=0,"earlier destructor observer");
        Check(pal4::NativeRwPlugin8450Lifecycle::Attach(),"actual game registration");
        const auto& state=pal4::NativeRwPlugin8450Lifecycle::State();
        Check(state.engine_offset==RwEngineGetPluginOffset(8450)&&state.atomic_offset==RpAtomicGetPluginOffset(8450),"game offsets from correct public registry");
        Check(RpGeometryGetPluginOffset(8450)==-1,"8450 is not a geometry plugin");
        window=pal4::test::CreateTestWindow("STATIC","PAL4 plugin boundary",WS_OVERLAPPEDWINDOW,64,64,nullptr,nullptr,GetModuleHandleA(nullptr),nullptr);Check(window,"hidden window");
        RwEngineOpenParams params{window};Check(RwEngineOpen(&params),"engine open");opened=true;
        Check(RwEngineStart(),"engine start");started=true;
    }
    void Close(){Check(RwEngineStop(),"engine stop");started=false;
        const auto offset=pal4::NativeRwPlugin8450Lifecycle::State().engine_offset;
        std::array<unsigned,2> words;std::memcpy(words.data(),static_cast<char*>(RwEngineInstance)+offset,8);Check(words==std::array<unsigned,2>{0,0},"actual game engine destructor resets threshold/enabled");
        Check(RwEngineClose(),"engine close");opened=false;Check(RwEngineTerm(),"engine term");initialized=false;
    }
};
struct Atomic {RpAtomic* p{};~Atomic(){if(p)RpAtomicDestroy(p);}};
int rendered_atomics{};
RpAtomic* ObserveRender(RpAtomic* atomic) {++rendered_atomics;return atomic;}
void VerifyDistanceFade() {
    auto* camera=RwCameraCreate();auto* camera_frame=RwFrameCreate();
    auto* raster=RwRasterCreate(64,64,32,rwRASTERTYPECAMERATEXTURE);
    auto* depth=RwRasterCreate(64,64,0,rwRASTERTYPEZBUFFER);
    auto* clump=RpClumpCreate();auto* frame=RwFrameCreate();
    auto* geometry=RpGeometryCreate(3,1,rpGEOMETRYPOSITIONS);
    auto* material=RpMaterialCreate();auto* atomic=RpAtomicCreate();
    Check(camera&&camera_frame&&raster&&depth&&clump&&frame&&geometry&&material&&atomic,"fade probe owners");
    RwCameraSetFrame(camera,camera_frame);RwCameraSetRaster(camera,raster);RwCameraSetZRaster(camera,depth);
    RwCameraSetNearClipPlane(camera,1.0f);RwCameraSetFarClipPlane(camera,2000.0f);
    RpClumpSetFrame(clump,frame);RpAtomicSetFrame(atomic,frame);
    RpGeometryTriangleSetMaterial(geometry,RpGeometryGetTriangles(geometry),material);
    RwSphere sphere{{0,0,0},1};RpMorphTargetSetBoundingSphere(RpGeometryGetMorphTarget(geometry,0),&sphere);
    RpAtomicSetGeometry(atomic,geometry,0);RpClumpAddAtomic(clump,atomic);
    RpGeometryDestroy(geometry);RpMaterialDestroy(material);
    RpAtomicSetRenderCallBack(atomic,ObserveRender);
    pal4::NativeRwPlugin8450Processor::SetDistanceThreshold(1200.0f);
    pal4::NativeRwPlugin8450Processor::ProcessClump(clump);
    rendered_atomics=0;
    const auto draw_at=[&](float distance){RwV3d p{0,0,distance};RwFrameTranslate(frame,&p,rwCOMBINEREPLACE);
        Check(RwCameraBeginUpdate(camera)!=nullptr,"fade probe camera begin");
        RpAtomicRender(atomic);RwCameraEndUpdate(camera);
        if(const auto error=pal4::NativeRwPlugin8450Processor::TakeRenderError())std::rethrow_exception(error);};
    draw_at(100.0f);
    Check(rendered_atomics==1,"100-unit atomic remains visible under1200-unit linear threshold");
    // IDA6634D5..6634EC adds0.5 before truncation. The valid fade input
    // therefore exceeds255 at the inner edge of the50-unit interval.
    for (const float distance : {1150.0f, 1150.05f}) {
        draw_at(distance);
        const auto* color=RpMaterialGetColor(RpGeometryGetMaterial(RpAtomicGetGeometry(atomic),0));
        Check(color->red==255&&color->green==255&&color->blue==255&&color->alpha==255,
              "inner fade boundary truncates255-plus-fraction to opaque white without error");
    }
    // Align to the recovered approximate distance, rather than assuming the
    // table lookup returns the exact Euclidean length at a chosen coordinate.
    pal4::NativeRwPlugin8450Processor::SetDistanceThreshold(
        static_cast<float>(pal4::NativeGameApproximateMagnitude({0,0,1190.0f}))+49.95f);
    draw_at(1190.0f);
    Check(RpMaterialGetColor(RpGeometryGetMaterial(RpAtomicGetGeometry(atomic),0))->alpha==255,
          "fractional255 fade input is valid before CRT truncation");
    pal4::NativeRwPlugin8450Processor::SetDistanceThreshold(1200.0f);
    draw_at(1190.0f);
    const auto alpha=RpMaterialGetColor(RpGeometryGetMaterial(RpAtomicGetGeometry(atomic),0))->alpha;
    Check(rendered_atomics==5&&alpha>0&&alpha<255,"last50 distance units fade material rather than cull squared distance");
    draw_at(1300.0f);Check(rendered_atomics==5,"atomic beyond linear distance threshold is culled");
    pal4::NativeRwPlugin8450Processor::RestoreClump(clump);RpClumpDestroy(clump);
    RwCameraSetFrame(camera,nullptr);RwCameraSetRaster(camera,nullptr);RwCameraSetZRaster(camera,nullptr);
    RwCameraDestroy(camera);RwFrameDestroy(camera_frame);RwRasterDestroy(depth);RwRasterDestroy(raster);
}
std::array<unsigned,3> Words(RpAtomic* p,int offset){std::array<unsigned,3> words;std::memcpy(words.data(),reinterpret_cast<char*>(p)+offset,12);return words;}
void Run(bool pools){
    for(int cycle=0;cycle<2;++cycle){Session s;s.Open(pools);const auto state=pal4::NativeRwPlugin8450Lifecycle::State();
        std::array<unsigned,2> engine;std::memcpy(engine.data(),static_cast<char*>(RwEngineInstance)+state.engine_offset,8);Check(engine==std::array<unsigned,2>{0,1},"actual game engine constructor storage");
        VerifyDistanceFade();
        observed_destructors=0;
        {Atomic source{RpAtomicCreate()};Check(source.p,"atomic create");Check(Words(source.p,state.atomic_offset)==std::array<unsigned,3>{0,0,1},"actual game atomic constructor");
        const std::array<unsigned,3> values{0x11223344,0x55667788,0x99aabbcc};std::memcpy(reinterpret_cast<char*>(source.p)+state.atomic_offset,values.data(),12);
        Atomic clone{RpAtomicClone(source.p)};Check(clone.p&&Words(clone.p,state.atomic_offset)==values,"public clone invokes actual game three-word copy");
        Check(Words(source.p,state.atomic_offset)==values,"clone preserves source");}
        Check(observed_destructors==2,"source and clone both destructed through actual game callback");
        // SDK rejects registration with engine instances; use a separate ID so game offsets stay valid.
        Check(RpAtomicRegisterPlugin(4,0x12345678,nullptr,nullptr,nullptr)==-1,"late registration rejected");s.Close();
    }
}
}
int main(int argc,char** argv){try{Check(argc==2,"one case required");const std::string_view c=argv[1];Check(c=="pooled"||c=="direct","unknown case");Run(c=="pooled");std::puts("native game 8450 lifecycle passed");return 0;}catch(const std::exception& e){std::fprintf(stderr,"8450 failed: %s\n",e.what());return 1;}}
