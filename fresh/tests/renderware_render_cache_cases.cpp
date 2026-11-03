#include "renderware_render_cache_cases.h"
#include "pal4/effect_system_render_state_native.h"
#include <d3d9.h>
// The SDK declares its cache-flush entry only in the D3D-aware driver block.
#include <rwcore.h>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
void Near(float a,float b) { Require(std::fabs(a-b)<0.00001f,"material value mismatch"); }
static_assert(std::is_same_v<decltype(&RwD3D9SetTextureStageState),void (__cdecl *)(RwUInt32,RwUInt32,RwUInt32)>);
static_assert(std::is_same_v<decltype(&RwD3D9SetSamplerState),void (__cdecl *)(RwUInt32,RwUInt32,RwUInt32)>);
static_assert(std::is_same_v<decltype(&RwD3D9SetSurfaceProperties),RwBool (__cdecl *)(const RwSurfaceProperties*,const RwRGBA*,RwUInt32)>);
DWORD Actual(IDirect3DDevice9* device,D3DRENDERSTATETYPE state) {
    DWORD value{};Require(SUCCEEDED(device->GetRenderState(state,&value)),"device render state");return value;
}
RwUInt32 Desired(RwUInt32 state) { RwUInt32 value{};RwD3D9GetRenderState(state,&value);return value; }
DWORD Stage(IDirect3DDevice9* device) {
    DWORD value{};Require(SUCCEEDED(device->GetTextureStageState(0,D3DTSS_COLOROP,&value)),"device texture stage");return value;
}
DWORD Sampler(IDirect3DDevice9* device) {
    DWORD value{};Require(SUCCEEDED(device->GetSamplerState(0,D3DSAMP_ADDRESSU,&value)),"device sampler");return value;
}
void Deferred() {
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0x11223344);_rwD3D9RenderStateFlushCache();
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0x55667788);
    Require(Desired(D3DRS_TEXTUREFACTOR)==0x55667788 && Actual(device,D3DRS_TEXTUREFACTOR)==0x11223344,
            "setter queues desired value without touching device");
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0x11223344);_rwD3D9RenderStateFlushCache();
    Require(Actual(device,D3DRS_TEXTUREFACTOR)==0x11223344,"restored-before-flush changes coalesce");
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0xaabbccdd);_rwD3D9RenderStateFlushCache();
    Require(Actual(device,D3DRS_TEXTUREFACTOR)==0xaabbccdd,"flush submits latest desired value");
    Require(SUCCEEDED(device->SetRenderState(D3DRS_TEXTUREFACTOR,0x12345678)),"external device mutation");
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0xaabbccdd);_rwD3D9RenderStateFlushCache();
    Require(Desired(D3DRS_TEXTUREFACTOR)==0xaabbccdd && Actual(device,D3DRS_TEXTUREFACTOR)==0x12345678,
            "cache hit does not repair external mutation");
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0xaabbccde);_rwD3D9RenderStateFlushCache();
    Require(Actual(device,D3DRS_TEXTUREFACTOR)==0xaabbccde,"changed value is submitted");
}
void StageAndSampler() {
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    RwD3D9SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2);_rwD3D9RenderStateFlushCache();
    RwD3D9SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);
    RwUInt32 cached{};RwD3D9GetTextureStageState(0,D3DTSS_COLOROP,&cached);
    Require(cached==D3DTOP_MODULATE && Stage(device)==D3DTOP_SELECTARG2,"stage setter deferred");
    _rwD3D9RenderStateFlushCache();Require(Stage(device)==D3DTOP_MODULATE,"stage flush");
    RwD3D9SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    Require(Sampler(device)==D3DTADDRESS_CLAMP,"sampler setter immediate");
    Require(SUCCEEDED(device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP)),"external sampler change");
    RwD3D9SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    RwD3D9GetSamplerState(0,D3DSAMP_ADDRESSU,&cached);
    Require(cached==D3DTADDRESS_CLAMP && Sampler(device)==D3DTADDRESS_WRAP,"sampler cache hit keeps divergence");
    Require(RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU,reinterpret_cast<void*>(rwTEXTUREADDRESSWRAP)),"U address");
    Require(RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV,reinterpret_cast<void*>(rwTEXTUREADDRESSCLAMP)),"V address");
    cached=0xabcdef12;
    Require(!RwRenderStateGet(rwRENDERSTATETEXTUREADDRESS,&cached) && cached==0,"combined address mismatch returns false and zero");
    cached=0xabcdef12;
    Require(!RwRenderStateGet(static_cast<RwRenderState>(5),&cached) && cached==0xabcdef12,"unsupported state leaves output unchanged");
}
void Material() {
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    RwSurfaceProperties surface{0.0f,0.2f,0.75f};RwRGBA color{128,64,32,192};
    for(RwUInt32 flags: {0u,8u,64u,72u}) {
        Require(RwD3D9SetSurfaceProperties(&surface,&color,flags),"surface material");
        D3DMATERIAL9 material{};Require(SUCCEEDED(device->GetMaterial(&material)),"get material");
        const bool modulate=(flags&64)!=0;
        Near(material.Diffuse.r,modulate?0.75f*128/255:0.75f);
        Near(material.Diffuse.g,modulate?0.75f*64/255:0.75f);
        Near(material.Diffuse.b,modulate?0.75f*32/255:0.75f);
        Near(material.Diffuse.a,modulate?192.0f/255:1.0f);
        Near(material.Ambient.r,0);Near(material.Emissive.r,0);Near(material.Specular.r,0);Near(material.Power,0);
        _rwD3D9RenderStateFlushCache();
        Require(Actual(device,D3DRS_COLORVERTEX)==((flags&8)?1u:0u),"prelight color-vertex selection");
        Require(Actual(device,D3DRS_AMBIENTMATERIALSOURCE)==(flags==72?D3DMCS_COLOR1:D3DMCS_MATERIAL),"ambient source");
        Require(Actual(device,D3DRS_EMISSIVEMATERIALSOURCE)==(flags==8?D3DMCS_COLOR1:D3DMCS_MATERIAL),"emissive source");
        Require(Actual(device,D3DRS_AMBIENT)==(flags==72?D3DCOLOR_ARGB(192,128,64,32):0xffffffff),"ambient packed color");
    }
    D3DMATERIAL9 external{};external.Diffuse={0.13f,0.17f,0.19f,0.23f};
    Require(SUCCEEDED(device->SetMaterial(&external)),"external material");surface.specular=99;
    Require(RwD3D9SetSurfaceProperties(&surface,&color,72|0x100),"specular and irrelevant flag cache hit");
    D3DMATERIAL9 actual{};device->GetMaterial(&actual);Near(actual.Diffuse.r,0.13f);
    surface.diffuse=0.5f;Require(RwD3D9SetSurfaceProperties(&surface,&color,72),"diffuse invalidates material cache");
    device->GetMaterial(&actual);Near(actual.Diffuse.r,0.5f*128/255);
    color={255,255,255,255};Require(RwD3D9SetSurfaceProperties(&surface,&color,72),"white bypass");
    _rwD3D9RenderStateFlushCache();Require(Actual(device,D3DRS_EMISSIVEMATERIALSOURCE)==D3DMCS_COLOR1,"white modulation takes unmodulated prelight path");
}
struct Camera {
    RwCamera* value=RwCameraCreate();RwFrame* frame=RwFrameCreate();
    explicit Camera(HWND window) {
        Require(value&&frame,"reset camera");RwCameraSetFrame(value,frame);RECT rect{};GetClientRect(window,&rect);
        RwCameraSetRaster(value,RwRasterCreate(rect.right,rect.bottom,0,rwRASTERTYPECAMERA));
        RwCameraSetZRaster(value,RwRasterCreate(rect.right,rect.bottom,0,rwRASTERTYPEZBUFFER));
        Require(RwCameraGetRaster(value)&&RwCameraGetZRaster(value),"reset rasters");
    }
    ~Camera(){auto* color=RwCameraGetRaster(value);auto* depth=RwCameraGetZRaster(value);
        RwCameraSetRaster(value,nullptr);RwCameraSetZRaster(value,nullptr);RwCameraSetFrame(value,nullptr);
        RwCameraDestroy(value);RwFrameDestroy(frame);RwRasterDestroy(color);RwRasterDestroy(depth);}
};
void Reset(HWND window) {
    auto* device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());Camera camera(window);
    RwRGBA clear{0,0,0,255};Require(RwCameraClear(camera.value,&clear,rwCAMERACLEARIMAGE|rwCAMERACLEARZ),"initial clear");
    Require(RwRenderStateSet(rwRENDERSTATESRCBLEND,reinterpret_cast<void*>(rwBLENDONE)),"blend before reset");
    Require(RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,nullptr) && RwRenderStateSet(rwRENDERSTATEZTESTENABLE,nullptr),"depth before reset");
    Require(RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU,reinterpret_cast<void*>(rwTEXTUREADDRESSCLAMP)),"RW address before reset");
    RwD3D9SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_MIRROR);
    RwD3D9SetRenderState(D3DRS_TEXTUREFACTOR,0x12345678);
    RwD3D9SetRenderState(D3DRS_FOGSTART,std::bit_cast<RwUInt32>(3.25f));
    RwD3D9SetRenderState(D3DRS_FOGEND,std::bit_cast<RwUInt32>(27.0f));
    RwD3D9SetRenderState(D3DRS_ALPHAREF,73);
    RwD3D9SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);_rwD3D9RenderStateFlushCache();
    Require(SetWindowPos(window,nullptr,0,0,240,180,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE),"resize");
    Require(RwCameraClear(camera.value,&clear,rwCAMERACLEARIMAGE|rwCAMERACLEARZ),"reset clear");
    Require(Actual(device,D3DRS_SRCBLEND)==D3DBLEND_ONE && Actual(device,D3DRS_ZWRITEENABLE)==0 && Actual(device,D3DRS_ZENABLE)==0,"RW state restored");
    Require(Actual(device,D3DRS_TEXTUREFACTOR)==0xffffffff && Stage(device)==D3DTOP_SELECTARG2 && Sampler(device)==D3DTADDRESS_CLAMP,"raw state reset while RW sampler preference restored");
    Require(Actual(device,D3DRS_FOGSTART)==std::bit_cast<RwUInt32>(3.25f) && Actual(device,D3DRS_FOGEND)==std::bit_cast<RwUInt32>(27.0f) && Actual(device,D3DRS_ALPHAREF)==73,"three explicit raw values preserved through reset");
}
struct GameQueue final : IEffectSystemRenderQueueRuntime {
    bool enabled=true,present=true;int calls=0;IDirect3DDevice9* device{};
    std::uint8_t ReadByte(std::uint32_t address) override {Require(address==0x1004,"game enable address");return enabled;}
    std::uint32_t ReadWord(std::uint32_t address) override {Require(address==0x10ac,"game object address");return present?1:0;}
    void SortAtomicGeometries(std::uint32_t data) override {
        Require(data==0x1034,"game queue address");++calls;
        Require(Desired(D3DRS_ALPHABLENDENABLE)==1 && Desired(D3DRS_SRCBLEND)==D3DBLEND_SRCALPHA && Desired(D3DRS_DESTBLEND)==D3DBLEND_INVSRCALPHA,"temporary game blend cache");
        Require(Actual(device,D3DRS_ALPHABLENDENABLE)==0,"game does not eagerly flush before dispatch");
        _rwD3D9RenderStateFlushCache();
        Require(Actual(device,D3DRS_ALPHABLENDENABLE)==1 && Actual(device,D3DRS_ZWRITEENABLE)==1,"queue draw flush sees temporary states");
    }
};
void Game() {
    GameQueue queue;queue.device=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    NativeEffectSystemRenderStateRuntime runtime(queue);
    const std::array<RwUInt32,5> states{27,19,20,7,14},prior{0,D3DBLEND_ONE,D3DBLEND_ZERO,0,0};
    for(size_t i=0;i<states.size();++i)runtime.SetRenderState(states[i],prior[i]);_rwD3D9RenderStateFlushCache();
    queue.enabled=false;EffectSystem::SetRenderStates(0x1000,runtime);queue.enabled=true;queue.present=false;
    EffectSystem::SetRenderStates(0x1000,runtime);Require(queue.calls==0,"game gate suppresses queue");
    queue.present=true;EffectSystem::SetRenderStates(0x1000,runtime);Require(queue.calls==1,"game queue called once");
    for(size_t i=0;i<states.size();++i)Require(runtime.GetRenderState(states[i])==prior[i],"game restores all desired values");
    Require(Actual(queue.device,D3DRS_ALPHABLENDENABLE)==1,"restoration stays deferred");
    _rwD3D9RenderStateFlushCache();
    for(size_t i=0;i<states.size();++i)Require(Actual(queue.device,static_cast<D3DRENDERSTATETYPE>(states[i]))==prior[i],"later flush restores device state");
}
}
void RunRenderCacheCase(std::string_view name,void* window) {
    if(name=="render_cache_deferred")Deferred();else if(name=="render_cache_stage_sampler")StageAndSampler();
    else if(name=="render_cache_material")Material();else if(name=="render_cache_reset")Reset(static_cast<HWND>(window));
    else if(name=="render_cache_game")Game();else throw std::runtime_error("unknown render cache case");
}
}
