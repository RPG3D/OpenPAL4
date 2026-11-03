#include "pal4/geo_plane.h"

#include <bit>
#include <stdexcept>

namespace pal4 {

namespace {

void WriteFloat(IGeoPlaneCreateRuntime& runtime,
                std::uint32_t address, float value) {
    runtime.WriteWord(address, std::bit_cast<std::uint32_t>(value));
}

}  // namespace

int GeoPlaneObject::SetMaterialFlags(
    GeoPlaneState& object, std::uint32_t flags,
    IGeoPlaneRuntime& runtime) {
    object.material_flags = flags;
    if ((object.material_flags >> 24u) == 0xffu)
        object.material_flags = (object.material_flags & 0x00ffffffu) |
                                0xfe000000u;
    return runtime.SetMaterialFlagsForAllAtomics(
        object.clump, object.material_flags);
}

int GeoPlaneObject::SetVector(
    GeoPlaneState& object, GeoPlaneVector& destination,
    GeoPlaneVector value, bool additive,
    IGeoPlaneRuntime& runtime) {
    if (additive) {
        destination.x += value.x;
        destination.y += value.y;
        destination.z += value.z;
    } else {
        destination = value;
    }
    return UpdateFrameTransform(object, runtime);
}

int GeoPlaneObject::SetRotation(
    GeoPlaneState& object, GeoPlaneVector value,
    bool additive, IGeoPlaneRuntime& runtime) {
    return SetVector(object, object.rotation, value, additive, runtime);
}

int GeoPlaneObject::SetScale(
    GeoPlaneState& object, GeoPlaneVector value,
    bool additive, IGeoPlaneRuntime& runtime) {
    return SetVector(object, object.scale, value, additive, runtime);
}

int GeoPlaneObject::SetPosition(
    GeoPlaneState& object, GeoPlaneVector value,
    bool additive, IGeoPlaneRuntime& runtime) {
    return SetVector(object, object.position, value, additive, runtime);
}

int GeoPlaneObject::WriteGeometryVertices(
    const GeoPlaneState& object,
    IGeoPlaneGeometryRuntime& runtime,
    bool conditional_order) {
    const auto geometry = runtime.ReadWord(object.atomic + 24u);
    runtime.LockGeometry(geometry, 16u);
    const auto vertices = runtime.ReadWord(geometry + 52u);
    const auto& b = object.vertex_bounds;
    const std::array<std::uint32_t, 8u> values =
        conditional_order && object.reversed_vertices != 0u
            ? std::array<std::uint32_t, 8u>{b[1], b[0], b[3], b[0],
                                            b[1], b[2], b[3], b[2]}
        : object.reversed_vertices != 0u
            ? std::array<std::uint32_t, 8u>{b[1], b[0], b[1], b[2],
                                            b[3], b[0], b[3], b[2]}
            : std::array<std::uint32_t, 8u>{b[0], b[1], b[0], b[3],
                                            b[2], b[1], b[2], b[3]};
    const std::array<std::uint32_t, 8u> forward{0u, 1u, 2u, 3u,
                                                 4u, 5u, 6u, 7u};
    const std::array<std::uint32_t, 8u> swapped{1u, 0u, 3u, 2u,
                                                 5u, 4u, 7u, 6u};
    const auto& order = conditional_order && object.reversed_vertices != 0u
                            ? swapped : forward;
    for (const auto index : order)
        runtime.WriteWord(vertices + index * 4u, values[index]);
    return runtime.UnlockGeometry(geometry);
}

int GeoPlaneObject::UpdateGeometryVertices(
    GeoPlaneState& object,
    std::array<std::uint32_t, 4u> bounds,
    IGeoPlaneGeometryRuntime& runtime) {
    // The native lock and vertex pointer lookup precede the cached writes.
    const auto geometry = runtime.ReadWord(object.atomic + 24u);
    runtime.LockGeometry(geometry, 16u);
    const auto vertices = runtime.ReadWord(geometry + 52u);
    object.vertex_bounds = bounds;
    const auto& b = object.vertex_bounds;
    const std::array<std::uint32_t, 8u> values =
        object.reversed_vertices != 0u
            ? std::array<std::uint32_t, 8u>{b[1], b[0], b[1], b[2],
                                            b[3], b[0], b[3], b[2]}
            : std::array<std::uint32_t, 8u>{b[0], b[1], b[0], b[3],
                                            b[2], b[1], b[2], b[3]};
    for (std::uint32_t index = 0; index < values.size(); ++index)
        runtime.WriteWord(vertices + index * 4u, values[index]);
    return runtime.UnlockGeometry(geometry);
}

std::uint8_t GeoPlaneObject::SetVertexOrientation(
    GeoPlaneState& object, std::uint8_t reversed,
    IGeoPlaneGeometryRuntime& runtime) {
    if (reversed == object.reversed_vertices) return reversed;
    object.reversed_vertices = reversed;
    return static_cast<std::uint8_t>(
        WriteGeometryVertices(object, runtime, true));
}

int GeoPlaneObject::SetGeometryMode(
    GeoPlaneState& object, int mode) {
    object.geometry_state = mode == 1 ? 3u : mode == 2 ? 2u : 1u;
    return mode;
}

std::uint8_t GeoPlaneObject::Create(
    GeoPlaneState& object, std::uint32_t texture,
    std::array<float, 2u> dimensions, std::uint8_t option,
    std::uint8_t plane_mode, IGeoPlaneCreateRuntime& runtime) {
    Destroy(object, runtime);
    const auto material = runtime.CreateMaterial();
    if (texture != 0u) runtime.SetMaterialTexture(material, texture);
    const auto geometry = runtime.CreateGeometry(4u, 2u, 77u);
    if (geometry == 0u)
        throw std::runtime_error(
            "create RpGeometry failed in geoPlane::create!");

    const auto triangle = runtime.ReadWord(geometry + 44u);
    const auto morph_target = runtime.ReadWord(geometry + 92u);
    const auto vertices = runtime.ReadWord(morph_target + 20u);
    const auto uv = runtime.ReadWord(geometry + 52u);
    const auto colors = runtime.ReadWord(geometry + 48u);
    const float half_width = dimensions[0] * 0.5f;
    const float top = plane_mode != 0u ? dimensions[1] * 0.5f
                                        : dimensions[1];
    const float bottom = plane_mode != 0u ? -top : 0.f;
    const std::array<std::array<float, 3u>, 4u> positions{{
        {-half_width, top, 0.f}, {-half_width, bottom, 0.f},
        {half_width, top, 0.f}, {half_width, bottom, 0.f}}};
    const std::array<std::array<float, 2u>, 4u> texcoords{{
        {0.f, 0.f}, {0.f, 1.f}, {1.f, 0.f}, {1.f, 1.f}}};
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            WriteFloat(runtime, vertices + index * 12u + axis * 4u,
                       positions[index][axis]);
        for (std::uint32_t axis = 0u; axis < 2u; ++axis)
            WriteFloat(runtime, uv + index * 8u + axis * 4u,
                       texcoords[index][axis]);
        for (std::uint32_t channel = 0u; channel < 4u; ++channel)
            runtime.WriteByte(colors + index * 4u + channel, 0xffu);
    }
    runtime.SetTriangleIndices(geometry, triangle, {0u, 1u, 2u});
    runtime.SetTriangleMaterial(geometry, triangle, material);
    runtime.SetTriangleIndices(geometry, triangle + 8u, {1u, 3u, 2u});
    runtime.SetTriangleMaterial(geometry, triangle + 8u, material);
    runtime.DestroyMaterial(material);
    const auto sphere = runtime.CalculateBoundingSphere(morph_target);
    for (std::uint32_t index = 0u; index < sphere.size(); ++index)
        runtime.WriteWord(morph_target + 4u + index * 4u, sphere[index]);
    runtime.UnlockGeometry(geometry);

    const auto atomic = runtime.CreateAtomic();
    const auto atomic_frame = runtime.CreateFrame();
    runtime.SetFrameIdentity(atomic_frame);
    runtime.SetAtomicFrame(atomic, atomic_frame);
    runtime.SetAtomicGeometry(atomic, geometry, 0u);
    runtime.DestroyGeometry(geometry);
    if (runtime.SharedAtomicCallback() == 0u)
        runtime.SetSharedAtomicCallback(runtime.ReadWord(atomic + 72u));

    object.clump = runtime.CreateClump();
    object.frame = runtime.CreateFrame();
    runtime.WriteWord(object.clump + 4u, object.frame);
    runtime.SetFrameIdentity(object.frame);
    runtime.AddAtomic(object.clump, atomic);
    runtime.AddFrameChild(object.frame, runtime.ReadWord(atomic + 4u));
    (void)runtime.SetMaterialFlagsForAllAtomics(object.clump, 0xffffffffu);

    object.atomic = atomic;
    object.width = dimensions[0];
    object.height = dimensions[1];
    object.material_flags = 0xffffffffu;
    object.rotation = {};
    object.scale = {1.f, 1.f, 1.f};
    object.position = {};
    object.vertex_bounds = {0u, 0u, 0x3f800000u, 0x3f800000u};
    object.active = 1u;
    object.option = option;
    (void)UpdateFrameTransform(object, runtime);
    return 1u;
}

void GeoPlaneObject::Destroy(GeoPlaneState& object,
                             IGeoPlaneDestroyRuntime& runtime) {
    if (object.active == 0u) return;
    object.active = 0u;
    const auto world = runtime.ClumpWorld(object.clump);
    if (world != 0u)
        runtime.RemoveClumpFromWorld(world, object.clump);
    if (object.clump != 0u) {
        runtime.ClearClumpFrame(object.clump);
        runtime.DestroyClump(object.clump);
        object.clump = 0u;
    }
    object.atomic = 0u;
    if (object.frame != 0u) {
        runtime.DestroyFrame(object.frame);
        object.frame = 0u;
    }
    object.rotation_first = 0u;
}

int GeoPlaneObject::Render(
    const GeoPlaneState& object, std::uint32_t first,
    std::uint32_t second, IGeoPlaneRenderRuntime& runtime) {
    if (object.active == 0u) return 0;

    const auto previous_27 = runtime.GetRenderState(27u);
    const auto previous_19 = runtime.GetRenderState(19u);
    const auto previous_20 = runtime.GetRenderState(20u);
    runtime.SetRenderState(27u, 1u);
    runtime.SetRenderState(19u, 5u);
    runtime.SetRenderState(20u, object.option != 0u ? 2u : 6u);
    const auto previous_7 = runtime.GetRenderState(7u);
    const auto previous_14 = runtime.GetRenderState(14u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(14u, 0u);

    const auto previous_device_20 = runtime.GetDeviceState(20u);
    runtime.SetDeviceState(20u, object.geometry_state);
    runtime.RenderClump(object.clump);
    runtime.SetDeviceState(20u, previous_device_20);
    runtime.SetDeviceState(1u, 0u);
    runtime.SetRenderState(7u, previous_7);
    runtime.SetRenderState(14u, previous_14);
    runtime.SetRenderState(27u, previous_27);
    runtime.SetRenderState(19u, previous_19);
    runtime.SetRenderState(20u, previous_20);
    runtime.SetTextureStage(first, second, 0u, 0u);
    runtime.SetTextureStage(first, second, 0u, 1u);
    (void)runtime.ClearStreamSource(0u);
    return runtime.ClearStreamSource(1u);
}

int GeoPlaneObject::UpdateFrameTransform(
    const GeoPlaneState& object, IGeoPlaneRuntime& runtime) {
    if (object.frame == 0u) return 0;
    const GeoPlaneVector axes[]{
        {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};
    const float angles[]{
        object.rotation.x, object.rotation.y, object.rotation.z};
    runtime.SetFrameIdentity(object.frame);
    if (object.rotation_first == 0u)
        runtime.ScaleFrame(object.frame, object.scale, 0u);
    for (std::size_t index = 0u; index < 3u; ++index)
        runtime.RotateFrame(object.frame, axes[index], angles[index],
                            index == 0u && object.rotation_first != 0u
                                ? 0u : 2u);
    if (object.rotation_first != 0u)
        runtime.ScaleFrame(object.frame, object.scale, 2u);
    runtime.TranslateFrame(object.frame, object.position, 2u);
    return runtime.UpdateFrameObjects(object.frame);
}

}  // namespace pal4
