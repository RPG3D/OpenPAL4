#include "renderware_state_cases.h"
#include "pal4/d3d_capability_native.h"
#include <windows.h>
// Preserve actual archive functions instead of the D3D-aware inline macros.
#include <rwcore.h>
#include <d3d9.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace pal4::test {
namespace {
using Microsoft::WRL::ComPtr;
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
static_assert(sizeof(D3DCAPS9) == 304 && sizeof(D3DLIGHT9) == 104);
static_assert(std::is_same_v<decltype(&RwD3D9GetTransform), void (__cdecl *)(RwUInt32, void*)>);
static_assert(std::is_same_v<decltype(&RwD3D9DeleteVertexDeclaration), void (__cdecl *)(void*)>);
IDirect3DDevice9* Device() { return static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice()); }
D3DMATRIX Identity() { D3DMATRIX m{}; m._11 = m._22 = m._33 = m._44 = 1; return m; }
void Same(const D3DMATRIX& a, const D3DMATRIX& b) {
    Require(std::memcmp(&a, &b, sizeof(a)) == 0, "transform bit pattern mismatch");
}
void Transforms() {
    auto* device = Device(); const auto identity = Identity();
    auto matrix = identity; matrix._41 = 3; matrix._42 = -7; matrix._43 = 11;
    D3DMATRIX cached{}, actual{};
    RwD3D9GetTransform(D3DTS_TEXTURE0, &cached); Same(cached, identity);
    RwD3D9GetTransform(D3DTS_VIEW, nullptr); // Documented body no-op.
    Require(RwD3D9SetTransform(D3DTS_VIEW, &matrix), "set view transform");
    Require(SUCCEEDED(device->GetTransform(D3DTS_VIEW, &actual)), "COM transform query"); Same(actual, matrix);
    // A cache hit does not repair external mutation; null means identity.
    Require(SUCCEEDED(device->SetTransform(D3DTS_VIEW, &identity)), "external transform mutation");
    Require(RwD3D9SetTransform(D3DTS_VIEW, &matrix), "same transform cache hit");
    RwD3D9GetTransform(D3DTS_VIEW, &cached); Same(cached, matrix);
    device->GetTransform(D3DTS_VIEW, &actual); Same(actual, identity);
    matrix._12 = -0.0f; Require(RwD3D9SetTransform(D3DTS_VIEW, &matrix), "bitwise negative-zero change");
    device->GetTransform(D3DTS_VIEW, &actual); Same(actual, matrix);
    Require(RwD3D9SetTransform(D3DTS_VIEW, nullptr), "null identity transform");
    device->GetTransform(D3DTS_VIEW, &actual); Same(actual, identity);
    RwMatrix world{}; RwMatrixSetIdentity(&world);
    world.pos = {5, 6, 7}; // Identity flag deliberately retained: SDK trusts it.
    Require(RwD3D9SetTransformWorld(&world), "identity-flag world");
    device->GetTransform(D3DTS_WORLD, &actual); Same(actual, identity);
    world.flags = 0;
    Require(RwD3D9SetTransformWorld(&world), "nonidentity world");
    auto expected = identity; expected._41 = 5; expected._42 = 6; expected._43 = 7;
    device->GetTransform(D3DTS_WORLD, &actual); Same(actual, expected);
    Require(RwD3D9SetTransformWorld(nullptr), "null world reset");
    device->GetTransform(D3DTS_WORLD, &actual); Same(actual, identity);
    std::puts("transform cache: bitwise comparison, external divergence and identity flags verified");
}
D3DLIGHT9 Light(D3DLIGHTTYPE type) {
    D3DLIGHT9 light{}; light.Type = type; light.Diffuse = {0.3f, 0.4f, 0.5f, 1};
    light.Direction = {0, 0, 1}; light.Position = {1, 2, 3}; light.Range = 100;
    light.Attenuation0 = 1; light.Falloff = 1; light.Theta = 0.2f; light.Phi = 0.6f;
    return light;
}
void Lights() {
    auto* device = Device();
    Require(RwD3D9EnableLight(4, TRUE), "unknown light enable no-op return");
    BOOL enabled{}; Require(FAILED(device->GetLightEnable(4, &enabled)), "unknown light unexpectedly created");
    auto light = Light(D3DLIGHT_POINT); Require(RwD3D9SetLight(0, &light), "create point light");
    Require(RwD3D9EnableLight(0, TRUE), "enable point");
    Require(SUCCEEDED(device->GetLightEnable(0, &enabled)) && enabled, "actual point enable");
    auto ignored = light; ignored.Ambient.r = 0.8f; ignored.Specular.g = 0.9f;
    ignored.Diffuse.a = 0.25f; ignored.Attenuation0 = 0.5f;
    Require(RwD3D9SetLight(0, &ignored), "ignored-field light update");
    D3DLIGHT9 actual{}; Require(SUCCEEDED(device->GetLight(0, &actual)), "point readback");
    Require(actual.Ambient.r == light.Ambient.r && actual.Specular.g == light.Specular.g &&
        actual.Diffuse.a == light.Diffuse.a && actual.Attenuation0 == light.Attenuation0,
        "comparator unexpectedly included ignored fields");
    ignored.Position.x += 1; Require(RwD3D9SetLight(0, &ignored), "significant point update");
    device->GetLight(0, &actual); Require(actual.Position.x == ignored.Position.x &&
        actual.Ambient.r == ignored.Ambient.r && actual.Attenuation0 == ignored.Attenuation0,
        "significant update failed to copy whole light");
    Require(RwD3D9EnableLight(0, FALSE), "disable point");
    std::puts("light diagnostic: equality ignores ambient/specular/alpha/attenuation; full copy occurs on significant change");
}
const D3DVERTEXELEMENT9 declaration[] = {
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};
struct Declaration {
    void* value{};
    ~Declaration() { if (value) RwD3D9DeleteVertexDeclaration(value); }
};
void Declarations() {
    auto* device = Device(); Declaration a, b, recreated;
    Require(RwD3D9CreateVertexDeclaration(declaration, &a.value) && a.value, "create declaration");
    Require(RwD3D9CreateVertexDeclaration(declaration, &b.value) && b.value == a.value, "declaration cache identity");
    RwD3D9SetVertexDeclaration(a.value);
    ComPtr<IDirect3DVertexDeclaration9> bound;
    Require(SUCCEEDED(device->GetVertexDeclaration(&bound)) && bound.Get() == a.value, "declaration COM bind");
    RwD3D9SetVertexDeclaration(nullptr); bound.Reset();
    RwD3D9DeleteVertexDeclaration(a.value); a.value = nullptr;
    UINT count{}; Require(SUCCEEDED(static_cast<IDirect3DVertexDeclaration9*>(b.value)->GetDeclaration(nullptr, &count)) && count == 2,
        "first delete invalidated second owner");
    RwD3D9DeleteVertexDeclaration(b.value); b.value = nullptr;
    Require(RwD3D9CreateVertexDeclaration(declaration, &recreated.value), "recreate zero-reference declaration cache entry");
    std::array<D3DVERTEXELEMENT9, 2> returned{}; count = 2;
    Require(SUCCEEDED(static_cast<IDirect3DVertexDeclaration9*>(recreated.value)->GetDeclaration(returned.data(), &count)) &&
        count == 2 && std::memcmp(returned.data(), declaration, sizeof(declaration)) == 0, "recreated declaration bytes");
    // Unknown COM declarations are not owned by the SDK cache; Delete ignores them.
    const D3DVERTEXELEMENT9 external_elements[]{
        {0, 0, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};
    ComPtr<IDirect3DVertexDeclaration9> external;
    Require(SUCCEEDED(device->CreateVertexDeclaration(external_elements, &external)), "external declaration");
    const auto retained = external->AddRef();
    RwD3D9DeleteVertexDeclaration(external.Get());
    Require(external->Release() == retained - 1, "SDK deleted an unregistered declaration");
    void* invalid = reinterpret_cast<void*>(1);
    const D3DVERTEXELEMENT9 bad[]{
        {0, 0, 0xfe, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0}, D3DDECL_END()};
    Require(!RwD3D9CreateVertexDeclaration(bad, &invalid) && invalid == nullptr,
        "invalid declaration does not clear output");
    ComPtr<IDirect3DIndexBuffer9> indices; void* raw{};
    Require(RwD3D9IndexBufferCreate(7, &raw) && raw, "index buffer create");
    indices.Attach(static_cast<IDirect3DIndexBuffer9*>(raw));
    D3DINDEXBUFFER_DESC desc{}; Require(SUCCEEDED(indices->GetDesc(&desc)) &&
        desc.Size == 14 && desc.Format == D3DFMT_INDEX16 && desc.Pool == D3DPOOL_MANAGED &&
        desc.Usage == D3DUSAGE_WRITEONLY, "index buffer descriptor ABI");
    std::puts("declaration cache sharing/recreation/unknown-delete/failure and index-buffer descriptor verified");
}
void Shaders() {
    auto* device = Device();
    D3DCAPS9 actual{}; Require(SUCCEEDED(device->GetDeviceCaps(&actual)), "device caps");
    const auto* cached = static_cast<const D3DCAPS9*>(RwD3D9GetCaps());
    Require(cached && RwD3D9GetCaps() == cached, "borrowed caps identity");
    ComPtr<IDirect3D9> parent; D3DDEVICE_CREATION_PARAMETERS creation{}; D3DCAPS9 adapter{};
    Require(SUCCEEDED(device->GetDirect3D(&parent)) && SUCCEEDED(device->GetCreationParameters(&creation)) &&
        SUCCEEDED(parent->GetDeviceCaps(creation.AdapterOrdinal, creation.DeviceType, &adapter)), "adapter caps");
    Require(std::memcmp(cached, &adapter, sizeof(adapter)) == 0, "RW caps must match adapter query");
    for (size_t i = 0; i < sizeof(actual) / 4; ++i)
        if (reinterpret_cast<const DWORD*>(cached)[i] != reinterpret_cast<const DWORD*>(&actual)[i])
            std::printf("caps difference +%zu: RW=%08lX COM=%08lX\n", i * 4,
                reinterpret_cast<const DWORD*>(cached)[i], reinterpret_cast<const DWORD*>(&actual)[i]);
    NativeD3DCapabilityRuntime runtime;
    Require(D3DCapabilityChecker::CheckVertexShaderVersion(actual.VertexShaderVersion, runtime) &&
        !D3DCapabilityChecker::CheckVertexShaderVersion(actual.VertexShaderVersion + 1, runtime) &&
        D3DCapabilityChecker::CheckPixelShaderVersion(actual.PixelShaderVersion, runtime) &&
        !D3DCapabilityChecker::CheckPixelShaderVersion(actual.PixelShaderVersion + 1, runtime),
        "actual game capability checks/native device adapter");
    // D3D9 bytecode: vs_2_0 dcl_position v0; mov oPos,v0. ps_1_1 mov r0,v0.
    const RwUInt32 vs_code[]{0xfffe0200, 0x0200001f, 0x80000000, 0x900f0000,
        0x02000001, 0xc00f0000, 0x90e40000, 0xffff};
    const RwUInt32 ps_code[]{0xffff0101, 1, 0x800f0000, 0x90e40000, 0xffff};
    void *vertex{}, *pixel{};
    Require(RwD3D9CreateVertexShader(vs_code, &vertex) && vertex, "vertex shader creation");
    struct VS { void* value; ~VS() { RwD3D9DeleteVertexShader(value); } } vs{vertex};
    Require(RwD3D9CreatePixelShader(ps_code, &pixel) && pixel, "pixel shader creation");
    struct PS { void* value; ~PS() { RwD3D9DeletePixelShader(value); } } ps{pixel};
    RwD3D9SetVertexShader(vertex); RwD3D9SetPixelShader(pixel);
    ComPtr<IDirect3DVertexShader9> bound_vs; ComPtr<IDirect3DPixelShader9> bound_ps;
    Require(SUCCEEDED(device->GetVertexShader(&bound_vs)) && bound_vs.Get() == vertex &&
        SUCCEEDED(device->GetPixelShader(&bound_ps)) && bound_ps.Get() == pixel, "shader COM binding");
    UINT vs_size{}, ps_size{}; bound_vs->GetFunction(nullptr, &vs_size); bound_ps->GetFunction(nullptr, &ps_size);
    Require(vs_size == sizeof(vs_code) && ps_size == sizeof(ps_code), "shader function sizes");
    std::array<RwUInt32, 8> returned{};
    Require(SUCCEEDED(bound_vs->GetFunction(returned.data(), &vs_size)) &&
        std::memcmp(returned.data(), vs_code, vs_size) == 0, "vertex bytecode readback");
    Require(SUCCEEDED(bound_ps->GetFunction(returned.data(), &ps_size)) &&
        std::memcmp(returned.data(), ps_code, ps_size) == 0, "pixel bytecode readback");
    RwD3D9SetVertexShader(nullptr); RwD3D9SetPixelShader(nullptr);
    void* wrong_stage{}; Require(!RwD3D9CreateVertexShader(ps_code, &wrong_stage), "pixel bytecode accepted as vertex shader");
    Require(!RwD3D9CreatePixelShader(vs_code, &wrong_stage), "vertex bytecode accepted as pixel shader");
    std::puts("caps agree with adapter, not necessarily created device; native vertex/pixel create/bind/read/delete and wrong-stage failure verified");
}
void Frustum() {
    struct Camera {
        RwCamera* value = RwCameraCreate(); RwFrame* frame = RwFrameCreate();
        Camera() { Require(value && frame, "frustum camera"); RwCameraSetFrame(value, frame); }
        ~Camera() { RwCameraSetFrame(value, nullptr); RwCameraDestroy(value); RwFrameDestroy(frame); }
    } camera;
    // Direct SDK camera plane fixture isolates the two tests from projection setup.
    // Outward normals define the inclusive cube [-1,1]^3.
    for (int i = 0; i < 6; ++i) {
        auto& p = camera.value->frustumPlanes[i]; p = {};
        float* normal = &p.plane.normal.x; normal[i / 2] = i % 2 ? -1.0f : 1.0f;
        p.plane.distance = 1;
        p.closestX = p.plane.normal.x > 0; p.closestY = p.plane.normal.y > 0; p.closestZ = p.plane.normal.z > 0;
    }
    RwSphere sphere{{0, 0, 0}, 1}; Require(RwD3D9CameraIsSphereFullyInsideFrustum(camera.value, &sphere), "tangent sphere excluded");
    sphere.radius = std::nextafter(1.0f, 2.0f);
    Require(!RwD3D9CameraIsSphereFullyInsideFrustum(camera.value, &sphere), "intersecting sphere counted fully inside");
    sphere = {{0, 0, 0}, 0.25f}; sphere.center.x = 0.76f;
    Require(!RwD3D9CameraIsSphereFullyInsideFrustum(camera.value, &sphere), "partial sphere containment");
    RwBBox box{{1, 1, 1}, {-1, -1, -1}};
    Require(RwD3D9CameraIsBBoxFullyInsideFrustum(camera.value, &box), "tangent box excluded");
    box.sup.x = std::nextafter(1.0f, 2.0f);
    Require(!RwD3D9CameraIsBBoxFullyInsideFrustum(camera.value, &box), "intersecting box counted fully inside");
    for (float x : {-2.0f, -0.5f, 0.0f, 0.5f, 2.0f}) {
        sphere = {{x, 0.2f, -0.1f}, 0.25f}; box = {{x + 0.25f, 0.45f, 0.15f}, {x - 0.25f, -0.05f, -0.35f}};
        const bool inside = std::fabs(x) + 0.25f <= 1.0f;
        Require(!!RwD3D9CameraIsSphereFullyInsideFrustum(camera.value, &sphere) == inside &&
            !!RwD3D9CameraIsBBoxFullyInsideFrustum(camera.value, &box) == inside, "cube containment reference");
    }
    sphere = {{std::numeric_limits<float>::quiet_NaN(), 0, 0}, 1};
    Require(RwD3D9CameraIsSphereFullyInsideFrustum(camera.value, &sphere), "unordered x87 diagnostic changed");
    std::puts("inclusive full-containment boundaries verified; NaN sphere passes unordered x87 checks (diagnostic)");
}
}
void RunStateCase(std::string_view name) {
    if (name == "state_transforms") Transforms();
    else if (name == "state_lights") Lights();
    else if (name == "state_declarations") Declarations();
    else if (name == "state_shaders") Shaders();
    else if (name == "state_frustum") Frustum();
    else throw std::runtime_error("unknown state case");
}
}
