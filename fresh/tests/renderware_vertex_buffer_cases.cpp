#include "renderware_vertex_buffer_cases.h"
#include <rwcore.h>
#include <rpworld.h>
#include <d3d9.h>
#include <windows.h>
#include <array>
#include <cstring>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <cstddef>

namespace pal4::test {
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static_assert(std::is_same_v<decltype(&RwD3D9CreateVertexBuffer),
              RwBool (__cdecl *)(RwUInt32,RwUInt32,void**,RwUInt32*)>);
static_assert(std::is_same_v<decltype(&RwD3D9DynamicVertexBufferDestroy),void (__cdecl *)(void*)>);
static_assert(sizeof(RxD3D9ResEntryHeader) == 64);
static_assert(sizeof(RxD3D9InstanceData) == 36);
static_assert(offsetof(RxD3D9InstanceData, minVert) == 4);
static_assert(offsetof(RxD3D9InstanceData, baseIndex) == 20);
static_assert(offsetof(RpWorldSector, numVertices) == 130);
static_assert(offsetof(D3DCAPS9, DeclTypes) == 236);
static_assert(D3DDTCAPS_DEC3N == 0x80);
static_assert(rpD3D9WORLDSECTORUSAGE_CREATETANGENTS == 0x10000);
IDirect3DVertexBuffer9* Buffer(void* value) { return static_cast<IDirect3DVertexBuffer9*>(value); }
D3DVERTEXBUFFER_DESC Description(void* value) {
    D3DVERTEXBUFFER_DESC out{}; Require(value && SUCCEEDED(Buffer(value)->GetDesc(&out)), "vertex descriptor"); return out;
}
bool Hardware() { return (static_cast<const D3DCAPS9*>(RwD3D9GetCaps())->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0; }
struct Range {
    RwUInt32 stride{}, size{}, offset{}; void* value{};
    Range(RwUInt32 step, RwUInt32 bytes) : stride(step), size(bytes) {
        Require(RwD3D9CreateVertexBuffer(stride,size,&value,&offset), "managed suballocation");
    }
    Range(const Range&) = delete;
    Range& operator=(const Range&) = delete;
    void Free() { if(value) { RwD3D9DestroyVertexBuffer(stride,size,value,offset); value=nullptr; } }
    ~Range() { Free(); }
};
struct Dynamic {
    void* value{};
    explicit Dynamic(RwUInt32 size) { Require(RwD3D9DynamicVertexBufferCreate(size,&value), "dynamic allocation"); }
    ~Dynamic() { if(value) RwD3D9DynamicVertexBufferDestroy(value); }
    Dynamic(const Dynamic&) = delete;
    Dynamic& operator=(const Dynamic&) = delete;
};
void Managed() {
    Range a(24,72),b(24,96),c(24,24); void* original=a.value;
    Require(a.value==b.value && a.value==c.value && a.offset==0 && b.offset==72 && c.offset==168,
            "same-stride managed ranges share buffer and advance by requested bytes");
    const auto desc=Description(original);
    Require(desc.Pool==D3DPOOL_MANAGED && desc.Usage==D3DUSAGE_WRITEONLY && desc.FVF==0 && desc.Size==131088,
            "128KiB base capacity rounded up to stride");
    Range other_stride(32,32); Require(other_stride.value!=original,"different stride selects another pool");
    b.Free(); a.Free(); Range merged(24,168);
    Require(merged.value==original && merged.offset==0,"adjacent freed ranges coalesce for reuse");
    merged.Free(); c.Free(); Range whole(24,desc.Size),overflow(24,24);
    Require(whole.value==original && whole.offset==0 && overflow.value!=original && overflow.offset==0,
            "whole-buffer reuse then allocate second backing buffer");
    void* data{};Require(SUCCEEDED(Buffer(whole.value)->Lock(0,24,&data,0)),"managed range lock");
    std::memset(data,0x3c,24);Require(SUCCEEDED(Buffer(whole.value)->Unlock()),"managed range unlock");
}
void Cache() {
    Dynamic a(192),b(384);void* previous=a.value;
    const auto desc=Description(a.value);
    Require(desc.Size==192 && desc.Usage==(D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY) &&
            desc.Pool==(Hardware()?D3DPOOL_DEFAULT:D3DPOOL_SYSTEMMEM),"dynamic usage and capability-selected pool");
    RwD3D9DynamicVertexBufferDestroy(a.value);a.value=nullptr;
    Dynamic reused(192);Require(reused.value==previous,"exact-size cached dynamic buffer reused");
    RwD3D9DynamicVertexBufferDestroy(reused.value);reused.value=nullptr;
    RwD3D9DynamicVertexBufferDestroy(b.value);b.value=nullptr;
    Dynamic resized(768);Require(Description(resized.value).Size==768,"nonmatching inactive cache entry recreated at requested size");
}
void Ring() {
    std::set<void*> seen;
    for(int i=0;i<5;++i){void* buffer{},*data{};RwUInt32 base=99;
        Require(RwD3D9DynamicVertexBufferLock(1,262144,&buffer,&data,&base),"ring full-slot lock");
        Require(buffer && data && base==0 && Description(buffer).Size==262144,"ring full-slot discard and capacity");
        seen.insert(buffer);std::memset(data,i+1,64);Require(RwD3D9DynamicVertexBufferUnlock(buffer),"ring unlock");
    }
    Require(seen.size()==(Hardware()?4u:1u),"hardware uses four rotating slots; software uses one");
    void *first{},*second{},*third{},*data{};RwUInt32 base{};
    Require(RwD3D9DynamicVertexBufferLock(20,3,&first,&data,&base) && base==0,"new small batch discards full slot");
    Require(RwD3D9DynamicVertexBufferUnlock(first),"first small unlock");
    Require(RwD3D9DynamicVertexBufferLock(20,3,&second,&data,&base) && second==first && base==(Hardware()?3u:0u),
            "hardware appends while software restarts at zero");
    Require(RwD3D9DynamicVertexBufferUnlock(second),"second small unlock");
    Require(RwD3D9DynamicVertexBufferLock(28,1,&third,&data,&base) && third==first && base==(Hardware()?5u:0u),
            "next append aligns byte cursor up to new vertex stride");
    Require(RwD3D9DynamicVertexBufferUnlock(third),"third small unlock");
    // Oversized request grows one internal slot; returned buffer is manager-owned.
    void* large{};Require(RwD3D9DynamicVertexBufferLock(4,70000,&large,&data,&base) && base==0 &&
            Description(large).Size==280000,"ring growth allocates requested byte count");
    Require(RwD3D9DynamicVertexBufferUnlock(large),"grown ring unlock");
}
void** watched{};bool released_null{},restored_nonnull{};unsigned releases{},restores{};
rwD3D9DeviceReleaseCallBack previous_release{};rwD3D9DeviceRestoreCallBack previous_restore{};
void Released() { if(previous_release)previous_release();++releases;released_null=*watched==nullptr; }
void Restored() { if(previous_restore)previous_restore();++restores;restored_nonnull=*watched!=nullptr; }
struct Callbacks {
    explicit Callbacks(void** output){watched=output;releases=restores=0;released_null=restored_nonnull=false;
        previous_release=_rwD3D9DeviceGetReleaseCallback();previous_restore=_rwD3D9DeviceGetRestoreCallback();
        _rwD3D9DeviceSetReleaseCallback(Released);_rwD3D9DeviceSetRestoreCallback(Restored);}
    ~Callbacks(){_rwD3D9DeviceSetReleaseCallback(previous_release);_rwD3D9DeviceSetRestoreCallback(previous_restore);watched=nullptr;}
};
struct Camera {
    RwCamera* value=RwCameraCreate();RwFrame* frame=RwFrameCreate();
    explicit Camera(HWND window){Require(value&&frame,"reset camera/frame");RwCameraSetFrame(value,frame);
        RECT rect{};GetClientRect(window,&rect);
        auto* color=RwRasterCreate(rect.right,rect.bottom,0,rwRASTERTYPECAMERA);
        auto* depth=RwRasterCreate(rect.right,rect.bottom,0,rwRASTERTYPEZBUFFER);Require(color&&depth,"reset rasters");
        RwCameraSetRaster(value,color);RwCameraSetZRaster(value,depth);}
    ~Camera(){auto* color=RwCameraGetRaster(value);auto* depth=RwCameraGetZRaster(value);
        RwCameraSetRaster(value,nullptr);RwCameraSetZRaster(value,nullptr);RwCameraSetFrame(value,nullptr);
        RwCameraDestroy(value);RwFrameDestroy(frame);RwRasterDestroy(color);RwRasterDestroy(depth);}
};
void Reset(HWND window) {
    Camera camera(window);RwRGBA color{1,2,3,255};Require(RwCameraClear(camera.value,&color,rwCAMERACLEARIMAGE|rwCAMERACLEARZ),"initial reset clear");
    Dynamic dynamic(512);Range managed(24,96);void* managed_before=managed.value;
    Callbacks callbacks(&dynamic.value);
    Require(SetWindowPos(window,nullptr,0,0,240,180,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE),"resize reset window");
    Require(RwCameraClear(camera.value,&color,rwCAMERACLEARIMAGE|rwCAMERACLEARZ),"buffer release/reset/restore");
    Require(releases==1 && restores==1 && released_null==Hardware() && restored_nonnull,
            "registered output storage updated before release/restore callbacks");
    Require(Description(dynamic.value).Size==512 && managed.value==managed_before &&
            Description(managed.value).Pool==D3DPOOL_MANAGED,"dynamic size recreated and managed buffer survives reset");
    void* data{};Require(SUCCEEDED(Buffer(dynamic.value)->Lock(0,512,&data,D3DLOCK_DISCARD)),"restored dynamic lock");
    std::memset(data,0x62,512);Require(SUCCEEDED(Buffer(dynamic.value)->Unlock()),"restored dynamic unlock");
}
}
void RunVertexBufferCase(std::string_view name,void* window) {
    if(name=="vertex_buffer_managed")Managed();else if(name=="vertex_buffer_cache")Cache();
    else if(name=="vertex_buffer_ring")Ring();else if(name=="vertex_buffer_reset")Reset(static_cast<HWND>(window));
    else throw std::runtime_error("unknown vertex buffer case");
}
}
