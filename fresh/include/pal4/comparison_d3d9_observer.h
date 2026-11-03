#pragma once
#include <d3d9.h>
#include "pal4/comparison_d3d9_shader_state.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace pal4 {
// Shared comparison instrumentation for the original observer and native host.
// One explicit request observes draws until EndScene or device Present. The SDK
// can present through a swap chain, bypassing IDirect3DDevice9::Present.
// All original calls and HRESULTs are forwarded; no render state is set.
class ComparisonD3D9Observer final {
    ComparisonD3D9Observer()=default;
    ComparisonD3D9Observer(const ComparisonD3D9Observer&)=delete;
    ComparisonD3D9Observer& operator=(const ComparisonD3D9Observer&)=delete;
    inline static constexpr std::array render_states{
        D3DRS_ALPHABLENDENABLE,D3DRS_SRCBLEND,D3DRS_DESTBLEND,D3DRS_ALPHATESTENABLE,
        D3DRS_ALPHAFUNC,D3DRS_ALPHAREF,D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,
        D3DRS_CULLMODE,D3DRS_LIGHTING,D3DRS_FOGENABLE,D3DRS_DITHERENABLE,
        D3DRS_SRGBWRITEENABLE,D3DRS_SPECULARENABLE,D3DRS_MULTISAMPLEANTIALIAS};
    inline static constexpr std::array<const char*,15> render_names{
        "alpha_blend","src_blend","dst_blend","alpha_test","alpha_func","alpha_ref",
        "z_enable","z_write","cull","lighting","fog","dither","srgb_write","specular","msaa"};
    inline static constexpr std::array sampler_states{
        D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER,D3DSAMP_ADDRESSU,
        D3DSAMP_ADDRESSV,D3DSAMP_MAXANISOTROPY,D3DSAMP_MIPMAPLODBIAS,D3DSAMP_SRGBTEXTURE};
    inline static constexpr std::array<const char*,8> sampler_names{
        "min","mag","mip","u","v","anisotropy","lod_bias_bits","srgb"};
    struct Draw {
        bool indexed{},read_failed{},vertex_shader{},pixel_shader{};
        UINT primitive{},vertices{},primitives{},stride{};
        DWORD fvf{};
        ComparisonD3D9ShaderState shader_state;
        std::array<DWORD,render_states.size()> render{};
        std::array<std::array<DWORD,sampler_states.size()>,2> samplers{};
        std::array<std::array<UINT,3>,2> textures{};
        std::array<UINT,4> viewport{},target{};
    };
    using Indexed=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,INT,UINT,UINT,UINT,UINT);
    using Primitive=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DPRIMITIVETYPE,UINT,UINT);
    using Present=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*);
    using EndScene=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*);
    IDirect3DDevice9* device_{};
    void** prior_{};
    inline static constexpr std::array<unsigned,4> slots{17,42,81,82};
    std::array<void*,4> methods_{};
    std::vector<Draw> draws_;
    std::filesystem::path path_;
    bool overflow_{},capture_failed_{};
    UINT max_vertex_constants_{};

    static bool Patch(void** slot,void* value) {
        DWORD protection{},ignored{};
        if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection))return false;
        InterlockedExchangePointer(reinterpret_cast<void* volatile*>(slot),value);
        return VirtualProtect(slot,sizeof(void*),protection,&ignored)!=FALSE;
    }
    bool Restore() {
        bool ok=true;
        for(unsigned i=0;i<slots.size();++i)ok=Patch(prior_+slots[i],methods_[i]) && ok;
        return ok;
    }

    void Observe(bool indexed,D3DPRIMITIVETYPE primitive,UINT vertices,UINT primitives) noexcept {
        if(draws_.size()>=5000){overflow_=true;return;}
        if(capture_failed_)return;
        try {
            Draw row;row.indexed=indexed;row.primitive=primitive;
            row.vertices=vertices;row.primitives=primitives;
            const auto check=[&](HRESULT hr){if(FAILED(hr))row.read_failed=true;};
            D3DVIEWPORT9 viewport{};check(device_->GetViewport(&viewport));
            row.viewport={viewport.X,viewport.Y,viewport.Width,viewport.Height};
            IDirect3DSurface9* target{};check(device_->GetRenderTarget(0,&target));
            if(target){
                D3DSURFACE_DESC description{};check(target->GetDesc(&description));
                row.target={description.Width,description.Height,static_cast<UINT>(description.Format),static_cast<UINT>(description.MultiSampleType)};
                target->Release();
            }
            for(std::size_t i=0;i<render_states.size();++i)
                check(device_->GetRenderState(render_states[i],&row.render[i]));
            for(UINT stage=0;stage<2;++stage){
                for(std::size_t i=0;i<sampler_states.size();++i)
                    check(device_->GetSamplerState(stage,sampler_states[i],&row.samplers[stage][i]));
                IDirect3DBaseTexture9* texture{};
                check(device_->GetTexture(stage,&texture));
                if(texture){
                    if(texture->GetType()==D3DRTYPE_TEXTURE){
                        D3DSURFACE_DESC description{};
                        check(static_cast<IDirect3DTexture9*>(texture)->GetLevelDesc(0,&description));
                        row.textures[stage]={description.Width,description.Height,static_cast<UINT>(description.Format)};
                    }
                    texture->Release();
                }
            }
            IDirect3DVertexShader9* vs{};IDirect3DPixelShader9* ps{};
            check(device_->GetVertexShader(&vs));check(device_->GetPixelShader(&ps));
            row.vertex_shader=vs!=nullptr;row.pixel_shader=ps!=nullptr;
            row.shader_state.Read(device_,vs,ps,max_vertex_constants_);
            if(vs)vs->Release();if(ps)ps->Release();
            check(device_->GetFVF(&row.fvf));
            IDirect3DVertexBuffer9* stream{};UINT offset{};
            check(device_->GetStreamSource(0,&stream,&offset,&row.stride));
            if(stream)stream->Release();
            draws_.push_back(row);
        }catch(...){capture_failed_=true;}
    }
    bool WriteFramePixels() noexcept {
        struct Surface {
            IDirect3DSurface9* value{};bool locked{};
            ~Surface(){if(value){if(locked)value->UnlockRect();value->Release();}}
        } target,staging;
        try {
            if(FAILED(device_->GetRenderTarget(0,&target.value)))return false;
            D3DSURFACE_DESC desc{};
            if(FAILED(target.value->GetDesc(&desc)) || desc.MultiSampleType!=D3DMULTISAMPLE_NONE ||
               (desc.Format!=D3DFMT_X8R8G8B8 && desc.Format!=D3DFMT_A8R8G8B8) ||
               !desc.Width || !desc.Height || desc.Width>8192u || desc.Height>8192u)return false;
            if(FAILED(device_->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,
                D3DPOOL_SYSTEMMEM,&staging.value,nullptr)) ||
               FAILED(device_->GetRenderTargetData(target.value,staging.value)))return false;
            D3DLOCKED_RECT pixels{};
            if(FAILED(staging.value->LockRect(&pixels,nullptr,D3DLOCK_READONLY)))return false;
            staging.locked=true;
            BITMAPFILEHEADER file{};BITMAPINFOHEADER info{};
            file.bfType=0x4d42u;file.bfOffBits=sizeof(file)+sizeof(info);
            file.bfSize=file.bfOffBits+desc.Width*desc.Height*4u;
            info.biSize=sizeof(info);info.biWidth=static_cast<LONG>(desc.Width);
            info.biHeight=-static_cast<LONG>(desc.Height);info.biPlanes=1u;
            info.biBitCount=32u;info.biCompression=BI_RGB;
            auto path=path_;path.replace_extension(L".bmp");
            auto temporary=path;temporary+=L".tmp";
            std::ofstream out(temporary,std::ios::binary);
            out.write(reinterpret_cast<const char*>(&file),sizeof(file));
            out.write(reinterpret_cast<const char*>(&info),sizeof(info));
            for(unsigned y=0;y<desc.Height;++y)out.write(
                static_cast<const char*>(pixels.pBits)+y*pixels.Pitch,desc.Width*4u);
            out.flush();const bool ok=bool(out);out.close();
            if(ok)std::filesystem::rename(temporary,path);
            return ok;
        }catch(...){return false;}
    }
    void Publish(HRESULT result,const char* boundary) noexcept {
        try {
            // Explicit comparison readback is independent of desktop composition.
            // It does not bind surfaces or change any rendering state.
            const bool frame_pixels=WriteFramePixels();
            auto temporary=path_;temporary+=L".tmp";
            std::ofstream out(temporary);
            out<<"{\"boundary\":"<<std::quoted(boundary)<<",\"boundary_result\":"<<result<<",\"overflow\":"<<overflow_
               <<",\"capture_failed\":"<<capture_failed_<<",\"frame_pixels_ok\":"<<frame_pixels<<",\"draws\":[";
            bool first=true;
            for(const auto& row:draws_){
                if(!first)out<<',';first=false;
                out<<"{\"indexed\":"<<row.indexed<<",\"read_failed\":"<<row.read_failed
                   <<",\"primitive\":"<<row.primitive<<",\"vertices\":"<<row.vertices
                   <<",\"primitives\":"<<row.primitives<<",\"fvf\":"<<row.fvf<<",\"stride\":"<<row.stride
                   <<",\"vs\":"<<row.vertex_shader<<",\"ps\":"<<row.pixel_shader
                   <<",\"viewport\":["<<row.viewport[0]<<','<<row.viewport[1]<<','<<row.viewport[2]<<','<<row.viewport[3]
                   <<"],\"target\":["<<row.target[0]<<','<<row.target[1]<<','<<row.target[2]<<','<<row.target[3]<<"],\"render\":{";
                for(std::size_t i=0;i<render_names.size();++i){if(i)out<<',';out<<std::quoted(render_names[i])<<':'<<row.render[i];}
                out<<"},\"stages\":[";
                for(unsigned stage=0;stage<2;++stage){
                    if(stage)out<<',';
                    out<<"{\"texture\":["<<row.textures[stage][0]<<','<<row.textures[stage][1]<<','<<row.textures[stage][2]<<"],\"sampler\":{";
                    for(std::size_t i=0;i<sampler_names.size();++i){if(i)out<<',';out<<std::quoted(sampler_names[i])<<':'<<row.samplers[stage][i];}
                    out<<"}}";
                }
                out<<"]}";
            }
            out<<"],\"shader_details\":[";first=true;
            for(const auto& row:draws_){if(!first)out<<',';first=false;row.shader_state.Write(out);}
            out<<"]}\n";out.flush();const bool ok=bool(out);out.close();
            if(ok)std::filesystem::rename(temporary,path_);
        }catch(...){/* Diagnostics must not change the game's Present result. */}
    }
    static HRESULT STDMETHODCALLTYPE DrawIndexed(IDirect3DDevice9* device,D3DPRIMITIVETYPE type,
        INT base,UINT minimum,UINT vertices,UINT start,UINT primitives) {
        auto& self=Instance();if(device==self.device_)self.Observe(true,type,vertices,primitives);
        return reinterpret_cast<Indexed>(self.methods_[3])(device,type,base,minimum,vertices,start,primitives);
    }
    static HRESULT STDMETHODCALLTYPE DrawPrimitive(IDirect3DDevice9* device,D3DPRIMITIVETYPE type,UINT start,UINT primitives) {
        auto& self=Instance();if(device==self.device_)self.Observe(false,type,0,primitives);
        return reinterpret_cast<Primitive>(self.methods_[2])(device,type,start,primitives);
    }
    static HRESULT STDMETHODCALLTYPE EndFrame(IDirect3DDevice9* device,const RECT* source,
        const RECT* target,HWND window,const RGNDATA* dirty) {
        auto& self=Instance();const auto present=reinterpret_cast<Present>(self.methods_[0]);
        if(device!=self.device_)return present(device,source,target,window,dirty);
        if(!self.Restore()){self.capture_failed_=true;return present(device,source,target,window,dirty);}
        const auto result=present(device,source,target,window,dirty);
        self.Publish(result,"Present");self.device_=nullptr;self.prior_=nullptr;
        device->Release();
        return result;
    }
    static HRESULT STDMETHODCALLTYPE EndSceneBoundary(IDirect3DDevice9* device) {
        auto& self=Instance();const auto end=reinterpret_cast<EndScene>(self.methods_[1]);
        if(device!=self.device_)return end(device);
        if(!self.Restore()){self.capture_failed_=true;return end(device);}
        const auto result=end(device);
        self.Publish(result,"EndScene");self.device_=nullptr;self.prior_=nullptr;
        device->Release();return result;
    }
public:
    static ComparisonD3D9Observer& Instance(){static ComparisonD3D9Observer observer;return observer;}
    bool Pending() const noexcept {return device_!=nullptr;}
    // Keep the same request when a device transition removes our temporary
    // callbacks. Re-arm only an intact table or our own callbacks; never
    // overwrite a third party's replacement. Discard partial frame draws.
    bool ResumeOnDeviceInterface(IDirect3DDevice9* device) {
        if(!device_ || !device)return false;
        IDirect3DDevice9* actual{};
        if(FAILED(device->QueryInterface(IID_IDirect3DDevice9,
            reinterpret_cast<void**>(&actual))) || !actual)return false;
        struct Release {IDirect3DDevice9* value;~Release(){value->Release();}} release{actual};
        const std::array<void*,4> callbacks{reinterpret_cast<void*>(&EndFrame),reinterpret_cast<void*>(&EndSceneBoundary),
            reinterpret_cast<void*>(&DrawPrimitive),reinterpret_cast<void*>(&DrawIndexed)};
        auto** table=*reinterpret_cast<void***>(actual);
        bool intact=actual==device_ && table==prior_;
        for(unsigned i=0;i<slots.size();++i)intact=intact && table[slots[i]]==callbacks[i];
        if(intact)return true;
        for(unsigned i=0;i<slots.size();++i)
            if(prior_[slots[i]]!=callbacks[i] && prior_[slots[i]]!=methods_[i])return false;
        const auto path=path_;
        if(!Restore())return false;
        auto* previous=device_;device_=nullptr;prior_=nullptr;previous->Release();
        return Begin(actual,path);
    }
    // The native host can dispatch a request from a nested loading/UI callback
    // while its device pointer is a scoped forwarding probe. COM resolves that
    // surface to the stable device before Begin retains it and patches slots.
    bool BeginOnDeviceInterface(IDirect3DDevice9* device,const std::filesystem::path& path) {
        if(device_ || !device)return false;
        IDirect3DDevice9* actual{};
        if(FAILED(device->QueryInterface(IID_IDirect3DDevice9,
            reinterpret_cast<void**>(&actual))) || !actual)return false;
        struct Release {
            IDirect3DDevice9* value;
            ~Release(){value->Release();}
        } release{actual};
        return Begin(actual,path);
    }
    bool Begin(IDirect3DDevice9* device,const std::filesystem::path& path) {
        if(device_ || !device)return false;
        D3DCAPS9 caps{};
        if(FAILED(device->GetDeviceCaps(&caps)))return false;
        max_vertex_constants_=caps.MaxVertexShaderConst;
        path_=path;draws_.clear();draws_.reserve(1024);overflow_=capture_failed_=false;
        prior_=*reinterpret_cast<void***>(device);
        for(unsigned i=0;i<slots.size();++i)methods_[i]=prior_[slots[i]];
        const std::array<void*,4> callbacks{reinterpret_cast<void*>(&EndFrame),reinterpret_cast<void*>(&EndSceneBoundary),
            reinterpret_cast<void*>(&DrawPrimitive),reinterpret_cast<void*>(&DrawIndexed)};
        device->AddRef();device_=device;
        for(unsigned i=0;i<slots.size();++i)if(!Patch(prior_+slots[i],callbacks[i])){
            if(Restore()){device_=nullptr;prior_=nullptr;device->Release();}
            return false;
        }
        return true;
    }
    ~ComparisonD3D9Observer(){if(device_ && Restore())device_->Release();}
};
}
