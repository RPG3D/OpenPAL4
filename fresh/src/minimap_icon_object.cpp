#include "pal4/minimap_icon_object.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

std::int32_t AddSigned32(const std::int32_t left,
                         const std::int32_t right) {
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) +
        static_cast<std::uint32_t>(right));
}

std::int32_t SubSigned32(const std::int32_t left,
                         const std::int32_t right) {
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) -
        static_cast<std::uint32_t>(right));
}

}  // namespace

std::uint32_t MinimapIconState::ReadDword(
    const std::uint32_t offset) const {
    if (offset > kBytes - 4u) throw std::out_of_range("icon field");
    std::uint32_t value = 0u;
    std::memcpy(&value, bytes_.data() + offset, sizeof(value));
    return value;
}

void MinimapIconState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) {
    if (offset > kBytes - 4u) throw std::out_of_range("icon field");
    std::memcpy(bytes_.data() + offset, &value, sizeof(value));
}

std::uint32_t MinimapIconObject::Construct(
    const std::uint32_t address, IMinimapIconRuntime& runtime) {
    runtime.ConstructString(address + 8u);
    try {
        runtime.ConstructString(address + 0x14u);
    } catch (...) {
        (void)runtime.CleanupString(address + 8u);
        throw;
    }
    auto& state = runtime.Access(address);
    for (auto offset = 0x20u; offset <= 0x30u; offset += 4u) {
        state.WriteDword(offset, 0u);
    }
    state.WriteDword(4u, 0u);
    state.WriteDword(0u, 0xffu);
    for (auto offset = 0x34u; offset <= 0x40u; offset += 4u) {
        state.WriteDword(offset, 0u);
    }
    state.Bytes()[0x44u] = 0u;
    return address;
}

int MinimapIconObject::ReleaseTexture(
    const std::uint32_t address, IMinimapIconRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto texture = state.ReadDword(0x20u);
    if (texture == 0u) return 0;
    const int result = runtime.DestroyTexture(texture);
    state.WriteDword(0x20u, 0u);
    return result;
}

int MinimapIconObject::Destruct(
    const std::uint32_t address, IMinimapIconRuntime& runtime) {
    (void)ReleaseTexture(address, runtime);
    (void)runtime.CleanupString(address + 0x14u);
    return runtime.CleanupString(address + 8u);
}

int MinimapIconObject::Destroy(
    const std::uint32_t address, IMinimapIconRuntime& runtime) {
    (void)ReleaseTexture(address, runtime);
    if (address == 0u) return 0;
    (void)Destruct(address, runtime);
    return runtime.Free(address);
}

bool MinimapIconObject::LoadTexture(
    const std::uint32_t address, const std::string_view filename,
    const std::uint32_t property, IMinimapIconRuntime& runtime) {
    (void)ReleaseTexture(address, runtime);
    runtime.ProcessPackageScripts();
    const std::string path =
        std::string("gamedata\\ui\\minimapIcon\\") +
        std::string(filename);
    auto& state = runtime.Access(address);
    const auto texture = runtime.LookupTexture(path);
    state.WriteDword(0x20u, texture);
    const auto raster = texture == 0u ? 0u : runtime.ReadDword(texture);
    if (raster == 0u) {
        runtime.CleanupPackageScripts();
        return false;
    }
    state.WriteDword(0x2cu, runtime.ReadDword(raster + 0x0cu));
    state.WriteDword(0x30u, runtime.ReadDword(raster + 0x10u));
    state.WriteDword(0x34u, property);
    runtime.InitializeString(address + 0x14u, filename);
    runtime.CleanupPackageScripts();
    return true;
}

std::int32_t MinimapIconObject::SetCenteredCoordinates(
    MinimapIconState& state, const std::int32_t x,
    const std::int32_t y) {
    const auto width = std::bit_cast<std::int32_t>(
        state.ReadDword(0x2cu));
    const auto height = std::bit_cast<std::int32_t>(
        state.ReadDword(0x30u));
    const auto half_width = width / 2;
    const auto half_height = height / 2;
    state.WriteDword(0x24u,
                     static_cast<std::uint32_t>(x) -
                         static_cast<std::uint32_t>(half_width));
    state.WriteDword(0x28u,
                     static_cast<std::uint32_t>(y) -
                         static_cast<std::uint32_t>(half_height));
    return half_height;
}

std::uint32_t MinimapIconObject::CopyTriplet(
    MinimapIconState& state,
    const std::array<std::uint32_t, 3>& values) {
    state.WriteDword(0x38u, values[0]);
    state.WriteDword(0x3cu, values[1]);
    state.WriteDword(0x40u, values[2]);
    return values[2];
}

MinimapIconState& MinimapIconDrawStub::Access(std::uint32_t) {
    return state_;
}
bool MinimapIconDrawStub::Intersects(
    std::uint32_t, const std::array<std::int32_t, 4>&) {
    return false;
}
std::uint32_t MinimapIconDrawStub::IntersectionAlpha(
    std::uint32_t, const std::array<std::int32_t, 4>&) {
    return 0u;
}
std::uint32_t MinimapIconDrawStub::ReadDword(std::uint32_t) {
    return 0u;
}
void MinimapIconDrawStub::LogMissingRaster(std::uint32_t) {}
void MinimapIconDrawStub::ReloadTexture(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void MinimapIconDrawStub::DrawTransformed(
    std::uint32_t, std::uint32_t,
    const std::array<std::uint32_t, 4>&, std::uint32_t) {}
void MinimapIconDrawStub::DrawPlain(
    std::uint32_t, const std::array<std::uint32_t, 4>&,
    std::uint32_t, const std::array<std::uint8_t, 3>&,
    std::uint32_t) {}

float MinimapIconTransformStub::NearClip(std::uint32_t) { return 1.0f; }
float MinimapIconTransformStub::VertexDepth() { return 0.0f; }
std::uint32_t MinimapIconTransformStub::ReadDword(std::uint32_t) {
    return 0u;
}
void MinimapIconTransformStub::Translate(
    MinimapMatrix&, const MinimapVector3&, std::uint32_t) {}
void MinimapIconTransformStub::Rotate(
    MinimapMatrix&, const MinimapVector3&, float, std::uint32_t) {}
void MinimapIconTransformStub::TransformPoints(
    std::array<MinimapVector3, 4>&, const MinimapMatrix&) {}
void MinimapIconTransformStub::SetRenderState(
    std::uint32_t, std::uint32_t) {}
void MinimapIconTransformStub::DrawPrimitive(
    std::uint32_t, const std::array<MinimapIm2DVertex, 4>&) {}

bool MinimapIconObject::Draw(
    const std::uint32_t address, const std::uint32_t context,
    const std::int32_t x, const std::int32_t y,
    const std::array<std::int32_t, 4>& bounds,
    const bool bypass_intersection, std::uint32_t alpha,
    IMinimapIconDrawRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto texture = state.ReadDword(0x20u);
    if (texture == 0u) return false;
    if (!bypass_intersection) {
        if (!runtime.Intersects(address, bounds)) return false;
        alpha = runtime.IntersectionAlpha(address, bounds);
    }
    if (alpha == 0u) return false;

    const std::array<std::uint32_t, 4> rectangle{
        static_cast<std::uint32_t>(x) + state.ReadDword(0x24u) -
            static_cast<std::uint32_t>(bounds[0]),
        static_cast<std::uint32_t>(y) + state.ReadDword(0x28u) -
            static_cast<std::uint32_t>(bounds[1]),
        state.ReadDword(0x2cu), state.ReadDword(0x30u)};
    const auto raster = runtime.ReadDword(texture);
    if (raster == 0u) {
        const auto name_pointer = state.ReadDword(0x14u);
        runtime.LogMissingRaster(name_pointer);
        runtime.ReloadTexture(address, name_pointer,
                              state.ReadDword(0x34u));
        return false;
    }

    state.WriteDword(0x2cu, runtime.ReadDword(raster + 0x0cu));
    state.WriteDword(0x30u, runtime.ReadDword(raster + 0x10u));
    if (state.Bytes()[0x44u] != 0u) {
        runtime.DrawTransformed(address, context, rectangle, alpha);
    } else {
        runtime.DrawPlain(context, rectangle, texture,
                          {0xffu, 0xffu, 0xffu}, alpha);
    }
    return true;
}

bool MinimapIconObject::ContainsPoint(
    const MinimapIconState& state, const std::int32_t x,
    const std::int32_t y) {
    const auto left = std::bit_cast<std::int32_t>(
        state.ReadDword(0x24u));
    if (x < left) return false;
    const auto top = std::bit_cast<std::int32_t>(
        state.ReadDword(0x28u));
    if (y < top) return false;
    const auto width = std::bit_cast<std::int32_t>(
        state.ReadDword(0x2cu));
    if (x > AddSigned32(left, width)) return false;
    const auto height = std::bit_cast<std::int32_t>(
        state.ReadDword(0x30u));
    return y <= AddSigned32(top, height);
}

bool MinimapIconObject::FitsWithin(
    const MinimapIconState& state,
    const std::array<std::int32_t, 4>& bounds) {
    if (state.ReadDword(0x20u) == 0u) return false;
    const auto left = std::bit_cast<std::int32_t>(
        state.ReadDword(0x24u));
    const auto top = std::bit_cast<std::int32_t>(
        state.ReadDword(0x28u));
    const auto width = std::bit_cast<std::int32_t>(
        state.ReadDword(0x2cu));
    const auto height = std::bit_cast<std::int32_t>(
        state.ReadDword(0x30u));
    return AddSigned32(left, width) <= AddSigned32(bounds[0], bounds[2]) &&
           AddSigned32(top, height) <= AddSigned32(bounds[1], bounds[3]) &&
           left >= bounds[0] && top >= bounds[1];
}

std::int32_t MinimapIconObject::CalculateClipAlpha(
    const MinimapIconState& state,
    const std::array<std::int32_t, 4>& bounds,
    IMinimapIconFloatConverter& converter) {
    const auto base = std::bit_cast<std::int32_t>(state.ReadDword(0u));
    const auto minimum = std::bit_cast<std::int32_t>(
        state.ReadDword(4u));
    if (base == minimum) return base;

    const float range = static_cast<float>(SubSigned32(base, minimum));
    const auto left = std::bit_cast<std::int32_t>(
        state.ReadDword(0x24u));
    const auto top = std::bit_cast<std::int32_t>(
        state.ReadDword(0x28u));
    const auto width = std::bit_cast<std::int32_t>(
        state.ReadDword(0x2cu));
    const auto height = std::bit_cast<std::int32_t>(
        state.ReadDword(0x30u));

    float x_penalty = 0.0f;
    if (left < AddSigned32(bounds[0], width)) {
        const auto overlap = AddSigned32(
            SubSigned32(bounds[0], left), width);
        x_penalty = static_cast<float>(
            static_cast<double>(overlap) / width * range);
    } else if (AddSigned32(left, width) >
               SubSigned32(AddSigned32(bounds[0], bounds[2]), width)) {
        const auto overlap = SubSigned32(
            AddSigned32(left, AddSigned32(width, width)),
            AddSigned32(bounds[0], bounds[2]));
        x_penalty = static_cast<float>(
            static_cast<double>(overlap) / width * range);
    }

    double y_penalty = 0.0;
    if (top < AddSigned32(bounds[1], height)) {
        const auto overlap = AddSigned32(
            SubSigned32(bounds[1], top), height);
        y_penalty = static_cast<double>(overlap) / height * range;
    } else if (AddSigned32(top, height) >
               SubSigned32(AddSigned32(bounds[1], bounds[3]), height)) {
        const auto overlap = SubSigned32(
            AddSigned32(top, AddSigned32(height, height)),
            AddSigned32(bounds[1], bounds[3]));
        y_penalty = static_cast<double>(overlap) / height * range;
    }

    const double penalty =
        !(static_cast<double>(x_penalty) >= y_penalty) ?
            y_penalty : static_cast<double>(x_penalty);
    return converter.FloatToInt(static_cast<double>(base) - penalty);
}

bool MinimapIconObject::DrawTransformed(
    const MinimapIconState& state, const std::uint32_t context,
    const std::array<std::uint32_t, 4>& rectangle,
    const std::uint32_t alpha,
    IMinimapIconTransformRuntime& runtime) {
    if (context == 0u) return false;
    const auto texture = state.ReadDword(0x20u);
    if (texture == 0u) return false;

    const auto left = std::bit_cast<std::int32_t>(rectangle[0]);
    const auto top = std::bit_cast<std::int32_t>(rectangle[1]);
    const auto right = AddSigned32(
        left, std::bit_cast<std::int32_t>(rectangle[2]));
    const auto bottom = AddSigned32(
        top, std::bit_cast<std::int32_t>(rectangle[3]));
    const float reciprocal_z = 1.0f / runtime.NearClip(context);
    // IDA40EEED..40EF1B uses world X/Z; screen Y is projected from Z.
    std::array<MinimapVector3, 4> points{{
        {static_cast<float>(left), 0.0f, static_cast<float>(bottom)},
        {static_cast<float>(right), 0.0f, static_cast<float>(bottom)},
        {static_cast<float>(left), 0.0f, static_cast<float>(top)},
        {static_cast<float>(right), 0.0f, static_cast<float>(top)},
    }};

    const float center_x = static_cast<float>(
        (static_cast<double>(right) - left) * 0.5 + left);
    const float center_y = static_cast<float>(
        (static_cast<double>(bottom) - top) * 0.5 + top);
    MinimapMatrix matrix;
    runtime.Translate(matrix, {-center_x, -0.0f, -center_y}, 2u);
    runtime.TransformPoints(points, matrix);

    matrix = MinimapMatrix{};
    runtime.Rotate(matrix, {1.0f, 0.0f, 0.0f},
                   std::bit_cast<float>(state.ReadDword(0x38u)), 0u);
    runtime.Rotate(matrix, {0.0f, 1.0f, 0.0f},
                   std::bit_cast<float>(state.ReadDword(0x3cu)), 2u);
    runtime.Rotate(matrix, {0.0f, 0.0f, 1.0f},
                   std::bit_cast<float>(state.ReadDword(0x40u)), 2u);
    runtime.TransformPoints(points, matrix);

    matrix = MinimapMatrix{};
    runtime.Translate(matrix, {center_x, 0.0f, center_y}, 2u);
    runtime.TransformPoints(points, matrix);

    const auto color = (alpha << 24u) | 0x00ffffffu;
    std::array<MinimapIm2DVertex, 4> vertices{{
        {points[0].x, points[0].z, runtime.VertexDepth(),
         reciprocal_z, color, 0.0f, 1.0f},
        {points[1].x, points[1].z, runtime.VertexDepth(),
         reciprocal_z, color, 1.0f, 1.0f},
        {points[2].x, points[2].z, runtime.VertexDepth(),
         reciprocal_z, color, 0.0f, 0.0f},
        {points[3].x, points[3].z, runtime.VertexDepth(),
         reciprocal_z, color, 1.0f, 0.0f},
    }};
    const std::array<std::array<std::uint32_t, 2>, 8> states{{
        {12u, 1u}, {10u, 5u}, {11u, 6u}, {6u, 0u},
        {8u, 0u}, {7u, 2u}, {9u, 2u}, {2u, 3u},
    }};
    for (const auto& pair : states) {
        runtime.SetRenderState(pair[0], pair[1]);
    }
    runtime.SetRenderState(1u, runtime.ReadDword(texture));
    runtime.DrawPrimitive(4u, vertices);
    return true;
}

static_assert(sizeof(MinimapMatrix) == 64u);
static_assert(sizeof(MinimapIm2DVertex) == 28u);

}  // namespace pal4
