#include "pal4/effect_visual_object.h"

#include <bit>

namespace pal4 {
namespace {

EffectVector3 Subtract(EffectVector3 a, EffectVector3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

EffectVector3 Cross(EffectVector3 a, EffectVector3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

EffectMatrix Orientation(EffectVector3 right, EffectVector3 up,
                         EffectVector3 at) {
    EffectMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = at;
    matrix.flags = 0x20003u;
    return matrix;
}

std::array<std::uint32_t, 3u> ObjectScale(
    const EffectVisualObjectState& object) {
    return {object.scale[0], object.scale[1], 0x3f800000u};
}

}  // namespace

std::uint32_t EffectVisualObject::AllocateGeometry(
    IEffectVisualObjectRuntime& runtime) {
    const auto geometry = runtime.Allocate(0x54u);
    if (geometry != 0u) {
        runtime.WriteGeometryByte(geometry, 0u);
        runtime.WriteGeometryByte(geometry + 1u, 0u);
        runtime.WriteGeometryByte(geometry + 2u, 0u);
        runtime.WriteGeometryWord(geometry + 4u, 3u);
        runtime.WriteGeometryWord(geometry + 8u, 0u);
        runtime.WriteGeometryWord(geometry + 12u, 0u);
        runtime.WriteGeometryWord(geometry + 16u, 0u);
    }
    return geometry;
}

void EffectVisualObject::ResetTransforms(EffectVisualObjectState& object) {
    for (auto& value : object.translation) value = 0u;
    object.scale[0] = 0x3f800000u;
    object.scale[1] = 0x3f800000u;
}

void EffectVisualObject::ReleaseFailedGeometry(
    EffectVisualObjectState& object,
    IEffectVisualObjectRuntime& runtime) {
    if (object.geometry != 0u) {
        runtime.DestroyGeometry(object.geometry);
        runtime.Free(object.geometry);
    }
    object.geometry = 0u;
}

std::int32_t EffectVisualObject::SetEnabled(
    EffectVisualObjectState& object, std::uint8_t enabled,
    IEffectVisualObjectRuntime& runtime) {
    object.enabled = enabled;
    const auto mode = enabled != 0u || object.geometry_mode != 0u ? 0 : 2;
    return runtime.SetGeometryState(object.geometry, mode);
}

std::uint8_t EffectVisualObject::Initialize(
    EffectVisualObjectState& object, std::uint32_t source,
    std::uint32_t vector, std::uint8_t geometry_mode,
    std::uint8_t property_flag, std::uint32_t option,
    IEffectVisualObjectRuntime& runtime) {
    Deinitialize(object, runtime);
    object.geometry = AllocateGeometry(runtime);
    if (!runtime.InitializeGeometry(
            object.geometry, source, vector, geometry_mode, option)) {
        ReleaseFailedGeometry(object, runtime);
        return object.active;
    }
    ResetTransforms(object);
    object.transform_mode = 1u;
    object.frame = runtime.CreateFrame();
    (void)InitializeTransform(object, runtime);
    runtime.AddFrameChild(
        object.frame, runtime.ReadGeometryWord(object.geometry + 16u));
    object.property_flag = property_flag;
    (void)runtime.SetGeometryState(
        object.geometry, object.geometry_mode != 0u ? 0 : 2);
    object.active = 1u;
    return object.active;
}

std::uint8_t EffectVisualObject::InitializeFromObject(
    EffectVisualObjectState& object,
    const EffectVisualObjectState& template_object,
    std::uint32_t source, std::uint32_t vector,
    std::uint8_t geometry_mode, std::uint8_t property_flag,
    std::uint32_t option, IEffectVisualObjectRuntime& runtime) {
    Deinitialize(object, runtime);
    object.geometry = AllocateGeometry(runtime);
    if (!runtime.InitializeGeometry(
            object.geometry, source, vector, geometry_mode, option)) {
        ReleaseFailedGeometry(object, runtime);
        return object.active;
    }
    ResetTransforms(object);
    object.transform_mode = 0u;
    object.owner = template_object.owner;
    object.source = template_object.source;
    object.context = template_object.context;
    (void)runtime.SetGeometryState(
        object.geometry, object.geometry_mode != 0u ? 0 : 2);
    object.frame = runtime.CreateFrame();
    (void)InitializeTransform(object, runtime);
    runtime.AddFrameChild(
        object.frame, runtime.ReadGeometryWord(object.geometry + 16u));
    object.active = 1u;
    object.property_flag = property_flag;
    return object.active;
}

void EffectVisualObject::Deinitialize(
    EffectVisualObjectState& object,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return;
    object.active = 0u;
    object.transform_mode = 1u;
    object.property_flag = 1u;
    for (auto& value : object.translation) value = 0u;
    object.scale[0] = 0u;
    object.scale[1] = 0u;
    if (object.frame != 0u) {
        runtime.DestroyFrame(object.frame);
        object.frame = 0u;
    }
    if (object.geometry != 0u) {
        runtime.DestroyGeometry(object.geometry);
        runtime.Free(object.geometry);
        object.geometry = 0u;
    }
}

std::uint8_t EffectVisualObject::Update(
    EffectVisualObjectState& object, std::uint32_t first,
    std::uint32_t second, std::uint32_t value,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    runtime.NoopUpdate(first, second);
    if (object.geometry_mode != 0u)
        return UpdateRotation(object, runtime);
    if (object.transform_mode != 0u)
        return UpdateTransform(object, value, runtime);
    return UpdateTransformComplex(object, runtime);
}

std::uint8_t EffectVisualObject::InitializeTransform(
    EffectVisualObjectState& object,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    const auto frame = object.frame;
    constexpr std::array<std::uint32_t, 3u> unit{
        0x3f800000u, 0x3f800000u, 0x3f800000u};
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, unit, 0u);
    runtime.RotateFrame(frame, 0x84b35cu, 0.0f, 2u);
    runtime.RotateFrame(frame, 0x84b368u, 0.0f, 2u);
    runtime.RotateFrame(frame, 0x84b374u, 0.0f, 2u);
    runtime.TranslateFrame(
        frame, {object.translation[1], object.translation[2],
                object.translation[3]}, 2u);
    return runtime.UpdateFrameObjects(frame);
}

std::uint8_t EffectVisualObject::UpdateRotation(
    EffectVisualObjectState& object,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    const auto frame = runtime.ReadGeometryWord(object.geometry + 16u);
    const std::array<std::uint32_t, 3u> scale{
        object.scale[0], object.scale[1], 0x3f800000u};
    runtime.SetFrameIdentity(frame);
    runtime.RotateFrame(frame, 0x84b374u,
                        std::bit_cast<float>(object.translation[0]), 0u);
    runtime.ScaleFrame(frame, scale, 2u);
    return runtime.UpdateFrameObjects(frame);
}

std::uint8_t EffectVisualObject::UpdateTransform(
    EffectVisualObjectState& object, std::uint32_t value,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    const auto input = runtime.GetObjectTransform(
        runtime.ReadObjectWord(value + 4u));
    const auto child = runtime.ReadGeometryWord(object.geometry + 16u);
    const auto parent = runtime.GetObjectTransform(object.frame);
    EffectVector3 right{};
    EffectVector3 up{};
    EffectVector3 at{};
    if (object.property_flag != 0u) {
        up = {0.0f, 1.0f, 0.0f};
        at = Subtract(parent.position, input.position);
        runtime.Normalize(at);
        right = Cross(at, up);
        runtime.Normalize(right);
    } else {
        up = input.up;
        right = Subtract(input.position, parent.position);
        runtime.Normalize(right);
        at = Cross(up, right);
        runtime.Normalize(at);
        // IDA64309A..6430C6: view direction cross computed horizontal axis.
        up = Cross(right, at);
        runtime.Normalize(up);
    }
    const auto parent_matrix = runtime.SetupParentTransform(object.frame);
    runtime.TransformVector(up, parent_matrix);
    if (object.property_flag != 0u) {
        runtime.TransformVector(at, parent_matrix);
        runtime.TransformVector(right, parent_matrix);
    } else {
        runtime.TransformVector(right, parent_matrix);
        runtime.TransformVector(at, parent_matrix);
    }
    runtime.Normalize(up);
    if (object.property_flag != 0u) {
        runtime.Normalize(at);
        runtime.Normalize(right);
    } else {
        runtime.Normalize(right);
        runtime.Normalize(at);
        runtime.Normalize(up);
        runtime.Normalize(right);
        runtime.Normalize(at);
    }
    // IDA642FAD..6431E3 swaps the two computed axes in the camera-up
    // branch: its cross product is matrix.right, view direction matrix.at.
    auto matrix = object.property_flag != 0u
        ? Orientation(right, up, at) : Orientation(at, up, right);
    runtime.UpdateMatrix(matrix);
    runtime.SetFrameIdentity(child);
    const auto angle = std::bit_cast<float>(object.translation[0]);
    if (object.enabled != 0u) {
        runtime.RotateFrame(child, 0x84b35cu, -90.0f, 0u);
        runtime.RotateFrame(child, 0x84b374u, angle, 2u);
    } else {
        runtime.RotateFrame(child, 0x84b374u, angle, 0u);
    }
    runtime.ScaleFrame(child, ObjectScale(object), 2u);
    runtime.ApplyMatrix(child, matrix, 2u);
    return runtime.UpdateFrameObjects(child);
}

std::uint8_t EffectVisualObject::UpdateTransformComplex(
    EffectVisualObjectState& object,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    const auto child = runtime.ReadGeometryWord(object.geometry + 16u);
    const auto parent = runtime.GetObjectTransform(object.frame);
    const EffectVector3 position{
        std::bit_cast<float>(object.owner),
        std::bit_cast<float>(object.source),
        std::bit_cast<float>(object.context)};
    EffectVector3 right{};
    EffectVector3 up{};
    EffectVector3 at{};
    if (object.property_flag != 0u) {
        up = {0.0f, 1.0f, 0.0f};
        at = Subtract(position, parent.position);
        runtime.Normalize(at);
        right = Cross(at, up);
        runtime.Normalize(right);
    } else {
        at = Subtract(position, parent.position);
        runtime.Normalize(at);
        right = Cross(at, {0.0f, 1.0f, 0.0f});
        runtime.Normalize(right);
        up = Cross(right, at);
        runtime.Normalize(up);
    }
    const auto parent_matrix = runtime.SetupParentTransform(object.frame);
    runtime.TransformVector(up, parent_matrix);
    runtime.TransformVector(at, parent_matrix);
    runtime.TransformVector(right, parent_matrix);
    for (int pass = 0; pass != 2; ++pass) {
        runtime.Normalize(up);
        runtime.Normalize(at);
        runtime.Normalize(right);
    }
    auto matrix = Orientation(right, up, at);
    runtime.UpdateMatrix(matrix);
    runtime.SetFrameIdentity(child);
    runtime.ScaleFrame(child, ObjectScale(object), 0u);
    runtime.RotateFrame(child, 0x84b374u,
                        std::bit_cast<float>(object.translation[0]), 2u);
    if (object.enabled != 0u)
        runtime.RotateFrame(child, 0x84b35cu, -90.0f, 2u);
    runtime.ApplyMatrix(child, matrix, 2u);
    return runtime.UpdateFrameObjects(child);
}

std::uint8_t EffectVisualObject::Render(
    const EffectVisualObjectState& object,
    std::uint32_t first, std::uint32_t second,
    IEffectVisualObjectRuntime& runtime) {
    if (object.active == 0u) return 0u;
    return runtime.RenderWithStates(object.geometry, first, second);
}

}  // namespace pal4
