#pragma once
#include <d3d9.h>
#include <initializer_list>
#include <set>

// Pinned SDK data symbol (d3d9device.obj), used only by tests. No binary patch.
extern "C" IDirect3D9* _RwDirect3DObject;
namespace pal4::test {
// Forward real COM calls, denying selected SDK format capability probes.
// Device/resource creation and conversion still use real D3D9.
class FormatUnsupported : public IDirect3D9 {
    std::set<D3DFORMAT> rejected_;
    IDirect3D9* original_;
public:
    unsigned denied{};
    explicit FormatUnsupported(std::initializer_list<D3DFORMAT> formats):rejected_(formats),original_(_RwDirect3DObject){_RwDirect3DObject=this;}
    ~FormatUnsupported(){_RwDirect3DObject=original_;}
    FormatUnsupported(const FormatUnsupported&)=delete;
    FormatUnsupported& operator=(const FormatUnsupported&)=delete;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID i,void** p) override{return original_->QueryInterface(i,p);}
    ULONG STDMETHODCALLTYPE AddRef() override{return original_->AddRef();}
    ULONG STDMETHODCALLTYPE Release() override{return original_->Release();}
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* p) override{return original_->RegisterSoftwareDevice(p);}
    UINT STDMETHODCALLTYPE GetAdapterCount() override{return original_->GetAdapterCount();}
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT a,DWORD f,D3DADAPTER_IDENTIFIER9* p) override{return original_->GetAdapterIdentifier(a,f,p);}
    UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT a,D3DFORMAT f) override{return original_->GetAdapterModeCount(a,f);}
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT a,D3DFORMAT f,UINT m,D3DDISPLAYMODE* p) override{return original_->EnumAdapterModes(a,f,m,p);}
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT a,D3DDISPLAYMODE* p) override{return original_->GetAdapterDisplayMode(a,p);}
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT a,D3DDEVTYPE t,D3DFORMAT f,D3DFORMAT b,BOOL w) override{return original_->CheckDeviceType(a,t,f,b,w);}
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT a,D3DDEVTYPE t,D3DFORMAT f,DWORD u,D3DRESOURCETYPE r,D3DFORMAT c) override{
        if(rejected_.contains(c)){++denied;return D3DERR_NOTAVAILABLE;}
        return original_->CheckDeviceFormat(a,t,f,u,r,c);
    }
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT a,D3DDEVTYPE t,D3DFORMAT f,BOOL w,D3DMULTISAMPLE_TYPE s,DWORD* q) override{return original_->CheckDeviceMultiSampleType(a,t,f,w,s,q);}
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT a,D3DDEVTYPE t,D3DFORMAT f,D3DFORMAT c,D3DFORMAT z) override{return original_->CheckDepthStencilMatch(a,t,f,c,z);}
    HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT a,D3DDEVTYPE t,D3DFORMAT s,D3DFORMAT d) override{return original_->CheckDeviceFormatConversion(a,t,s,d);}
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT a,D3DDEVTYPE t,D3DCAPS9* c) override{return original_->GetDeviceCaps(a,t,c);}
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT a) override{return original_->GetAdapterMonitor(a);}
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT a,D3DDEVTYPE t,HWND w,DWORD f,D3DPRESENT_PARAMETERS* p,IDirect3DDevice9** d) override{return original_->CreateDevice(a,t,w,f,p,d);}
};
class DxtUnsupported final : public FormatUnsupported {
public:
    DxtUnsupported():FormatUnsupported({D3DFMT_DXT1,D3DFMT_DXT2,D3DFMT_DXT3,D3DFMT_DXT4,D3DFMT_DXT5}){}
};
}
