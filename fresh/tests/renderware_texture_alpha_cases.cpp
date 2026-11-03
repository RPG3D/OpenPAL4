#include "renderware_texture_alpha_cases.h"
#include <d3d9.h>
#include <rwcore.h>
#include <rpworld.h>
#include <rpanisot.h>
#include <array>
#include <bit>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Set(RwRenderState state,RwUInt32 value){Require(RwRenderStateSet(state,reinterpret_cast<void*>(value)),"RW state set");}
RwUInt32 Get(RwRenderState state){RwUInt32 v{};Require(RwRenderStateGet(state,&v),"RW state get");return v;}
RwUInt32 Desired(RwUInt32 state){RwUInt32 v{};RwD3D9GetRenderState(state,&v);return v;}
DWORD Actual(IDirect3DDevice9* d,D3DRENDERSTATETYPE state){DWORD v{};Require(SUCCEEDED(d->GetRenderState(state,&v)),"device state");return v;}
DWORD Sampler(IDirect3DDevice9* d,DWORD stage,D3DSAMPLERSTATETYPE type){DWORD v{};Require(SUCCEEDED(d->GetSamplerState(stage,type,&v)),"device sampler");return v;}
struct Texture {
    RwTexture* value{};
    explicit Texture(int format=rwRASTERFORMAT8888){auto* r=RwRasterCreate(4,4,0,rwRASTERTYPETEXTURE|format);Require(r,"texture raster");value=RwTextureCreate(r);Require(value,"texture create");}
    ~Texture(){RwD3D9SetTexture(nullptr,0);RwD3D9SetTexture(nullptr,1);RwTextureDestroy(value);}
    Texture(const Texture&)=delete;
};
void Alpha(){
    auto* d=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());Texture alpha,opaque(rwRASTERFORMAT888);
    Set(rwRENDERSTATEVERTEXALPHAENABLE,0);Set(rwRENDERSTATEALPHATESTFUNCTION,rwALPHATESTFUNCTIONGREATER);
    Require(RwD3D9SetTexture(alpha.value,1),"alpha stage1");_rwD3D9RenderStateFlushCache();
    Require(Actual(d,D3DRS_ALPHABLENDENABLE)==0,"stage1 alpha does not enable blend");
    Require(RwD3D9SetTexture(alpha.value,0),"alpha stage0");_rwD3D9RenderStateFlushCache();
    Require(Get(rwRENDERSTATEVERTEXALPHAENABLE)==0 && Actual(d,D3DRS_ALPHABLENDENABLE)==1 && Actual(d,D3DRS_ALPHATESTENABLE)==1,"raster alpha enables device while vertex preference remains false");
    Set(rwRENDERSTATEVERTEXALPHAENABLE,1);RwD3D9SetTexture(nullptr,0);_rwD3D9RenderStateFlushCache();
    Require(Actual(d,D3DRS_ALPHABLENDENABLE)==1,"vertex alpha keeps blend after unbind");
    RwD3D9SetTexture(alpha.value,0);Set(rwRENDERSTATEVERTEXALPHAENABLE,0);_rwD3D9RenderStateFlushCache();
    Require(Actual(d,D3DRS_ALPHABLENDENABLE)==1,"raster alpha keeps blend after vertex disable");
    Set(rwRENDERSTATEALPHATESTFUNCTION,rwALPHATESTFUNCTIONALWAYS);_rwD3D9RenderStateFlushCache();
    Require(Actual(d,D3DRS_ALPHABLENDENABLE)==1 && Actual(d,D3DRS_ALPHATESTENABLE)==0,"ALWAYS disables test without disabling blend");
    RwD3D9SetTexture(opaque.value,0);_rwD3D9RenderStateFlushCache();
    Require(Actual(d,D3DRS_ALPHABLENDENABLE)==0,"opaque raster and no vertex alpha disable blend");
}
void Sampling(){
    auto* d=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());Texture texture;
    RwTextureSetAddressingU(texture.value,rwTEXTUREADDRESSCLAMP);RwTextureSetAddressingV(texture.value,rwTEXTUREADDRESSMIRROR);
    RwTextureSetFilterMode(texture.value,rwFILTERLINEARMIPLINEAR);Require(RwD3D9SetTexture(texture.value,1),"bind stage1");
    Require(Sampler(d,1,D3DSAMP_ADDRESSU)==D3DTADDRESS_CLAMP && Sampler(d,1,D3DSAMP_ADDRESSV)==D3DTADDRESS_MIRROR,"texture addressing routes to selected stage");
    const std::array<DWORD,6> minmag{1,2,1,2,1,2},mip{0,0,1,1,2,2};
    for(RwUInt32 i=1;i<=6;++i){Set(rwRENDERSTATETEXTUREFILTER,i);Require(Get(rwRENDERSTATETEXTUREFILTER)==i && Sampler(d,0,D3DSAMP_MINFILTER)==minmag[i-1] && Sampler(d,0,D3DSAMP_MAGFILTER)==minmag[i-1] && Sampler(d,0,D3DSAMP_MIPFILTER)==mip[i-1],"six RW filter mappings");}
    Require(Sampler(d,1,D3DSAMP_MINFILTER)==2 && Sampler(d,1,D3DSAMP_MIPFILTER)==2,"global RW filter changes stage0 only");
    IDirect3DBaseTexture9* before{};Require(SUCCEEDED(d->GetTexture(1,&before)) && before,"device texture binding");before->Release();
    Require(RwD3D9SetTexture(nullptr,1),"unbind stage1");IDirect3DBaseTexture9* after{};d->GetTexture(1,&after);Require(!after,"null unbind");
    Require(Sampler(d,1,D3DSAMP_ADDRESSU)==D3DTADDRESS_CLAMP,"unbind preserves sampler preferences");
}
void DepthStencil(){
    auto* d=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    for(RwUInt32 test: {0u,1u})for(RwUInt32 write: {0u,1u}){
        Set(rwRENDERSTATEZTESTENABLE,test);Set(rwRENDERSTATEZWRITEENABLE,write);_rwD3D9RenderStateFlushCache();
        Require(Actual(d,D3DRS_ZENABLE)==(test||write) && Actual(d,D3DRS_ZWRITEENABLE)==write && Actual(d,D3DRS_ZFUNC)==(test?D3DCMP_LESSEQUAL:D3DCMP_ALWAYS),"depth test/write truth table");
    }
    Set(rwRENDERSTATESTENCILENABLE,1);
    for(RwUInt32 operation=1;operation<=8;++operation){
        Set(rwRENDERSTATESTENCILFAIL,operation);Set(rwRENDERSTATESTENCILZFAIL,operation);Set(rwRENDERSTATESTENCILPASS,operation);Set(rwRENDERSTATESTENCILFUNCTION,operation);
        _rwD3D9RenderStateFlushCache();Require(Actual(d,D3DRS_STENCILFAIL)==operation && Actual(d,D3DRS_STENCILZFAIL)==operation && Actual(d,D3DRS_STENCILPASS)==operation && Actual(d,D3DRS_STENCILFUNC)==operation,"stencil operation/function enum mapping");
    }
    Set(rwRENDERSTATESTENCILFUNCTIONREF,0x42);Set(rwRENDERSTATESTENCILFUNCTIONMASK,0x73);Set(rwRENDERSTATESTENCILFUNCTIONWRITEMASK,0x95);
    Set(rwRENDERSTATESRCBLEND,rwBLENDONE);Set(rwRENDERSTATEDESTBLEND,rwBLENDZERO);Set(rwRENDERSTATESHADEMODE,rwSHADEMODEFLAT);Set(rwRENDERSTATECULLMODE,rwCULLMODECULLFRONT);
    _rwD3D9RenderStateFlushCache();Require(Actual(d,D3DRS_STENCILREF)==0x42 && Actual(d,D3DRS_STENCILMASK)==0x73 && Actual(d,D3DRS_STENCILWRITEMASK)==0x95,"stencil scalar fields");
    Require(Actual(d,D3DRS_SRCBLEND)==D3DBLEND_ONE && Actual(d,D3DRS_DESTBLEND)==D3DBLEND_ZERO && Actual(d,D3DRS_SHADEMODE)==D3DSHADE_FLAT && Actual(d,D3DRS_CULLMODE)==D3DCULL_CCW,"blend/shade/cull mappings");
    Require(!RwRenderStateSet(static_cast<RwRenderState>(18),nullptr),"unsupported set");
    Require(!RwRenderStateSet(static_cast<RwRenderState>(5),nullptr) && RwRenderStateSet(static_cast<RwRenderState>(5),reinterpret_cast<void*>(1)),"legacy perspective state reports nonzero without mutation");
}
void AnisotropyMismatch(){
    auto* d=static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Require(RpAnisotGetMaxSupportedMaxAnisotropy()>=2,"anisotropy diagnostic needs level2");Texture texture;
    Require(RpAnisotTextureSetMaxAnisotropy(texture.value,2)==texture.value,"set texture anisotropy");
    RwD3D9SetSamplerState(0,D3DSAMP_MAXANISOTROPY,1);RwD3D9SetSamplerState(1,D3DSAMP_MAXANISOTROPY,1);
    Require(RwD3D9SetTexture(texture.value,1),"bind anisotropic stage1");
    Require(Sampler(d,0,D3DSAMP_MAXANISOTROPY)==2 && Sampler(d,1,D3DSAMP_MAXANISOTROPY)==1,"diagnostic: SDK writes stage0 although target stage1; EXE writes target stage");
    Set(rwRENDERSTATETEXTUREFILTER,rwFILTERNEAREST);
    Require(Sampler(d,0,D3DSAMP_MAXANISOTROPY)==2,"stage0 high-level anisotropy cache remains1 and cannot reset accidental device value");
}
void FogMismatch(){
    auto* camera=RwCameraCreate();auto* frame=RwFrameCreate();Require(camera&&frame,"fog camera");RwCameraSetFrame(camera,frame);
    auto* raster=RwRasterCreate(16,16,0,rwRASTERTYPECAMERATEXTURE|rwRASTERFORMAT8888);Require(raster,"fog raster");RwCameraSetRaster(camera,raster);
    RwCameraSetNearClipPlane(camera,1.0f);RwCameraSetFarClipPlane(camera,23.0f);RwCameraSetFogDistance(camera,7.0f);
    Require(RwCameraBeginUpdate(camera)==camera,"fog camera begin");Set(rwRENDERSTATEFOGTYPE,rwFOGTYPEEXPONENTIAL);
    RwD3D9SetRenderState(D3DRS_FOGSTART,std::bit_cast<RwUInt32>(2.0f));RwD3D9SetRenderState(D3DRS_FOGEND,std::bit_cast<RwUInt32>(50.0f));
    Set(rwRENDERSTATEFOGTYPE,rwFOGTYPELINEAR);
    Require(Desired(D3DRS_FOGSTART)==std::bit_cast<RwUInt32>(7.0f) && Desired(D3DRS_FOGEND)==std::bit_cast<RwUInt32>(23.0f),"diagnostic: SDK LINEAR setter overwrites distances from current camera; EXE setter does not");
    float density=0.375f;Require(RwRenderStateSet(rwRENDERSTATEFOGDENSITY,&density),"fog density pointer");float out{};Require(RwRenderStateGet(rwRENDERSTATEFOGDENSITY,&out)&&out==density,"fog density typed pointer contract");
    Require(RwCameraEndUpdate(camera)==camera,"fog camera end");RwCameraSetFrame(camera,nullptr);RwCameraSetRaster(camera,nullptr);RwCameraDestroy(camera);RwFrameDestroy(frame);RwRasterDestroy(raster);
}
}
void RegisterTextureAlphaPlugins(){Require(RpWorldPluginAttach() && RpAnisotPluginAttach(),"World/Anisot plugins");}
void RunTextureAlphaCase(std::string_view name){
    if(name=="texture_alpha_blending")Alpha();else if(name=="texture_alpha_sampling")Sampling();else if(name=="texture_alpha_depth_stencil")DepthStencil();
    else if(name=="texture_alpha_anisotropy_mismatch")AnisotropyMismatch();else if(name=="texture_alpha_fog_mismatch")FogMismatch();
    else throw std::runtime_error("unknown texture alpha case");
}
}
