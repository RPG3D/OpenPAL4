#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <d3d9.h>
#include <wrl/client.h>
#include "support/d3d9_device_forwarder.h"
#include "pal4/clump_bounds_render_native.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <vector>

extern "C" {
extern IDirect3DDevice9 *_RwD3DDevice;
RwBool _rwD3D9Im3DRenderOpen();
void _rwD3D9Im3DRenderClose();
RxNodeDefinition *RxNodeDefinitionGetD3D9ImmInstance();
RxNodeDefinition *RxNodeDefinitionGetD3D9SubmitNoLight();
}
namespace {
using Microsoft::WRL::ComPtr;
static_assert(sizeof(RxNodeDefinition) == 64 && sizeof(RxPipelineNodeParam) == 8);
static_assert(sizeof(rwIm3DPool) == 60 && sizeof(RwIm3DVertex) == 36 &&
              offsetof(RwIm3DVertex, color) == 24);
void Require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
struct Session {
    HWND window{};
    Session() {
        Require(RwEngineInit(nullptr, 0, 0), "init");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 Im3D submit", WS_OVERLAPPEDWINDOW, 64, 64, nullptr,
                               nullptr, GetModuleHandleA(nullptr), nullptr);
        Require(window, "window");
        RwEngineOpenParams p{window};
        Require(RwEngineOpen(&p) && RwEngineStart(), "engine start");
    }
    ~Session() {
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        DestroyWindow(window);
    }
};
struct Camera {
    RwCamera *value = RwCameraCreate();
    RwFrame *frame = RwFrameCreate();
    RwRaster *color{}, *depth{};
    explicit Camera(HWND window) {
        Require(value && frame, "im3d camera/frame");
        RwCameraSetFrame(value, frame);
        color = RwRasterCreate(32, 32, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
        depth = RwRasterCreate(32, 32, 0, rwRASTERTYPEZBUFFER);
        Require(color && depth, "im3d targets");
        RwCameraSetRaster(value, color);
        RwCameraSetZRaster(value, depth);
        RwCameraSetNearClipPlane(value, 1);
        RwCameraSetFarClipPlane(value, 10);
        const RwV2d view{1, 1};
        RwCameraSetViewWindow(value, &view);
        Require(RwCameraBeginUpdate(value) && RwCameraEndUpdate(value) &&
                    RwCameraShowRaster(value, window, 0),
                "im3d initial present policy");
    }
    ~Camera() {
        RwCameraSetFrame(value, nullptr);
        RwCameraSetRaster(value, nullptr);
        RwCameraSetZRaster(value, nullptr);
        RwCameraDestroy(value);
        RwFrameDestroy(frame);
        RwRasterDestroy(color);
        RwRasterDestroy(depth);
    }
    void Begin() {
        RwRGBA black{0, 0, 0, 255};
        Require(RwCameraClear(value, &black, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) &&
                    RwCameraBeginUpdate(value),
                "im3d clear/begin");
        RwD3D9SetVertexShader(nullptr);
        RwD3D9SetPixelShader(nullptr);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, reinterpret_cast<void *>(FALSE));
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, reinterpret_cast<void *>(FALSE));
        RwRenderStateSet(rwRENDERSTATECULLMODE, reinterpret_cast<void *>(rwCULLMODECULLNONE));
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, reinterpret_cast<void *>(FALSE));
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, reinterpret_cast<void *>(FALSE));
        RwD3D9SetRenderState(D3DRS_LIGHTING, FALSE);
        RwD3D9SetRenderState(D3DRS_STENCILENABLE, FALSE);
        RwD3D9SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }
    std::vector<RwRGBA> Read(HWND window) {
        Require(RwCameraEndUpdate(value) && RwCameraShowRaster(value, window, 0), "im3d end/present");
        auto *image = RwImageCreate(32, 32, 32);
        Require(image && RwImageAllocatePixels(image) && RwImageSetFromRaster(image, color),
                "im3d image readback");
        std::vector<RwRGBA> out(1024);
        for (int y = 0; y < 32; ++y)
            std::memcpy(out.data() + 32 * y, RwImageGetPixels(image) + y * RwImageGetStride(image), 128);
        RwImageDestroy(image);
        return out;
    }
};

class DeviceProbe final : public pal4::test::D3D9DeviceForwarder {
  public:
    bool rejectBuffered{}, rejectUP{}, rejectIB{}, rejectDeclaration{};
    unsigned primitive{}, indexed{}, primitiveUP{}, indexedUP{}, indexCreates{}, declarationCreates{};
    UINT lastStride{}, ibSize{};
    DWORD ibUsage{};
    D3DPOOL ibPool{};
    std::vector<UINT> indexStarts;
    DeviceProbe() : D3D9DeviceForwarder(_RwD3DDevice) { _RwD3DDevice = this; }
    ~DeviceProbe() { _RwD3DDevice = original_; }
    HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE t, UINT start, UINT count) override {
        ++primitive;
        return rejectBuffered ? D3DERR_INVALIDCALL : original_->DrawPrimitive(t, start, count);
    }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE t, INT base, UINT min, UINT vertices,
                                                   UINT start, UINT count) override {
        ++indexed;
        indexStarts.push_back(start);
        return rejectBuffered ? D3DERR_INVALIDCALL
                              : original_->DrawIndexedPrimitive(t, base, min, vertices, start, count);
    }
    HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE t, UINT count, const void *data,
                                              UINT stride) override {
        ++primitiveUP;
        lastStride = stride;
        return rejectUP ? D3DERR_INVALIDCALL : original_->DrawPrimitiveUP(t, count, data, stride);
    }
    HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE t, UINT min, UINT vertices,
                                                     UINT count, const void *idx, D3DFORMAT format,
                                                     const void *data, UINT stride) override {
        ++indexedUP;
        lastStride = stride;
        return rejectUP ? D3DERR_INVALIDCALL
                        : original_->DrawIndexedPrimitiveUP(t, min, vertices, count, idx, format, data,
                                                            stride);
    }
    HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT length, DWORD usage, D3DFORMAT format, D3DPOOL pool,
                                                IDirect3DIndexBuffer9 **out, HANDLE *handle) override {
        ++indexCreates;
        ibSize = length;
        ibUsage = usage;
        ibPool = pool;
        if (rejectIB) {
            *out = nullptr;
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        return original_->CreateIndexBuffer(length, usage, format, pool, out, handle);
    }
    HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const D3DVERTEXELEMENT9 *elements,
                                                      IDirect3DVertexDeclaration9 **out) override {
        ++declarationCreates;
        if (rejectDeclaration) {
            *out = nullptr;
            return D3DERR_OUTOFVIDEOMEMORY;
        }
        return original_->CreateVertexDeclaration(elements, out);
    }
};
std::vector<RwIm3DVertex> Vertices(int count = 3) {
    std::vector<RwIm3DVertex> v(count);
    constexpr RwV3d position[] = {{-1, -1, 2}, {1, -1, 2}, {0, 1, 2}};
    for (int i = 0; i < count; ++i) {
        v[i].objVertex = position[i % 3];
        v[i].objNormal = {0, 0, 1};
        v[i].color = 0xffdb4923;
        v[i].u = .25f;
        v[i].v = .75f;
    }
    return v;
}
void Draw(std::vector<RwIm3DVertex> &vertices, int indexCount = -1, RwUInt32 flags = rwIM3D_ALLOPAQUE,
          RwMatrix *matrix = nullptr) {
    std::vector<RwImVertexIndex> indices(indexCount < 0 ? 0 : indexCount);
    for (size_t i = 0; i < indices.size(); ++i)
        indices[i] = i % 3;
    Require(RwIm3DTransform(vertices.data(), static_cast<RwUInt32>(vertices.size()), matrix, flags) !=
                nullptr,
            "transform");
    Require(_rwIm3DGetPool()->elements == vertices.data(), "public pool retains input until submission");
    Require(indexCount < 0 ? RwIm3DRenderPrimitive(rwPRIMTYPETRILIST)
                           : RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, indices.data(), indexCount),
            "public render result");
    Require(RwIm3DEnd(), "end");
}
void Center(const std::vector<RwRGBA> &p, bool drawn = true) {
    const auto c = p[16 * 32 + 16];
    Require(c.red == (drawn ? 219 : 0) && c.green == (drawn ? 73 : 0) && c.blue == (drawn ? 35 : 0),
            "center color");
    Require(p[0].red == 0 && p[0].green == 0 && p[0].blue == 0, "outside clear");
}
void Nodes() {
    auto *instance = RxNodeDefinitionGetD3D9ImmInstance();
    auto *submit = RxNodeDefinitionGetD3D9SubmitNoLight();
    Require(instance && submit && instance != submit, "distinct static nodes");
    std::printf("nodes: %s / %s\n", instance->name, submit->name);
    Require(std::strcmp(instance->name, "ImmInstance.csl") == 0 &&
                std::strcmp(submit->name, "nodeD3D9SubmitNoLight.csl") == 0,
            "node identities");
    Require(instance->nodeMethods.nodeBody && submit->nodeMethods.nodeBody &&
                instance->io.numClustersOfInterest == 0 && submit->io.numClustersOfInterest == 0,
            "no cluster dependencies");
    Require(RwIm3DGetTransformPipeline()->nodes[0].nodeDef == instance &&
                RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST)->nodes[0].nodeDef == submit,
            "platform pipelines use these nodes");
    Require(instance == RxNodeDefinitionGetD3D9ImmInstance() &&
                submit == RxNodeDefinitionGetD3D9SubmitNoLight(),
            "stable getter pointers");
}
void PipelineLifecycle() {
    auto *transform = RwIm3DGetTransformPipeline();
    auto *render = RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST);
    Require(transform && render && transform != render, "separate Im3D pipelines");
    Require(!transform->locked && !render->locked && transform->numNodes == 1 &&
                render->numNodes == 1 && transform->pluginId == 1 && render->pluginId == 1,
            "unlocked single-node platform pipelines");
    Require(transform->nodes[0].nodeDef == RxNodeDefinitionGetD3D9ImmInstance() &&
                render->nodes[0].nodeDef == RxNodeDefinitionGetD3D9SubmitNoLight(),
            "platform node binding");
    for (auto primitive : {rwPRIMTYPETRILIST, rwPRIMTYPETRIFAN, rwPRIMTYPETRISTRIP,
                           rwPRIMTYPELINELIST, rwPRIMTYPEPOLYLINE})
        Require(RwIm3DGetRenderPipeline(primitive) == render, "five primitive routes share render pipeline");

    Require(RwEngineStop(), "stop Im3D pipeline owner");
    Require(RwEngineStart(), "restart Im3D pipeline owner");
    transform = RwIm3DGetTransformPipeline();
    render = RwIm3DGetRenderPipeline(rwPRIMTYPETRILIST);
    Require(transform && render && transform->numNodes == 1 && render->numNodes == 1 &&
                transform->nodes[0].nodeDef == RxNodeDefinitionGetD3D9ImmInstance() &&
                render->nodes[0].nodeDef == RxNodeDefinitionGetD3D9SubmitNoLight(),
            "recreated platform pipelines after engine restart");
}
void Routes(HWND window) {
    Camera camera(window);
    for (auto [vertices, indices] :
         std::array<std::pair<int, int>, 4>{{{3, 3}, {4, 6}, {9, 9}, {9, 6}}}) {
        auto v = Vertices(vertices);
        const auto old = v;
        camera.Begin();
        DeviceProbe probe;
        Draw(v, indices);
        const bool indexed = indices > vertices || indices > 8;
        Require(probe.indexed == unsigned(indexed) && probe.primitive == unsigned(!indexed) &&
                    !probe.primitiveUP && !probe.indexedUP,
                "eight-index and vertex-count route threshold");
        ComPtr<IDirect3DIndexBuffer9> ib;
        Require(SUCCEEDED(probe.GetIndices(&ib)), "bound index buffer");
        if (indexed) {
            D3DINDEXBUFFER_DESC d{};
            Require(ib && SUCCEEDED(ib->GetDesc(&d)) && d.Size == 131070 && d.Format == D3DFMT_INDEX16,
                    "65535 index capacity");
        }
        Require(!std::memcmp(v.data(), old.data(), v.size() * sizeof(v[0])),
                "source vertices unchanged");
        Center(camera.Read(window));
    }
    camera.Begin();
    DeviceProbe probe;
    auto v = Vertices();
    Require(RwIm3DTransform(v.data(), 0, nullptr, rwIM3D_ALLOPAQUE) &&
                RwIm3DRenderPrimitive(rwPRIMTYPETRILIST) && RwIm3DEnd(),
            "zero pool succeeds without a draw");
    Require(!probe.primitive && !probe.indexed && !probe.primitiveUP && !probe.indexedUP,
            "zero vertex no draw calls");
    Center(camera.Read(window), false);
}
void State(HWND window) {
    Camera camera(window);
    auto v = Vertices();
    for (bool uv : {false, true}) {
        camera.Begin();
        DeviceProbe probe;
        RwD3D9SetRenderState(D3DRS_LIGHTING, TRUE);
        RwD3D9SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
        Draw(v, -1, rwIM3D_ALLOPAQUE | (uv ? rwIM3D_VERTEXUV | rwIM3D_NOCLIP : 0));
        ComPtr<IDirect3DVertexBuffer9> vb;
        UINT offset{}, stride{};
        Require(SUCCEEDED(probe.GetStreamSource(0, &vb, &offset, &stride)) && vb &&
                    stride == (uv ? 24u : 16u),
                "packed vertex stride");
        ComPtr<IDirect3DVertexDeclaration9> decl;
        Require(SUCCEEDED(probe.GetVertexDeclaration(&decl)) && decl, "packed declaration");
        std::array<D3DVERTEXELEMENT9, 5> e{};
        UINT n = 5;
        Require(SUCCEEDED(decl->GetDeclaration(e.data(), &n)) && n == (uv ? 4u : 3u),
                "packed declaration size");
        Require(e[0].Type == D3DDECLTYPE_FLOAT3 && e[0].Offset == 0 &&
                    e[1].Type == D3DDECLTYPE_D3DCOLOR && e[1].Offset == 12,
                "packed position/color");
        if (uv)
            Require(e[2].Type == D3DDECLTYPE_FLOAT2 && e[2].Offset == 16, "packed UV");
        DWORD lighting{}, normal{}, clip{}, alpha{};
        probe.GetRenderState(D3DRS_LIGHTING, &lighting);
        probe.GetRenderState(D3DRS_NORMALIZENORMALS, &normal);
        probe.GetRenderState(D3DRS_CLIPPING, &clip);
        probe.GetRenderState(D3DRS_ALPHABLENDENABLE, &alpha);
        Require(!lighting && !normal && clip == unsigned(!uv) && !alpha, "fixed function flags");
        Center(camera.Read(window));
    }
    auto *texture = RwRasterCreate(1, 1, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
    Require(texture, "state texture");
    auto *texels = RwRasterLock(texture, 0, rwRASTERLOCKWRITE);
    Require(texels, "state texture pixels");
    const DWORD texel = 0xff804020;
    std::memcpy(texels, &texel, 4);
    RwRasterUnlock(texture);
    camera.Begin();
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, texture);
    Draw(v, -1, rwIM3D_VERTEXUV | rwIM3D_ALLOPAQUE);
    DWORD colorOp{}, alphaOp{};
    _RwD3DDevice->GetTextureStageState(0, D3DTSS_COLOROP, &colorOp);
    _RwD3DDevice->GetTextureStageState(0, D3DTSS_ALPHAOP, &alphaOp);
    Require(colorOp == D3DTOP_MODULATE && alphaOp == D3DTOP_MODULATE, "texture stage modulation");
    auto textured = camera.Read(window)[16 * 32 + 16];
    Require(textured.red >= 109 && textured.red <= 110 && textured.green >= 18 && textured.green <= 19 &&
                textured.blue >= 4 && textured.blue <= 5,
            "real texture multiplied by vertex color");
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nullptr);
    RwRasterDestroy(texture);
    camera.Begin();
    DeviceProbe probe;
    RwMatrix matrix{};
    RwMatrixSetIdentity(&matrix);
    matrix.pos.x = .25f;
    RwMatrixUpdate(&matrix);
    Draw(v, -1, 0, &matrix);
    D3DMATRIX world{};
    DWORD alpha{};
    Require(SUCCEEDED(probe.GetTransform(D3DTS_WORLD, &world)) && world._41 == .25f,
            "supplied world matrix");
    probe.GetRenderState(D3DRS_ALPHABLENDENABLE, &alpha);
    Require(alpha, "absence of ALLOPAQUE enables vertex alpha");
    camera.Read(window);
}
void Ring(HWND window) {
    Camera camera(window);
    camera.Begin();
    DeviceProbe probe;
    auto v = Vertices();
    for (int i = 0; i < 3; ++i)
        Draw(v, 30000);
    Require(probe.indexStarts == std::vector<UINT>({0, 30000, 0}),
            "65535-index ring wraps before third draw");
    Center(camera.Read(window));
}
void Fallback(HWND window) {
    Camera camera(window);
    auto v = Vertices();
    camera.Begin();
    {
        DeviceProbe probe;
        Draw(v, 65538);
        Require(!probe.indexed && probe.indexedUP == 1 && probe.lastStride == 36,
                "oversized index request falls back to indexed UP");
    }
    Center(camera.Read(window));
    for (int indices : {-1, 3, 6}) {
        camera.Begin();
        DeviceProbe probe;
        probe.rejectBuffered = true;
        Draw(v, indices);
        Require(probe.primitive + probe.indexed == 1 && probe.primitiveUP + probe.indexedUP == 1 &&
                    probe.lastStride == 36,
                "buffered failure retries original36-byte data");
        ComPtr<IDirect3DVertexDeclaration9> decl;
        probe.GetVertexDeclaration(&decl);
        std::array<D3DVERTEXELEMENT9, 5> e{};
        UINT n = 5;
        Require(decl && SUCCEEDED(decl->GetDeclaration(e.data(), &n)) && n == 5 &&
                    e[1].Usage == D3DDECLUSAGE_NORMAL && e[2].Offset == 24 && e[3].Offset == 28,
                "UP legacy declaration includes normal/color/UV");
        Center(camera.Read(window));
    }
}
void Failures(HWND window) {
    Camera camera(window);
    auto v = Vertices();
    for (int indices : {-1, 3, 6}) {
        camera.Begin();
        DeviceProbe probe;
        probe.rejectBuffered = probe.rejectUP = true;
        Draw(v, indices);
        Require(probe.primitive + probe.indexed == 1 && probe.primitiveUP + probe.indexedUP == 1,
                "both attempts rejected while public API reports success");
        Center(camera.Read(window), false);
    }
}
void OpenFailure() {
    _rwD3D9Im3DRenderClose();
    _rwD3D9Im3DRenderClose();
    {
        DeviceProbe probe;
        probe.rejectIB = true;
        Require(!_rwD3D9Im3DRenderOpen() && probe.indexCreates == 1 && !probe.declarationCreates,
                "IB failure returnsfalse before declarations");
        Require(probe.ibSize == 131070 && probe.ibUsage == (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY),
                "IB creation contract");
    }
    {
        DeviceProbe probe;
        probe.rejectDeclaration = true;
        Require(_rwD3D9Im3DRenderOpen() && probe.indexCreates == 1 && probe.declarationCreates == 3,
                "three declaration failures ignored by open");
    }
    _rwD3D9Im3DRenderClose();
    Require(_rwD3D9Im3DRenderOpen(), "restore real declarations and index buffer");
}
void Game(HWND window) {
    Camera camera(window);
    pal4::ClumpBoundsState box{};
    box.minimum = {-.5f, -.5f, 2};
    box.maximum = {.5f, .5f, 3};
    pal4::NativeClumpBoundsRenderRuntime runtime;
    for (bool reject : {false, true}) {
        camera.Begin();
        DeviceProbe probe;
        probe.rejectBuffered = probe.rejectUP = reject;
        Require(pal4::ClumpSpatialBounds::RenderBoundingBox(box, 0xff2349db, 0, runtime) != 0,
                "game returns shade-state result despite draw failure");
        Require(probe.indexed == 1 && probe.indexedUP == unsigned(reject) && !_rwIm3DGetPool()->elements,
                "game twelve edges use IB and End even after hidden draw failures");
        auto pixels = camera.Read(window);
        unsigned colored = 0;
        for (auto c : pixels)
            if (c.red == 219 && c.green == 73 && c.blue == 35)
                ++colored;
        Require(reject ? colored == 0 : colored > 20, "game box pixels follow device result");
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        Require(argc == 2, "one case");
        Session session;
        std::string_view name = argv[1];
        if (name == "nodes")
            Nodes();
        else if (name == "pipeline_lifecycle")
            PipelineLifecycle();
        else if (name == "routes")
            Routes(session.window);
        else if (name == "state")
            State(session.window);
        else if (name == "ring")
            Ring(session.window);
        else if (name == "fallback")
            Fallback(session.window);
        else if (name == "failures")
            Failures(session.window);
        else if (name == "open_failure")
            OpenFailure();
        else if (name == "game")
            Game(session.window);
        else
            throw std::runtime_error("unknown case");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
