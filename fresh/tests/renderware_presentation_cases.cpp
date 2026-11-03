#include "renderware_presentation_cases.h"
#include "pal4/game_render_system.h"
#include "pal4/renderware_presentation_native.h"
// Include RW first: D3D macros discard HRESULT and differ from matched setters.
// Global RWSUPPRESSINLINE cannot be used: this Release archive omits some accessors.
#include <rwcore.h>
#include <d3d9.h>
#include <wrl/client.h>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "pal4/comparison_d3d9_observer.h"
#include "support/d3d9_device_forwarder.h"
#include "pal4/camera_texture_pass_native.h"
#include "pal4/camera_collision_render_native.h"
#include "pal4/camera_render_pass_native.h"
#include "pal4/camera_render_geometry_native.h"
#include "pal4/native_hook_clump_names.h"
#include <rpcollis.h>
#include "pal4/native_pal4_camera_factory.h"

namespace pal4::test {
namespace {
using Microsoft::WRL::ComPtr;
static_assert(std::is_same_v<decltype(&RwD3D9DrawIndexedPrimitive),
    void (__cdecl *)(RwUInt32, RwInt32, RwUInt32, RwUInt32, RwUInt32, RwUInt32)>);
static_assert(sizeof(RxD3D9VertexStream) == 16);
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Near(float a, float b) { Require(std::fabs(a - b) < 0.0001f, "camera transform mismatch"); }
struct Camera {
    RwCamera* value;
    RwFrame* frame = RwFrameCreate();
    Camera(HWND window, bool texture = false, int side = 32, bool game_variant = false)
        : value(game_variant ? CreateNativePal4Camera() : RwCameraCreate()) {
        Require(value && frame, "camera/frame creation");
        RwCameraSetFrame(value, frame);
        RECT client{}; GetClientRect(window, &client);
        RwCameraSetRaster(value, RwRasterCreate(texture ? side : client.right,
            texture ? side : client.bottom, 0, texture ?
            rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888 : rwRASTERTYPECAMERA));
        Require(RwCameraGetRaster(value) != nullptr, "presentation raster");
        RwCameraSetNearClipPlane(value, 2.0f); RwCameraSetFarClipPlane(value, 10.0f);
        const RwV2d view{2.0f, 4.0f}; RwCameraSetViewWindow(value, &view);
    }
    ~Camera() {
        auto* raster = RwCameraGetRaster(value);
        RwCameraSetRaster(value, nullptr); RwCameraSetFrame(value, nullptr);
        RwCameraDestroy(value); RwFrameDestroy(frame); RwRasterDestroy(raster);
    }
    void Update() {
        Require(RwCameraBeginUpdate(value) == value, "camera begin");
        Require(RwCameraEndUpdate(value) == value, "camera end");
    }
};
std::vector<int> events;
rwD3D9DeviceReleaseCallBack old_release{};
rwD3D9DeviceRestoreCallBack old_restore{};
void Released() { if (old_release) old_release(); events.push_back(1); }
void Restored() { if (old_restore) old_restore(); events.push_back(2); }
struct Callbacks {
    Callbacks() {
        events.clear(); old_release = _rwD3D9DeviceGetReleaseCallback();
        old_restore = _rwD3D9DeviceGetRestoreCallback();
        _rwD3D9DeviceSetReleaseCallback(Released); _rwD3D9DeviceSetRestoreCallback(Restored);
    }
    ~Callbacks() {
        _rwD3D9DeviceSetReleaseCallback(old_release); _rwD3D9DeviceSetRestoreCallback(old_restore);
    }
};
DWORD Interval(IDirect3DDevice9* device) {
    ComPtr<IDirect3DSwapChain9> chain;
    Require(SUCCEEDED(device->GetSwapChain(0, &chain)), "get presentation chain");
    D3DPRESENT_PARAMETERS params{};
    Require(SUCCEEDED(chain->GetPresentParameters(&params)), "presentation parameters");
    Require(params.Windowed, "fixture must remain windowed"); return params.PresentationInterval;
}
void Policy(HWND window) {
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Camera camera(window); Callbacks callbacks;
    GameRenderState state(0); state.Write(0, reinterpret_cast<std::uint32_t>(window));
    GameRenderSystem game(state); NativeRenderWarePresentation presentation;
    const auto address = reinterpret_cast<std::uint32_t>(camera.value);
    auto show = [&](unsigned first, unsigned second, DWORD interval) {
        state.Write(8, first); state.Write(0x54c, second); camera.Update();
        Require(game.ShowGameRaster(address, presentation) == address, "game presentation return pointer");
        Require(Interval(device) == interval, "game flag did not reach actual swapchain");
    };
    show(0, 1, D3DPRESENT_INTERVAL_IMMEDIATE);
    events.clear(); show(1, 1, D3DPRESENT_INTERVAL_ONE);
    Require(events == std::vector<int>({1, 2}), "vsync change release/restore order");
    show(1, 1, D3DPRESENT_INTERVAL_ONE);
    Require(events == std::vector<int>({1, 2}), "same interval reset device");
    show(1, 0, D3DPRESENT_INTERVAL_IMMEDIATE);
    Require(events == std::vector<int>({1, 2, 1, 2}), "vsync disable reset order");
    std::puts("game presentation: immediate -> one -> one -> immediate; two resets");
}
void Getter(HWND window) {
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Camera camera(window, true);
    Require(RwCameraBeginUpdate(camera.value) == camera.value, "getter camera begin");
    RwD3D9SetVertexShader(nullptr); RwD3D9SetPixelShader(nullptr);
    RwD3D9SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE); RwD3D9SetIndices(nullptr);
    ComPtr<IDirect3DSurface9> before, after;
    Require(SUCCEEDED(device->GetRenderTarget(0, &before)) &&
        before.Get() == RwD3D9GetCurrentD3DRenderTarget(0), "populated target cache");
    Require(RwD3D9GetCurrentD3DDevice() == device, "device getter identity");
    Require(RwD3D9GetCurrentD3DRenderTarget(0) == nullptr, "getter did not invalidate target cache");
    DWORD fvf{};
    Require(SUCCEEDED(device->GetRenderTarget(0, &after)) && after.Get() == before.Get() &&
        SUCCEEDED(device->GetFVF(&fvf)) && fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE),
        "getter unexpectedly changed actual COM state");
    Require(SUCCEEDED(device->SetFVF(D3DFVF_XYZ)), "external FVF mutation");
    RwD3D9SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    Require(SUCCEEDED(device->GetFVF(&fvf)) && fvf == (D3DFVF_XYZRHW | D3DFVF_DIFFUSE),
        "getter failed to invalidate FVF cache: original binding skipped");
    Require(RwD3D9SetRenderTarget(0, RwCameraGetRaster(camera.value)), "rebind after invalidation");
    Require(RwCameraEndUpdate(camera.value) == camera.value, "getter camera end");
    std::puts("getter diagnostic: borrowed device unchanged, RW caches invalidated, COM target/FVF preserved");
}
void Projection(HWND window) {
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Camera camera(window, true);
    const RwV3d position{3, 5, 7}; RwFrameTranslate(camera.frame, &position, rwCOMBINEREPLACE);
    const RwV2d offset{0.5f, -0.25f}; RwCameraSetViewOffset(camera.value, &offset);
    RwCameraSetFogDistance(camera.value, 6.0f);
    // The EXE adds fog distance/far writes here; the available SDK omits them.
    const auto start = std::bit_cast<RwUInt32>(17.0f), end = std::bit_cast<RwUInt32>(19.0f);
    RwD3D9SetRenderState(D3DRS_FOGSTART, start); RwD3D9SetRenderState(D3DRS_FOGEND, end);
    for (auto projection : {rwPERSPECTIVE, rwPARALLEL}) {
        RwCameraSetProjection(camera.value, projection);
        Require(RwCameraBeginUpdate(camera.value) == camera.value, "projection begin");
        D3DMATRIX view{}, actual{};
        Require(SUCCEEDED(device->GetTransform(D3DTS_VIEW, &view)) &&
            SUCCEEDED(device->GetTransform(D3DTS_PROJECTION, &actual)), "transform readback");
        D3DMATRIX expected{};
        expected._11 = -1; expected._22 = 1; expected._33 = 1;
        expected._41 = 3; expected._42 = -5; expected._43 = -7; expected._44 = 1;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) Near(view.m[r][c], expected.m[r][c]);
        expected = {}; expected._11 = 0.5f; expected._22 = 0.25f;
        expected._31 = 0.25f; expected._32 = -0.0625f;
        expected._41 = -0.25f; expected._42 = 0.0625f;
        expected._33 = projection == rwPERSPECTIVE ? 1.25f : 0.125f;
        expected._43 = -2 * expected._33;
        expected._34 = projection == rwPERSPECTIVE ? 1.0f : 0.0f;
        expected._44 = projection == rwPARALLEL ? 1.0f : 0.0f;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) Near(actual.m[r][c], expected.m[r][c]);
        RwUInt32 observed_start{}, observed_end{};
        RwD3D9GetRenderState(D3DRS_FOGSTART, &observed_start);
        RwD3D9GetRenderState(D3DRS_FOGEND, &observed_end);
        Require(observed_start == start && observed_end == end, "SDK fog omission diagnostic changed");
        Require(RwCameraEndUpdate(camera.value) == camera.value, "projection end");
    }
    std::puts("projection matrices verified; SDK retains fog17/19 while EXE writes camera6/10 (pending adaptation)");
}
struct Vertex { float x, y, z, rhw; DWORD color; };
void Draw(HWND window, bool sdk_buffer = false, bool observe = false) {
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Camera camera(window, true);
    camera.Update();
    // Establish IMMEDIATE before drawing: the first interval transition resets
    // the device and recreates camera textures, discarding their old contents.
    Require(RwCameraShowRaster(camera.value, window, 0), "initial presentation interval");
    // A real depth attachment is required by this driver fixture, even with Z disabled.
    auto* depth = RwRasterCreate(32, 32, 0, rwRASTERTYPEZBUFFER);
    Require(depth != nullptr, "draw depth"); RwCameraSetZRaster(camera.value, depth);
    struct Depth { RwCamera* camera; RwRaster* raster; ~Depth() { RwCameraSetZRaster(camera, nullptr); RwRasterDestroy(raster); } } depth_owner{camera.value, depth};
    ComPtr<IDirect3DVertexBuffer9> vertices;
    struct SdkRange {
        void* value{}; RwUInt32 offset{}, size{};
        ~SdkRange(){if(value)RwD3D9DestroyVertexBuffer(sizeof(Vertex),size,value,offset);}
    } sdk;
    ComPtr<IDirect3DIndexBuffer9> indices;
    const Vertex triangle[]{{2, 2, 0.5f, 1, 0xffdb4923}, {29, 2, 0.5f, 1, 0xffdb4923}, {2, 29, 0.5f, 1, 0xffdb4923}};
    if(sdk_buffer) {
        sdk.size=sizeof(triangle);
        Require(RwD3D9CreateVertexBuffer(sizeof(Vertex),sdk.size,&sdk.value,&sdk.offset) && sdk.offset==0,
                "SDK triangle vertex allocation at initial offset");
        // Retain one COM reference for this fixture; SDK still owns the buffer.
        vertices=static_cast<IDirect3DVertexBuffer9*>(sdk.value);
    } else {
        Require(SUCCEEDED(device->CreateVertexBuffer(sizeof(triangle), 0, D3DFVF_XYZRHW | D3DFVF_DIFFUSE,
            D3DPOOL_MANAGED, &vertices, nullptr)), "vertex buffer creation");
    }
    Require(SUCCEEDED(device->CreateIndexBuffer(6, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &indices, nullptr)),
        "index buffer creation");
    void* data{}; Require(SUCCEEDED(vertices->Lock(0, 0, &data, 0)), "vertex lock");
    std::memcpy(data, triangle, sizeof(triangle)); vertices->Unlock();
    const unsigned short order[]{0, 1, 2}; Require(SUCCEEDED(indices->Lock(0, 0, &data, 0)), "index lock");
    std::memcpy(data, order, sizeof(order)); indices->Unlock();
    for (bool indexed : {false, true}) {
        RwRGBA black{0, 0, 0, 255}; Require(RwCameraClear(camera.value, &black, rwCAMERACLEARIMAGE), "draw clear");
        Require(RwCameraBeginUpdate(camera.value), "draw begin");
        RwD3D9SetVertexShader(nullptr); RwD3D9SetPixelShader(nullptr);
        RwD3D9SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
        RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE); RwD3D9SetRenderState(D3DRS_ZENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); RwD3D9SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_FOGENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_STENCILENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        RwD3D9SetRenderState(D3DRS_MULTISAMPLEMASK, ~0u);
        RwD3D9SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
        RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
        RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
        RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        RwD3D9SetTexture(nullptr, 0);
        RxD3D9VertexStream streams[2]{}; streams[0].vertexBuffer = vertices.Get();
        streams[0].offset = sizeof(Vertex); streams[0].stride = sizeof(Vertex);
        _rwD3D9SetStreams(streams, TRUE);
        ComPtr<IDirect3DVertexBuffer9> bound; UINT offset{}, stride{};
        Require(SUCCEEDED(device->GetStreamSource(0, &bound, &offset, &stride)) &&
            bound.Get() == vertices.Get() && offset == sizeof(Vertex) && stride == sizeof(Vertex), "stream offset binding");
        _rwD3D9SetStreams(streams, FALSE);
        bound.Reset(); Require(SUCCEEDED(device->GetStreamSource(0, &bound, &offset, &stride)) && offset == 0,
            "useOffsets=false must force zero");
        RwD3D9SetIndices(indices.Get());
        auto** table=*reinterpret_cast<void***>(device);
        constexpr std::array<unsigned,4> observed_slots{17,42,81,82};
        std::array<void*,4> original_methods{};
        for(unsigned i=0;i<observed_slots.size();++i)original_methods[i]=table[observed_slots[i]];
        const auto capture=std::filesystem::current_path() /
            ("d3d_observer_"+std::to_string(GetCurrentProcessId())+"_"+std::to_string(indexed)+".json");
        if(observe){
            if(indexed){
                auto transient=std::make_unique<pal4::test::D3D9DeviceForwarder>(device);
                auto** forwarding_table=*reinterpret_cast<void***>(transient.get());
                std::array<void*,4> forwarding_methods{};
                for(unsigned i=0;i<observed_slots.size();++i)
                    forwarding_methods[i]=forwarding_table[observed_slots[i]];
                Require(ComparisonD3D9Observer::Instance().BeginOnDeviceInterface(transient.get(),capture),
                        "capture resolves a scoped forwarding device to the real COM interface");
                for(unsigned i=0;i<observed_slots.size();++i)
                    Require(forwarding_table[observed_slots[i]]==forwarding_methods[i],
                            "scoped forwarding vtable remains untouched");
                transient.reset();
                for(unsigned i=0;i<observed_slots.size();++i){
                    DWORD protection{},restored{};
                    Require(VirtualProtect(table+observed_slots[i],sizeof(void*),PAGE_READWRITE,&protection)!=FALSE,
                            "device transition fixture opens callback slot");
                    InterlockedExchangePointer(reinterpret_cast<void* volatile*>(table+observed_slots[i]),original_methods[i]);
                    Require(VirtualProtect(table+observed_slots[i],sizeof(void*),protection,&restored)!=FALSE,
                            "device transition fixture restores callback slot protection");
                }
                Require(ComparisonD3D9Observer::Instance().Pending(),"lost hooks retain the same pending request");
                Require(ComparisonD3D9Observer::Instance().ResumeOnDeviceInterface(device),
                        "same observation resumes after device callbacks are restored");
            }else{
                Require(ComparisonD3D9Observer::Instance().Begin(device,capture),"begin real device draw observation");
            }
            Require(!ComparisonD3D9Observer::Instance().Begin(device,capture),"reject overlapping observation");
        }
        if (indexed) RwD3D9DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 3, 0, 1);
        else RwD3D9DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);
        DWORD passes{};
        Require(SUCCEEDED(device->ValidateDevice(&passes)), "draw fixture render states invalid");
        Require(RwCameraEndUpdate(camera.value), "draw end");
        Require(RwCameraShowRaster(camera.value, window, 0), "draw present/end scene");
        if(observe){
            Require(*reinterpret_cast<void***>(device)==table,"preserve driver vtable identity");
            for(unsigned i=0;i<observed_slots.size();++i)
                Require(table[observed_slots[i]]==original_methods[i],"restore original device methods");
            std::ifstream file(capture);
            const std::string content{std::istreambuf_iterator<char>(file),{}};
            Require(content.find("\"capture_failed\":0")!=std::string::npos &&
                content.find("\"read_failed\":0")!=std::string::npos &&
                content.find("\"draws\":[{")!=std::string::npos,"real device draw states captured without failures");
        }
        auto* image = RwImageCreate(32, 32, 32);
        Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image, RwCameraGetRaster(camera.value)), "draw readback");
        auto pixel = [&](int x, int y) { return *reinterpret_cast<RwRGBA*>(RwImageGetPixels(image) + y * RwImageGetStride(image) + 4 * x); };
        const auto inside = pixel(8, 8), outside = pixel(30, 30); RwImageDestroy(image);
        std::printf("draw indexed=%d inside=%u,%u,%u outside=%u,%u,%u\n", indexed,
            inside.red, inside.green, inside.blue, outside.red, outside.green, outside.blue);
        Require(inside.red == 219 && inside.green == 73 && inside.blue == 35 &&
            outside.red == 0 && outside.green == 0 && outside.blue == 0, "triangle pixel coverage");
    }
    RxD3D9VertexStream empty[2]{}; _rwD3D9SetStreams(empty, TRUE); RwD3D9SetIndices(nullptr);
    for (UINT stream = 0; stream < 2; ++stream) {
        ComPtr<IDirect3DVertexBuffer9> bound; UINT offset{}, stride{};
        Require(SUCCEEDED(device->GetStreamSource(stream, &bound, &offset, &stride)) && !bound,
            "null stream record did not clear binding");
    }
    ComPtr<IDirect3DIndexBuffer9> bound_indices;
    Require(SUCCEEDED(device->GetIndices(&bound_indices)) && !bound_indices, "index unbind");
    std::puts("both archive draw functions rendered expected pixels; stream offsets and clearing verified");
}
void ShadowCollision(HWND window) {
    Camera camera(window,true,32,true), texture(window,true,32,true);
    RwCameraSetProjection(camera.value,rwPARALLEL);
    const RwV2d window_size{1,1};RwCameraSetViewWindow(camera.value,&window_size);
    RwCameraSetNearClipPlane(camera.value,1);
    RwRGBA clear{219,73,35,255},white{255,255,255,255};
    Require(RwCameraClear(camera.value,&clear,rwCAMERACLEARIMAGE) &&
        RwCameraClear(texture.value,&white,rwCAMERACLEARIMAGE),"shadow collision clear targets");
    RwBBox bounds{{2,2,3},{-2,-2,1}};
    struct WorldOwner { RpWorld* world;~WorldOwner() { if(world)RpWorldDestroy(world); } } owner{RpWorldCreate(&bounds)};
    Require(owner.world!=nullptr,"shadow collision world");
    auto* sector=reinterpret_cast<RpWorldSector*>(owner.world->rootSector);
    Require(sector && sector->type<0 && !sector->vertices && !sector->triangles,"empty leaf collision fixture");
    sector->vertices=static_cast<RwV3d*>(RwMalloc(3*sizeof(RwV3d),rwMEMHINTDUR_EVENT));
    sector->triangles=static_cast<RpTriangle*>(RwMalloc(sizeof(RpTriangle),rwMEMHINTDUR_EVENT));
    Require(sector->vertices && sector->triangles,"shadow collision sector storage");
    const RwV3d points[]{{-0.75f,-0.75f,2},{-0.75f,0.75f,2},{0.75f,-0.75f,2}};
    std::memcpy(sector->vertices,points,sizeof(points));
    *sector->triangles=RpTriangle{{0,1,2},0};sector->numVertices=3;sector->numTriangles=1;
    Require(RpCollisionWorldBuildData(owner.world,nullptr)==owner.world,"build actual SDK world collision data");
    RpIntersection intersection{};intersection.type=rpINTERSECTBOX;intersection.t.box=bounds;
    NativeCameraCollisionRenderRuntime runtime;
    Require(RwCameraBeginUpdate(camera.value)==camera.value,"shadow collision camera begin");
    RwRenderStateSet(rwRENDERSTATECULLMODE,reinterpret_cast<void*>(rwCULLMODECULLNONE));
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,reinterpret_cast<void*>(FALSE));
    Require(CameraCollisionRender::RenderWorld(reinterpret_cast<std::uint32_t>(camera.value),
        reinterpret_cast<std::uint32_t>(RwCameraGetRaster(texture.value)),reinterpret_cast<std::uint32_t>(owner.world),
        reinterpret_cast<std::uint32_t>(&intersection),1,0,0,runtime)==1,"shadow collision world render");
    Require(runtime.Statistic()==1 && runtime.VertexCount()==0,"shadow collision triangle visited and batch drained");
    Require(RwCameraEndUpdate(camera.value)==camera.value,"shadow collision camera end");
    auto* image=RwImageCreate(32,32,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera.value)),"shadow collision readback");
    unsigned dark=0,unchanged=0,unexpected=0;
    for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
        const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
        if(pixel->red==0 && pixel->green==0 && pixel->blue==0)++dark;
        else if(pixel->red==219 && pixel->green==73 && pixel->blue==35)++unchanged;
        else ++unexpected;
    }
    RwImageDestroy(image);
    std::printf("shadow collision: triangles=%u dark=%u unchanged=%u unexpected=%u\n",runtime.Statistic(),dark,unchanged,unexpected);
    Require(dark>100 && unchanged>100 && unexpected==0,"projected collision triangle darkens GPU target");
    // All triangle vertices have projected Z=2: depth attenuation is zero.
    // Their projected Y varies inside [0,1], so using Y would wrongly darken it.
    Require(RwCameraClear(camera.value,&clear,rwCAMERACLEARIMAGE) && RwCameraBeginUpdate(camera.value),"shadow depth attenuation begin");
    Require(CameraCollisionRender::RenderWorld(reinterpret_cast<std::uint32_t>(camera.value),
        reinterpret_cast<std::uint32_t>(RwCameraGetRaster(texture.value)),reinterpret_cast<std::uint32_t>(owner.world),
        reinterpret_cast<std::uint32_t>(&intersection),1,0,1,runtime)==1 && runtime.Statistic()==1,"shadow attenuated collision batch");
    Require(RwCameraEndUpdate(camera.value),"shadow depth attenuation end");
    image=RwImageCreate(32,32,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera.value)),"shadow depth attenuation readback");
    unsigned different=0;
    for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
        const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
        different+=pixel->red!=219 || pixel->green!=73 || pixel->blue!=35;
    }
    RwImageDestroy(image);
    Require(different==0,"shadow attenuation uses projected depth Z instead of texture Y");
    NativeHookClumpNames names;
    Require(names.Load("I:/Games/original/gamedata/Common/rpHookNameTable.hnt"),"composite shadow name table");
    NativeCameraRenderPassRuntime pass(names);NativeCameraRenderGeometryRuntime geometry;
    CameraRenderState state;state.active=1;state.camera=reinterpret_cast<std::uint32_t>(camera.value);
    state.world=reinterpret_cast<std::uint32_t>(owner.world);
    state.current_raster=reinterpret_cast<std::uint32_t>(RwCameraGetRaster(texture.value));
    const float target[]{0,0,2,1};state.target=reinterpret_cast<std::uint32_t>(target);
    CameraRenderGlobals globals;globals.render_enabled=true;globals.fog_strength=1;
    Require(RwCameraClear(camera.value,&clear,rwCAMERACLEARIMAGE) && RwCameraBeginUpdate(camera.value),"composite shadow projection begin");
    CameraRenderPass::Render(state,0,globals,geometry,pass);
    Require(RwCameraEndUpdate(camera.value),"composite shadow projection end");
    image=RwImageCreate(32,32,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,RwCameraGetRaster(camera.value)),"composite shadow projection readback");
    dark=0;
    for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
        const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
        dark+=pixel->red==0 && pixel->green==0 && pixel->blue==0;
    }
    RwImageDestroy(image);Require(dark>100,"CameraRenderPass normal branch projects actual collision triangles");
    auto* fog_raster=RwCameraGetRaster(camera.value);
    struct RestoreRaster { RwCamera* camera;RwRaster* raster;~RestoreRaster(){RwCameraSetRaster(camera,raster);} } restore{camera.value,fog_raster};
    RwRGBA black{0,0,0,255};Require(RwCameraClear(camera.value,&black,rwCAMERACLEARIMAGE),"fog clear");
    Require(pass.RenderFog(reinterpret_cast<std::uint32_t>(camera.value),reinterpret_cast<std::uint32_t>(fog_raster),0.5f,0,1,0)==1 &&
        !RwCameraGetRaster(camera.value),"fog overlay return and unbind");
    image=RwImageCreate(32,32,32);
    Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image,fog_raster),"fog overlay readback");
    different=0;
    for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
        const auto* pixel=reinterpret_cast<RwRGBA*>(RwImageGetPixels(image)+y*RwImageGetStride(image)+x*4);
        different+=pixel->red!=128 || pixel->green!=128 || pixel->blue!=128;
    }
    RwImageDestroy(image);Require(different==0,"fog x87 half rounds to128 and draws full raster");
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER,nullptr);
}
void ShadowTexture(HWND window) {
    Camera source(window, true, 64, true), first(window, true, 32, true),
        second(window, true, 32, true), auxiliary(window, true, 32, true);
    NativeCameraTexturePassRuntime runtime;
    for (auto* camera : {first.value, second.value}) {
        auto* raster = reinterpret_cast<RwRaster*>(CameraTexturePass::CreateSquareRaster(32, runtime));
        Require(raster != nullptr, "shared shadow raster creation");
        auto* previous = RwCameraGetRaster(camera);
        RwCameraSetRaster(camera, raster);
        RwRasterDestroy(previous);
    }
    RwRGBA source_color{219, 73, 35, 255}, empty{7, 13, 29, 255};
    Require(RwCameraClear(source.value, &source_color, rwCAMERACLEARIMAGE), "source clear");
    Require(RwCameraClear(first.value, &empty, rwCAMERACLEARIMAGE), "first clear");
    Require(RwCameraClear(second.value, &empty, rwCAMERACLEARIMAGE), "second clear");
    auto* auxiliary_raster = RwCameraGetRaster(auxiliary.value);
    // Shared passes borrow the camera and leave its framebuffer unbound.
    struct Restore {
        RwCamera* camera; RwRaster* raster;
        ~Restore() { RwCameraSetRaster(camera, raster); }
    } restore{auxiliary.value, auxiliary_raster};
    const auto address = [](const void* p) { return reinterpret_cast<std::uint32_t>(p); };
    auto verify = [&](RwRaster* raster) {
        Require(RwRasterGetWidth(raster) == 32 && RwRasterGetHeight(raster) == 32, "shadow target dimensions");
        auto* image = RwImageCreate(32, 32, 32);
        Require(image != nullptr, "shadow readback image");
        struct ImageOwner { RwImage* p; ~ImageOwner() { RwImageDestroy(p); } } owner{image};
        Require(RwImageAllocatePixels(image) && RwImageSetFromRaster(image, raster), "shadow GPU readback");
        for (int y = 0; y < 32; ++y) for (int x = 0; x < 32; ++x) {
            const auto* pixel = reinterpret_cast<RwRGBA*>(RwImageGetPixels(image) + y * RwImageGetStride(image) + x * 4);
            Require(pixel->red == source_color.red && pixel->green == source_color.green &&
                pixel->blue == source_color.blue, "shadow texture pass pixel mismatch");
        }
    };
    auto* first_raster = RwCameraGetRaster(first.value);
    auto* second_raster = RwCameraGetRaster(second.value);
    Require(CameraTexturePass::RenderToTexture(address(first_raster),
        address(RwCameraGetRaster(source.value)), address(auxiliary.value), runtime) == 1, "downsample result");
    Require(!RwCameraGetRaster(auxiliary.value), "downsample framebuffer unbound");
    verify(first_raster);
    Require(CameraTexturePass::RenderToTextureRepeated(address(first_raster), address(second_raster),
        address(auxiliary.value), 2, runtime) == 1, "two texture iterations result");
    Require(!RwCameraGetRaster(auxiliary.value), "repeated framebuffer unbound");
    verify(first_raster); verify(second_raster);
    Require(CameraTexturePass::RenderToTextureRepeated(address(first_raster), address(second_raster),
        address(auxiliary.value), 0, runtime) == 1 && !RwCameraGetRaster(auxiliary.value), "zero iterations unbound");
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    std::puts("shadow texture: 64->32 downsample and two ping-pong iterations preserve all RGB pixels; framebuffer unbound");
}
}
void RegisterShadowCollisionPlugin() { Require(RpCollisionPluginAttach()!=FALSE,"shadow collision plugin"); }
void RunPresentationCase(std::string_view name, HWND window) {
    if (name == "presentation_shadow_collision") ShadowCollision(window);
    else if (name == "presentation_shadow_texture") ShadowTexture(window);
    else if (name == "presentation_policy") Policy(window);
    else if (name == "presentation_getter") Getter(window);
    else if (name == "presentation_projection") Projection(window);
    else if (name == "presentation_draw") Draw(window);
    else if (name == "presentation_sdk_vertex_buffer") Draw(window, true);
    else if (name == "presentation_comparison_observer") Draw(window,false,true);
    else throw std::runtime_error("unknown presentation case");
}
}

