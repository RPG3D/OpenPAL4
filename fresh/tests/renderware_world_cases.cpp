#include "renderware_world_cases.h"
#include <rwcore.h>
#include <rpworld.h>
#include <rtworld.h>
#include <rwversion.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace pal4::test {
namespace {
void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
constexpr RwUInt32 kClumpPluginId = 0x1234C101;
int clump_plugin_offset = -1;
int clump_plugin_constructs = 0;
int clump_plugin_destructs = 0;
void* ConstructClumpPlugin(void* object, RwInt32 offset, RwInt32 size) {
    Require(offset == clump_plugin_offset && size == 4, "clump plugin constructor ABI");
    *reinterpret_cast<RwUInt32*>(static_cast<char*>(object) + offset) = 0x13572468;
    ++clump_plugin_constructs;
    return object;
}
void* DestructClumpPlugin(void* object, RwInt32 offset, RwInt32 size) {
    Require(offset == clump_plugin_offset && size == 4, "clump plugin destructor ABI");
    Require(*reinterpret_cast<RwUInt32*>(static_cast<char*>(object) + offset) ==
                0x13572468, "clump plugin storage lifetime");
    ++clump_plugin_destructs;
    return object;
}

RpWorldSector* VisitSector(RpWorldSector* sector, void* context) {
    auto** seen = static_cast<RpWorldSector**>(context);
    Require(!*seen, "created world has one sector");
    *seen = sector;
    return sector;
}

RpWorldSector* RenderSector(RpWorldSector* sector) { return sector; }
RpWorldSector* StopAtSector(RpWorldSector* sector, void* context) {
    ++*static_cast<int*>(context);
    Require(sector != nullptr, "stop callback receives sector");
    return nullptr;
}
RpMaterial* VisitMaterial(RpMaterial*, void* context) {
    ++*static_cast<int*>(context);
    return nullptr;
}
RpLight* VisitLight(RpLight* light, void* context) {
    auto** seen = static_cast<RpLight**>(context);
    Require(!*seen, "single world light");
    *seen = light;
    return light;
}
RpAtomic* CountClumpAtomic(RpAtomic* atomic, void* context) {
    Require(atomic != nullptr, "clump atomic callback object");
    ++*static_cast<int*>(context);
    return atomic;
}
RpAtomic* FindClumpAtomic(RpAtomic* atomic, void* context) {
    auto** found = static_cast<RpAtomic**>(context);
    Require(!*found, "streamed clump has one atomic");
    *found = atomic;
    return atomic;
}
RwCamera* CountClumpCamera(RwCamera* camera, void* context) {
    Require(camera != nullptr, "clump camera callback object");
    ++*static_cast<int*>(context);
    return camera;
}
RpLight* CountClumpLight(RpLight* light, void* context) {
    Require(light != nullptr, "clump light callback object");
    ++*static_cast<int*>(context);
    return light;
}
int clump_render_calls = 0;
RpAtomic* CountClumpRender(RpAtomic* atomic) {
    ++clump_render_calls;
    return atomic;
}
RpWorldSector* VisitAtomicSector(RpWorldSector* sector, void* context) {
    auto** seen = static_cast<RpWorldSector**>(context);
    Require(!*seen, "single atomic sector");
    *seen = sector;
    return sector;
}
RpWorldSector* CountFrustumSector(RpWorldSector* sector, void* context) {
    Require(sector != nullptr, "frustum callback receives sector");
    ++*static_cast<int*>(context);
    return sector;
}

void Lifecycle() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    Require(world != nullptr, "create empty world");
    RpWorldSector* sector = nullptr;
    Require(RpWorldForAllWorldSectors(world, VisitSector, &sector) == world && sector,
            "enumerate created sector");
    int sectors_visited = 0;
    Require(RpWorldForAllWorldSectors(world, StopAtSector, &sectors_visited) == world &&
                sectors_visited == 1, "sector enumeration stops and returns world");
    int materials_visited = 0;
    Require(RpWorldForAllMaterials(world, VisitMaterial, &materials_visited) == world &&
                materials_visited == 0, "empty world has no material callbacks");
    Require(RpWorldSectorGetWorld(sector) == world, "sector resolves its world");
    Require(RpWorldSetSectorRenderCallBack(world, RenderSector) == world &&
                RpWorldGetSectorRenderCallBack(world) == RenderSector,
            "world render callback roundtrip");
    Require(RpWorldSetSectorRenderCallBack(world, nullptr) == world &&
                RpWorldGetSectorRenderCallBack(world) != nullptr &&
                RpWorldGetSectorRenderCallBack(world) != RenderSector,
            "null callback restores SDK default");
    Require(RpWorldUnlock(world) == world, "unlock empty world");
    Require(RpWorldDestroy(world), "destroy empty world");
}

void UsageFlagsBoundary() {
    RpGeometry* geometry = RpGeometryCreate(0, 0, 0);
    Require(geometry != nullptr, "create geometry for usage flags");
    RpD3D9GeometrySetUsageFlags(geometry, rpD3D9GEOMETRYUSAGE_CREATETANGENTS);
    Require(RpD3D9GeometryGetUsageFlags(geometry) == rpD3D9GEOMETRYUSAGE_CREATETANGENTS,
            "geometry tangent usage flag roundtrip");
    RpD3D9GeometrySetUsageFlags(geometry, static_cast<RpD3D9GeometryUsageFlag>(0));
    Require(RpD3D9GeometryGetUsageFlags(geometry) == 0,
            "geometry usage flag clear");
    Require(RpGeometryDestroy(geometry), "destroy usage geometry");

    RwBBox box{{1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f}};
    RpWorld* world = RpWorldCreate(&box);
    Require(world != nullptr, "create world for sector usage flags");
    RpWorldSector* sector = nullptr;
    Require(RpWorldForAllWorldSectors(world, VisitSector, &sector) == world && sector,
            "find world sector for usage flags");
    RpD3D9WorldSectorSetUsageFlags(sector, rpD3D9WORLDSECTORUSAGE_CREATETANGENTS);
    Require(RpD3D9WorldSectorGetUsageFlags(sector) == rpD3D9WORLDSECTORUSAGE_CREATETANGENTS,
            "sector tangent usage flag roundtrip");
    RpD3D9WorldSectorSetUsageFlags(sector, static_cast<RpD3D9WorldSectorUsageFlag>(0));
    Require(RpD3D9WorldSectorGetUsageFlags(sector) == 0,
            "sector usage flag clear");
    Require(RpWorldDestroy(world), "destroy usage world");

    RpGeometry* space = RpGeometryCreateSpace(3.5f);
    Require(space != nullptr && space->numMorphTargets == 1,
            "create one-target geometry space");
    const RwSphere* bounds = RpMorphTargetGetBoundingSphere(RpGeometryGetMorphTarget(space, 0));
    Require(bounds && bounds->radius == 3.5f, "geometry space preserves radius");
    Require(RpGeometryDestroy(space), "destroy geometry space");
}

void CameraBoundary() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    RwCamera* camera = RwCameraCreate();
    RwFrame* frame = RwFrameCreate();
    RwRaster* raster = RwRasterCreate(64, 64, 32,
                                     rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    Require(world && camera && frame && raster, "world camera resources");
    RwCameraSetFrame(camera, frame);
    RwCameraSetRaster(camera, raster);
    Require(RpWorldAddCamera(world, camera) == world && RwCameraGetWorld(camera) == world,
            "attach camera to world");
    RwCamera* clone = RwCameraClone(camera);
    Require(clone && RwCameraGetWorld(clone) == world,
            "cloned camera retains world extension");
    Require(RwCameraDestroy(clone), "destroy cloned camera");
    Require(RwCameraBeginUpdate(camera) == camera, "begin world camera update");
    Require(RwCameraEndUpdate(camera) == camera, "end world camera update");
    int visible_sectors = 0;
    Require(RwCameraForAllSectorsInFrustum(camera, CountFrustumSector,
                                           &visible_sectors) == camera &&
                visible_sectors == 1, "enumerate camera frustum sector");
    Require(RpWorldRemoveCamera(world, camera) == world && !RwCameraGetWorld(camera),
            "detach camera from world");
    RwCameraSetRaster(camera, nullptr);
    RwCameraSetFrame(camera, nullptr);
    Require(RwCameraDestroy(camera) && RwFrameDestroy(frame) && RwRasterDestroy(raster),
            "destroy world camera resources");
    Require(RpWorldDestroy(world), "destroy camera world");
}

void LightBoundary() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    RpLight* light = RpLightCreate(rpLIGHTPOINT);
    RwFrame* frame = RwFrameCreate();
    Require(world && light && frame, "world light resources");
    const RwInt32 d3d9_offset = RpLightGetPluginOffset(0x505);
    Require(d3d9_offset >= 0, "D3D9 light plugin registered");
    struct D3D9LightExtension {
        RwInt32 index;
        RwReal constant;
        RwReal linear;
        RwReal quadratic;
    };
    D3D9LightExtension extension{};
    std::memcpy(&extension, reinterpret_cast<const char*>(light) + d3d9_offset,
                sizeof(extension));
    Require(extension.index == -1 && extension.constant == 1.0f &&
                extension.linear == 0.0f && extension.quadratic == 5.0f,
            "D3D9 light plugin constructor defaults");
    RpLightSetFrame(light, frame);
    Require(RpLightSetRadius(light, 2.0f) == light &&
                std::fabs(RpLightGetRadius(light) - 2.0f) < 0.001f,
            "light radius roundtrip");
    Require(RpWorldAddLight(world, light) == world && RpLightGetWorld(light) == world,
            "attach point light to world");
    const RwV3d position{0.0f, 0.0f, 1.0f};
    Require(RwFrameTranslate(frame, &position, rwCOMBINEREPLACE) == frame,
            "move world light frame");
    RwCamera* camera = RwCameraCreate();
    RwFrame* camera_frame = RwFrameCreate();
    RwRaster* raster = RwRasterCreate(64, 64, 32,
                                     rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    Require(camera && camera_frame && raster, "world light update camera");
    RwCameraSetFrame(camera, camera_frame);
    RwCameraSetRaster(camera, raster);
    Require(RpWorldAddCamera(world, camera) == world &&
                RwCameraBeginUpdate(camera) == camera &&
                RwCameraEndUpdate(camera) == camera,
            "update world light through camera");
    RpWorldSector* sector = nullptr;
    Require(RpWorldForAllWorldSectors(world, VisitSector, &sector) == world && sector,
            "find sector for light");
    RpLight* visited = nullptr;
    Require(RpWorldSectorForAllLights(sector, VisitLight, &visited) == sector,
            "sector light enumeration return");
    Require(visited == light, "point light tied to sector");
    Require(RpWorldRemoveLight(world, light) == world && !RpLightGetWorld(light),
            "detach point light");
    visited = nullptr;
    Require(RpWorldSectorForAllLights(sector, VisitLight, &visited) == sector &&
                !visited, "detached light has no sector tie");
    RpLightSetFrame(light, nullptr);
    Require(RpWorldRemoveCamera(world, camera) == world, "remove update camera");
    RwCameraSetRaster(camera, nullptr);
    RwCameraSetFrame(camera, nullptr);
    Require(RwCameraDestroy(camera) && RwFrameDestroy(camera_frame) &&
                RwRasterDestroy(raster), "destroy update camera");
    RpLight* spot = RpLightCreate(rpLIGHTSPOT);
    Require(spot != nullptr, "create spot light");
    const RwRGBAReal color{0.25f, 0.5f, 0.75f, 1.0f};
    Require(RpLightSetColor(spot, &color) == spot,
            "set spot light color");
    const RwRGBAReal* got_color = RpLightGetColor(spot);
    Require(got_color && std::fabs(got_color->red - color.red) < 0.001f &&
                std::fabs(got_color->green - color.green) < 0.001f &&
                std::fabs(got_color->blue - color.blue) < 0.001f &&
                std::fabs(got_color->alpha - color.alpha) < 0.001f,
            "spot light color roundtrip");
    Require(RpLightSetConeAngle(spot, 0.5f) == spot &&
                std::fabs(RpLightGetConeAngle(spot) - 0.5f) < 0.001f,
            "spot light cone roundtrip");
    Require(RpLightSetConeAngle(spot, -0.01f) == nullptr &&
                RpLightSetConeAngle(spot, 1.6f) == nullptr &&
                std::fabs(RpLightGetConeAngle(spot) - 0.5f) < 0.001f,
            "spot cone rejects out-of-range values without mutation");
    Require(RpLightDestroy(spot), "destroy spot light");
    Require(RpLightDestroy(light) && RwFrameDestroy(frame) && RpWorldDestroy(world),
            "destroy light and world");
}

void ClumpBoundary() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    RpClump* clump = RpClumpCreate();
    RwFrame* frame = RwFrameCreate();
    Require(world && clump && frame, "world clump resources");
    RpClumpSetFrame(clump, frame);
    Require(RpWorldAddClump(world, clump) == world && RpClumpGetWorld(clump) == world,
            "attach clump to world");
    RpClump* clone = RpClumpClone(clump);
    Require(clone && RpClumpGetWorld(clone) == world,
            "cloned clump retains world association");
    Require(RpWorldRemoveClump(world, clone) == world && RpClumpDestroy(clone),
            "remove and destroy cloned clump");
    Require(RpWorldRemoveClump(world, clump) == world && RpClumpDestroy(clump) &&
                RpWorldDestroy(world), "remove and destroy world clump");
}

void ClumpPluginBoundary() {
    Require(RpClumpGetPluginOffset(kClumpPluginId) == clump_plugin_offset,
            "clump plugin offset lookup");
    RpClump* clump = RpClumpCreate();
    Require(clump && clump_plugin_constructs == 1 &&
                clump_plugin_destructs == 0,
            "clump plugin constructor runs at create");
    Require(RpClumpDestroy(clump) && clump_plugin_destructs == 1,
            "clump plugin destructor runs at destroy");
}

void ClumpMembersBoundary() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    RpClump* clump = RpClumpCreate();
    RwFrame* root = RwFrameCreate();
    RwFrame* atomic_frame = RwFrameCreate();
    RwFrame* light_frame = RwFrameCreate();
    RwFrame* camera_frame = RwFrameCreate();
    RpAtomic* atomic = RpAtomicCreate();
    RpLight* light = RpLightCreate(rpLIGHTPOINT);
    RwCamera* camera = RwCameraCreate();
    RpGeometry* geometry = RpGeometryCreate(3, 0, rpGEOMETRYPOSITIONS);
    Require(world && clump && root && atomic_frame && light_frame && camera_frame &&
                atomic && light && camera && geometry, "populated clump resources");
    const RwV3d vertices[]{{-1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
                           {0.0f, 1.0f, 1.0f}};
    std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0)),
                vertices, sizeof(vertices));
    Require(RpGeometryUnlock(geometry) == geometry, "populated clump geometry");
    RpAtomicSetGeometry(atomic, geometry, 0);
    RpGeometryDestroy(geometry);
    RpClumpSetFrame(clump, root);
    RwFrameAddChild(root, atomic_frame);
    RwFrameAddChild(root, light_frame);
    RwFrameAddChild(root, camera_frame);
    RpAtomicSetFrame(atomic, atomic_frame);
    RpLightSetFrame(light, light_frame);
    RwCameraSetFrame(camera, camera_frame);
    Require(RpClumpAddAtomic(clump, atomic) == clump &&
                RpClumpAddLight(clump, light) == clump &&
                RpClumpAddCamera(clump, camera) == clump,
            "attach members to clump");
    Require(RpAtomicGetClump(atomic) == clump,
            "added atomic records owning clump");
    int atomics = 0;
    int lights = 0;
    int cameras = 0;
    Require(RpClumpForAllAtomics(clump, CountClumpAtomic, &atomics) == clump &&
                RpClumpForAllLights(clump, CountClumpLight, &lights) == clump &&
                RpClumpForAllCameras(clump, CountClumpCamera, &cameras) == clump &&
                atomics == 1 && lights == 1 && cameras == 1,
            "enumerate each clump member class");
    Require(RpAtomicGetRenderCallBack(atomic) != nullptr,
            "new atomic has SDK default render callback");
    RpAtomicSetRenderCallBack(atomic, CountClumpRender);
    clump_render_calls = 0;
    Require(RpClumpRender(clump) == clump && clump_render_calls == 1,
            "render clump through atomic callback");
    Require(RpWorldAddClump(world, clump) == world &&
                RpAtomicGetWorld(atomic) == world &&
                RpLightGetWorld(light) == world &&
                RwCameraGetWorld(camera) == world,
            "world membership propagates to clump members");
    Require(RpWorldRemoveClump(world, clump) == world &&
                !RpAtomicGetWorld(atomic) && !RpLightGetWorld(light) &&
                !RwCameraGetWorld(camera), "clump member world removal");
    Require(RpWorldRemoveClump(world, clump) == nullptr,
            "removing an unattached clump fails");
    Require(RpClumpDestroy(clump) && RpWorldDestroy(world),
            "destroy populated clump and world");
}

void AtomicBoundary() {
    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    RpAtomic* atomic = RpAtomicCreate();
    RwFrame* frame = RwFrameCreate();
    RpGeometry* geometry = RpGeometryCreate(3, 0, rpGEOMETRYPOSITIONS);
    Require(world && atomic && frame && geometry, "world atomic resources");
    const RwV3d vertices[]{{-1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
                           {0.0f, 1.0f, 1.0f}};
    std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0)),
                vertices, sizeof(vertices));
    const RwSphere local_sphere{{1.0f, 2.0f, 3.0f}, 2.0f};
    Require(RpMorphTargetSetBoundingSphere(RpGeometryGetMorphTarget(geometry, 0),
                                            &local_sphere) != nullptr,
            "set atomic local bounding sphere");
    Require(RpGeometryUnlock(geometry) == geometry, "atomic geometry unlock");
    Require(RpAtomicSetGeometry(atomic, geometry, 0) == atomic &&
                geometry->refCount == 2, "atomic retains geometry reference");
    Require(RpAtomicSetGeometry(atomic, geometry, 0) == atomic &&
                geometry->refCount == 2, "setting same geometry is idempotent");
    RpGeometryDestroy(geometry);
    Require(RpAtomicGetGeometry(atomic)->refCount == 1,
            "atomic holds final geometry reference");
    RpAtomicSetFrame(atomic, frame);
    const RwSphere* world_sphere = RpAtomicGetWorldBoundingSphere(atomic);
    Require(world_sphere && std::fabs(world_sphere->center.x - 1.0f) < 0.001f &&
                std::fabs(world_sphere->radius - 2.0f) < 0.001f,
            "atomic world sphere starts at local sphere");
    const RwV3d offset{3.0f, 4.0f, 5.0f};
    Require(RwFrameTranslate(frame, &offset, rwCOMBINEREPLACE) == frame,
            "translate atomic frame");
    world_sphere = RpAtomicGetWorldBoundingSphere(atomic);
    Require(world_sphere && std::fabs(world_sphere->center.x - 4.0f) < 0.001f &&
                std::fabs(world_sphere->center.y - 6.0f) < 0.001f &&
                std::fabs(world_sphere->center.z - 8.0f) < 0.001f &&
                std::fabs(world_sphere->radius - 2.0f) < 0.001f,
            "atomic world sphere follows frame translation");
    Require(RpWorldAddAtomic(world, atomic) == world && RpAtomicGetWorld(atomic) == world,
            "attach atomic to world");
    RwCamera* camera = RwCameraCreate();
    RwFrame* camera_frame = RwFrameCreate();
    RwRaster* raster = RwRasterCreate(64, 64, 32,
                                     rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    Require(camera && camera_frame && raster, "atomic update camera");
    RwCameraSetFrame(camera, camera_frame);
    RwCameraSetRaster(camera, raster);
    Require(RpWorldAddCamera(world, camera) == world &&
                RwCameraBeginUpdate(camera) == camera &&
                RwCameraEndUpdate(camera) == camera, "update atomic world");
    RpWorldSector* sector = nullptr;
    Require(RpAtomicForAllWorldSectors(atomic, VisitAtomicSector, &sector) == atomic &&
                sector, "atomic tied to sector");
    Require(RpWorldRemoveAtomic(world, atomic) == world && !RpAtomicGetWorld(atomic),
            "remove world atomic");
    RpAtomicSetFrame(atomic, nullptr);
    Require(RpAtomicDestroy(atomic) && RwFrameDestroy(frame), "destroy world atomic");
    Require(RpWorldRemoveCamera(world, camera) == world, "remove atomic update camera");
    RwCameraSetRaster(camera, nullptr);
    RwCameraSetFrame(camera, nullptr);
    Require(RwCameraDestroy(camera) && RwFrameDestroy(camera_frame) &&
                RwRasterDestroy(raster) && RpWorldDestroy(world),
            "destroy atomic update camera and world");
}

void StreamBoundary() {
    RpGeometry* geometry = RpGeometryCreate(3, 0, rpGEOMETRYPOSITIONS);
    Require(geometry && RpGeometryUnlock(geometry) == geometry,
            "create stream geometry");
    RwMemory geometry_bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpGeometryStreamWrite(geometry, writer) == geometry &&
                RwStreamClose(writer, &geometry_bytes), "write geometry stream");
    RwUInt32 geometry_library_id{};
    Require(geometry_bytes.length >= 12, "geometry chunk header length");
    std::memcpy(&geometry_library_id, geometry_bytes.start + 8, sizeof geometry_library_id);
    Require(RWLIBRARYIDUNPACKVERSION(geometry_library_id) == 0x37002 &&
                RWLIBRARYIDUNPACKBUILDNUM(geometry_library_id) == 0x0A,
            "available SDK geometry stream uses build 10");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &geometry_bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_GEOMETRY, nullptr, nullptr),
            "find geometry chunk");
    RpGeometry* loaded_geometry = RpGeometryStreamRead(reader);
    Require(loaded_geometry && loaded_geometry->numVertices == 3 &&
                RwStreamClose(reader, nullptr), "read geometry stream");
    Require(RpGeometryDestroy(loaded_geometry) && RpGeometryDestroy(geometry),
            "destroy streamed geometries");
    RwFree(geometry_bytes.start);

    RwBBox box{{10.0f, 20.0f, 30.0f}, {-10.0f, -20.0f, -30.0f}};
    RpWorld* world = RpWorldCreate(&box);
    Require(world != nullptr, "create stream world");
    RwMemory world_bytes{};
    writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpWorldStreamWrite(world, writer) == world &&
                RwStreamClose(writer, &world_bytes), "write world stream");
    reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &world_bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_WORLD, nullptr, nullptr),
            "find world chunk");
    RpWorld* loaded_world = RpWorldStreamRead(reader);
    Require(loaded_world && RwStreamClose(reader, nullptr), "read world stream");
    const RwBBox* loaded_box = RpWorldGetBBox(loaded_world);
    Require(loaded_box && loaded_box->inf.x == box.inf.x &&
                loaded_box->inf.y == box.inf.y && loaded_box->inf.z == box.inf.z &&
                loaded_box->sup.x == box.sup.x && loaded_box->sup.y == box.sup.y &&
                loaded_box->sup.z == box.sup.z,
            "streamed world restores bounding box");
    RpWorldSector* sector = nullptr;
    Require(RpWorldForAllWorldSectors(loaded_world, VisitSector, &sector) == loaded_world &&
                sector, "streamed world sector");
    const RwBBox* sector_box = RpWorldSectorGetBBox(sector);
    Require(sector_box && sector_box->inf.x == box.inf.x &&
                sector_box->inf.y == box.inf.y && sector_box->inf.z == box.inf.z &&
                sector_box->sup.x == box.sup.x && sector_box->sup.y == box.sup.y &&
                sector_box->sup.z == box.sup.z,
            "streamed leaf sector restores bounding box");
    Require(RpWorldDestroy(loaded_world) && RpWorldDestroy(world),
            "destroy streamed worlds");
    RwFree(world_bytes.start);
}

void ClumpStreamBoundary() {
    RpClump* clump = RpClumpCreate();
    RwFrame* root = RwFrameCreate();
    RwFrame* child = RwFrameCreate();
    RpAtomic* atomic = RpAtomicCreate();
    RpGeometry* geometry = RpGeometryCreate(3, 0, rpGEOMETRYPOSITIONS);
    Require(clump && root && child && atomic && geometry,
            "clump stream resources");
    const RwV3d vertices[]{{-1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
                           {0.0f, 1.0f, 1.0f}};
    std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0)),
                vertices, sizeof(vertices));
    Require(RpGeometryUnlock(geometry) == geometry,
            "unlock clump stream geometry");
    RpClumpSetFrame(clump, root);
    RwFrameAddChild(root, child);
    RpAtomicSetFrame(atomic, child);
    Require(RpAtomicSetGeometry(atomic, geometry, 0) == atomic &&
                RpClumpAddAtomic(clump, atomic) == clump,
            "add geometry atomic to streamed clump");
    RpGeometryDestroy(geometry);

    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpClumpStreamWrite(clump, writer) == clump &&
                RwStreamClose(writer, &bytes), "write clump stream");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_CLUMP, nullptr, nullptr),
            "find clump stream chunk");
    RpClump* loaded = RpClumpStreamRead(reader);
    Require(loaded && RwStreamClose(reader, nullptr), "read clump stream");
    RpAtomic* loaded_atomic = nullptr;
    Require(RpClumpForAllAtomics(loaded, FindClumpAtomic, &loaded_atomic) == loaded &&
                loaded_atomic && RpAtomicGetClump(loaded_atomic) == loaded &&
                RpAtomicGetGeometry(loaded_atomic)->numVertices == 3,
            "streamed clump restores atomic geometry and ownership");
    Require(RpClumpDestroy(loaded) && RpClumpDestroy(clump),
            "destroy streamed clumps");
    RwFree(bytes.start);
}

void LightStreamBoundary() {
    RpLight* light = RpLightCreate(rpLIGHTSPOT);
    Require(light != nullptr, "create streamed spot light");
    const RwRGBAReal color{0.2f, 0.4f, 0.6f, 0.8f};
    Require(RpLightSetRadius(light, 3.5f) == light &&
                RpLightSetColor(light, &color) == light &&
                RpLightSetConeAngle(light, 0.65f) == light,
            "configure streamed spot light");
    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpLightStreamWrite(light, writer) == light &&
                RwStreamClose(writer, &bytes), "write spot light stream");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_LIGHT, nullptr, nullptr),
            "find spot light chunk");
    RpLight* loaded = RpLightStreamRead(reader);
    Require(loaded && RwStreamClose(reader, nullptr), "read spot light stream");
    const RwRGBAReal* loaded_color = RpLightGetColor(loaded);
    Require(RpLightGetType(loaded) == rpLIGHTSPOT,
            "spot light stream restores type");
    Require(std::fabs(RpLightGetRadius(loaded) - 3.5f) < 0.001f,
            "spot light stream restores radius");
    Require(std::fabs(RpLightGetConeAngle(loaded) - 0.65f) < 0.001f,
            "spot light stream restores cone");
    Require(loaded_color && std::fabs(loaded_color->red - color.red) < 0.001f &&
                std::fabs(loaded_color->green - color.green) < 0.001f &&
                std::fabs(loaded_color->blue - color.blue) < 0.001f &&
                std::fabs(loaded_color->alpha - 1.0f) < 0.001f,
            "spot light stream preserves RGB and restores opaque alpha");
    Require(RpLightDestroy(loaded) && RpLightDestroy(light),
            "destroy streamed spot lights");
    RwFree(bytes.start);
}

void MaterialBoundary() {
    RpMaterial* material = RpMaterialCreate();
    Require(material && material->refCount == 1 && !RpMaterialGetTexture(material),
            "create untextured material");
    const RwRGBA color{17, 68, 153, 221};
    Require(RpMaterialSetColor(material, &color) == material,
            "set material color");
    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpMaterialStreamWrite(material, writer) == material &&
                RwStreamClose(writer, &bytes), "write untextured material");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_MATERIAL, nullptr, nullptr),
            "find material chunk");
    RpMaterial* loaded = RpMaterialStreamRead(reader);
    Require(loaded && RwStreamClose(reader, nullptr), "read material stream");
    const RwRGBA* loaded_color = RpMaterialGetColor(loaded);
    Require(loaded->refCount == 1 && !RpMaterialGetTexture(loaded) &&
                loaded_color && std::memcmp(loaded_color, &color, sizeof(color)) == 0,
            "material stream restores color and empty texture");
    Require(RpMaterialDestroy(loaded), "destroy streamed material");
    RwFree(bytes.start);

    RwTexture* texture = RwTextureCreate(nullptr);
    Require(texture && texture->refCount == 1, "create material texture");
    Require(RpMaterialSetTexture(material, texture) == material &&
                RpMaterialGetTexture(material) == texture && texture->refCount == 2,
            "material retains assigned texture");
    Require(RwTextureDestroy(texture) && texture->refCount == 1,
            "release caller texture reference");
    Require(RpMaterialDestroy(material), "destroy material and final texture reference");
}

void NormalsOptimizeBoundary() {
    RpGeometry* geometry = RpGeometryCreate(3, 1, rpGEOMETRYPOSITIONS | rpGEOMETRYNORMALS);
    RpMaterial* material = RpMaterialCreate();
    Require(geometry && material, "normal calculation resources");
    const RwV3d vertices[]{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f}};
    std::memcpy(RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0)),
                vertices, sizeof(vertices));
    RpTriangle* triangle = RpGeometryGetTriangles(geometry);
    Require(triangle && RpGeometryTriangleSetVertexIndices(geometry, triangle, 0, 1, 2) == geometry &&
                RpGeometryTriangleSetMaterial(geometry, triangle, material) == geometry &&
                RpGeometryUnlock(geometry) == geometry,
            "configure normal calculation triangle");
    Require(RtGeometryCalculateVertexNormals(geometry) == geometry,
            "SDK normal calculation returns geometry");
    const RwV3d* normals = RpMorphTargetGetVertexNormals(RpGeometryGetMorphTarget(geometry, 0));
    Require(normals != nullptr, "calculated normals exist");
    for (int index = 0; index < 3; ++index)
        Require(std::fabs(normals[index].x) < 0.001f &&
                    std::fabs(normals[index].y) < 0.001f &&
                    std::fabs(std::fabs(normals[index].z) - 1.0f) < 0.001f,
                "triangle normals are unit length and perpendicular to face");

    RpAtomic* atomic = RpAtomicCreate();
    RpClump* clump = RpClumpCreate();
    RwFrame* root = RwFrameCreate();
    RwFrame* child = RwFrameCreate();
    Require(atomic && clump && root && child, "optimizer resources");
    RpClumpSetFrame(clump, root);
    RwFrameAddChild(root, child);
    RpAtomicSetFrame(atomic, child);
    Require(RpAtomicSetGeometry(atomic, geometry, 0) == atomic &&
                RpClumpAddAtomic(clump, atomic) == clump,
            "attach atomic to optimizer clump");
    Require(RtAtomicOptimize(atomic, 1.0f) == atomic &&
                RtClumpOptimize(clump, 1.0f) == clump,
            "SDK Atomic and populated Clump optimization boundaries");
    Require(RpGeometryDestroy(geometry) && RpMaterialDestroy(material) &&
                RpClumpDestroy(clump), "release optimizer resources");
}

void MeshBoundary() {
    RpGeometry* geometry = RpGeometryCreate(4, 2, rpGEOMETRYPOSITIONS);
    RpMaterial* first = RpMaterialCreate();
    RpMaterial* second = RpMaterialCreate();
    Require(geometry && first && second, "create mesh boundary resources");
    RpTriangle* triangles = RpGeometryGetTriangles(geometry);
    Require(triangles && RpGeometryTriangleSetVertexIndices(geometry, &triangles[0], 0, 1, 2) == geometry &&
                RpGeometryTriangleSetVertexIndices(geometry, &triangles[1], 0, 2, 3) == geometry &&
                RpGeometryTriangleSetMaterial(geometry, &triangles[0], first) == geometry &&
                RpGeometryTriangleSetMaterial(geometry, &triangles[1], second) == geometry,
            "set triangles and distinct materials");
    Require(RpGeometryUnlock(geometry) == geometry, "generate geometry meshes");
    const RpMeshHeader* header = geometry->mesh;
    Require(header && header->numMeshes == 2 && header->totalIndicesInMesh == 6 &&
                (header->flags & rpMESHHEADERTRISTRIP) == 0,
            "SDK groups two triangle-list meshes by material");
    struct MeshSummary { int count = 0; RpMaterial* first = nullptr; } summary;
    auto visit = [](RpMesh* mesh, RpMeshHeader*, void* data) -> RpMesh* {
        auto& state = *static_cast<MeshSummary*>(data);
        Require(mesh->numIndices == 3, "each mesh holds one triangle");
        if (state.count == 0) state.first = mesh->material;
        else Require(mesh->material != state.first, "meshes have distinct materials");
        ++state.count;
        return mesh;
    };
    Require(RpGeometryForAllMeshes(geometry, visit, &summary) == geometry && summary.count == 2,
            "enumerate material mesh groups");

    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpGeometryStreamWrite(geometry, writer) == geometry &&
                RwStreamClose(writer, &bytes), "write grouped geometry");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_GEOMETRY, nullptr, nullptr),
            "find grouped geometry chunk");
    RpGeometry* loaded = RpGeometryStreamRead(reader);
    Require(loaded && RwStreamClose(reader, nullptr) && loaded->mesh &&
                loaded->mesh->numMeshes == 2 && loaded->mesh->totalIndicesInMesh == 6,
            "read grouped geometry meshes");
    Require(RpGeometryDestroy(loaded) && RpGeometryDestroy(geometry) &&
                RpMaterialDestroy(first) && RpMaterialDestroy(second),
            "release mesh boundary resources");
    RwFree(bytes.start);
}

void MeshStripBoundary() {
    RpTriStripMeshCallBack previous{};
    void* previous_data{};
    Require(RpMeshGetTriStripMethod(&previous, &previous_data), "read SDK strip callback");
    int callback_calls = 0;
    RpTriStripMeshCallBack callback = +[](RpBuildMesh* build, void* data) -> RpMeshHeader* {
        ++*static_cast<int*>(data);
        return RpBuildMeshGenerateDefaultTriStrip(build, data);
    };
    Require(RpMeshSetTriStripMethod(callback, &callback_calls),
            "select forwarding strip callback");
    RpTriStripMeshCallBack installed{};
    void* installed_data{};
    Require(RpMeshGetTriStripMethod(&installed, &installed_data) &&
                installed == callback && installed_data == &callback_calls,
            "strip callback and data roundtrip");
    RpGeometry* geometry = RpGeometryCreate(4, 2, rpGEOMETRYPOSITIONS | rpGEOMETRYTRISTRIP);
    RpMaterial* material = RpMaterialCreate();
    Require(geometry && material, "create strip boundary resources");
    RpTriangle* triangles = RpGeometryGetTriangles(geometry);
    Require(triangles && RpGeometryTriangleSetVertexIndices(geometry, &triangles[0], 0, 1, 2) == geometry &&
                RpGeometryTriangleSetVertexIndices(geometry, &triangles[1], 2, 1, 3) == geometry &&
                RpGeometryTriangleSetMaterial(geometry, &triangles[0], material) == geometry &&
                RpGeometryTriangleSetMaterial(geometry, &triangles[1], material) == geometry,
            "configure adjacent strip triangles");
    Require(RpGeometryUnlock(geometry) == geometry && callback_calls == 1,
            "geometry unlock invokes strip callback with data");
    const RpMeshHeader* header = geometry->mesh;
    Require(header && header->numMeshes == 1 &&
                (header->flags & rpMESHHEADERTRISTRIP) == rpMESHHEADERTRISTRIP,
            "adjacent triangles form one strip mesh");
    struct StripSummary { int count = 0; RwUInt32 indices = 0; } summary;
    auto visit = [](RpMesh* mesh, RpMeshHeader*, void* data) -> RpMesh* {
        auto& state = *static_cast<StripSummary*>(data);
        ++state.count;
        state.indices = mesh->numIndices;
        Require(mesh->material != nullptr && mesh->indices != nullptr,
                "strip retains material and indices");
        return mesh;
    };
    Require(RpGeometryForAllMeshes(geometry, visit, &summary) == geometry &&
                summary.count == 1 && summary.indices == 4 &&
                summary.indices == header->totalIndicesInMesh,
            "two adjacent triangles use four strip indices");
    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && RpGeometryStreamWrite(geometry, writer) == geometry &&
                RwStreamClose(writer, &bytes), "write strip geometry");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    Require(reader && RwStreamFindChunk(reader, rwID_GEOMETRY, nullptr, nullptr),
            "find strip geometry chunk");
    RpGeometry* loaded = RpGeometryStreamRead(reader);
    Require(loaded && RwStreamClose(reader, nullptr) && loaded->mesh &&
                loaded->mesh->numMeshes == 1 &&
                loaded->mesh->totalIndicesInMesh == 4 &&
                (loaded->mesh->flags & rpMESHHEADERTRISTRIP) == rpMESHHEADERTRISTRIP,
            "read four-index strip mesh");
    Require(RpGeometryDestroy(loaded) && RpGeometryDestroy(geometry) &&
                RpMaterialDestroy(material),
            "release strip boundary resources");
    RwFree(bytes.start);
    Require(RpMeshSetTriStripMethod(previous, previous_data),
            "restore prior strip callback");
}

void MaterialListBoundary() {
    RpMaterialList list{};
    Require(_rpMaterialListInitialize(&list) == &list && !list.materials &&
                list.numMaterials == 0 && list.space == 0,
            "initialize empty material list");
    RpMaterial* first = RpMaterialCreate();
    RpMaterial* second = RpMaterialCreate();
    Require(first && second, "create list materials");
    Require(_rpMaterialListAppendMaterial(&list, first) == 0 &&
                _rpMaterialListAppendMaterial(&list, second) == 1 &&
                _rpMaterialListAppendMaterial(&list, first) == 2 &&
                list.numMaterials == 3 && list.materials[0] == list.materials[2] &&
                _rpMaterialListGetMaterial(&list, 1) == second &&
                _rpMaterialListFindMaterialIndex(&list, first) == 2,
            "append and find the last repeated material pointer");
    Require(RpMaterialDestroy(first) && RpMaterialDestroy(second),
            "release caller material references");

    RwMemory bytes{};
    RwStream* writer = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMWRITE, nullptr);
    Require(writer && _rpMaterialListStreamWrite(&list, writer) == &list &&
                RwStreamClose(writer, &bytes), "write material list");
    RwStream* reader = RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &bytes);
    RwChunkHeaderInfo header{};
    Require(reader && RwStreamReadChunkHeaderInfo(reader, &header) &&
                header.type == rwID_MATLIST && header.version == 0x37002 &&
                header.buildNum == 0x0A &&
                header.length == _rpMaterialListStreamGetSize(&list),
            "SDK material-list chunk version, build and size");
    RpMaterialList loaded{};
    Require(_rpMaterialListStreamRead(reader, &loaded) == &loaded &&
                RwStreamClose(reader, nullptr) && loaded.numMaterials == 3 &&
                loaded.materials[0] == loaded.materials[2] &&
                loaded.materials[0] != loaded.materials[1],
            "read material-list shared references");
    Require(_rpMaterialListDeinitialize(&loaded) == &loaded &&
                _rpMaterialListDeinitialize(&list) == &list,
            "release both material lists");
    RwFree(bytes.start);
}
}

void RegisterClumpPluginBoundary() {
    clump_plugin_constructs = 0;
    clump_plugin_destructs = 0;
    clump_plugin_offset = RpClumpRegisterPlugin(
        4, kClumpPluginId, ConstructClumpPlugin, DestructClumpPlugin, nullptr);
    Require(clump_plugin_offset >= 0, "register clump plugin before engine open");
}

void RunWorldCase(std::string_view name) {
    if (name == "world_lifecycle") Lifecycle();
    else if (name == "world_usage") UsageFlagsBoundary();
    else if (name == "world_camera") CameraBoundary();
    else if (name == "world_light") LightBoundary();
    else if (name == "world_clump") ClumpBoundary();
    else if (name == "world_clump_plugin") ClumpPluginBoundary();
    else if (name == "world_clump_members") ClumpMembersBoundary();
    else if (name == "world_atomic") AtomicBoundary();
    else if (name == "world_stream") StreamBoundary();
    else if (name == "world_clump_stream") ClumpStreamBoundary();
    else if (name == "world_light_stream") LightStreamBoundary();
    else if (name == "world_material") MaterialBoundary();
    else if (name == "world_mesh") MeshBoundary();
    else if (name == "world_mesh_strip") MeshStripBoundary();
    else if (name == "world_normals_optimize") NormalsOptimizeBoundary();
    else if (name == "world_material_list") MaterialListBoundary();
    else throw std::runtime_error("unknown world case");
}
}
