#include "pal4/geo_cylinder.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

constexpr double kRadiansPerDegree = 0.017453292;

struct AtomicAndFrame {
    std::uint32_t atomic{};
    std::uint32_t frame{};
};

void AppendPair(std::vector<CylinderVertexPair>& pairs,
                const CylinderVertexPair& pair, int copies) {
    for (int index = 0; index < copies; ++index)
        pairs.push_back(pair);
}

CylinderVertexPair MakePair(float angle, float first_radius,
                            float second_radius, float top,
                            float bottom, CylinderUv first_uv,
                            CylinderUv second_uv) {
    const auto cosine = std::cos(angle);
    const auto sine = std::sin(angle);
    return {{cosine * first_radius, top, sine * first_radius},
            {cosine * second_radius, bottom, sine * second_radius},
            first_uv, second_uv};
}

std::vector<CylinderVertexPair> MakePairs(
    const CylinderCreateOptions& options, float half_u,
    float half_v) {
    const float top = options.centered ? options.height * 0.5f
                                       : options.height;
    const float bottom = options.centered ? options.height * -0.5f
                                          : 0.0f;
    const int steps = options.bordered ? options.border_count * 2
                                      : options.border_count;
    const float angle_step = static_cast<float>(
        static_cast<double>(options.arc_degrees) * kRadiansPerDegree /
        static_cast<double>(steps));
    std::vector<CylinderVertexPair> pairs;
    pairs.reserve(static_cast<std::size_t>(options.bordered ? 8 * options.border_count
                                                            : 2 * options.border_count));
    for (int index = 0; index <= steps; ++index) {
        const auto outer_fraction = options.bordered
            ? static_cast<float>(2 * index) / static_cast<float>(steps)
            : static_cast<float>(index) / static_cast<float>(steps);
        const auto outer_angle = options.bordered
            ? static_cast<float>(2 * index) * angle_step
            : static_cast<float>(index) * angle_step;
        CylinderUv first_uv{};
        CylinderUv second_uv{};
        if (options.alternate_uv) {
            first_uv = {options.bordered ? 0.0f : half_u,
                        outer_fraction + (options.bordered ? 0.0f : half_v)};
            second_uv = {1.0f + half_u, outer_fraction + half_v};
        } else {
            first_uv = {outer_fraction + (options.bordered ? 0.0f : half_u),
                        options.bordered ? 0.0f : half_v};
            second_uv = {outer_fraction + half_u, 1.0f + half_v};
        }
        const auto outer = MakePair(
            outer_angle, options.first_radius, options.second_radius,
            top, bottom, first_uv, second_uv);
        AppendPair(pairs, outer,
                   index == 0 || index == steps ? 1 : 2);

        if (options.bordered && index != steps) {
            const float inner_fraction =
                static_cast<float>(2 * index + 1) /
                static_cast<float>(steps);
            CylinderUv inner_first{};
            CylinderUv inner_second{};
            if (options.alternate_uv) {
                inner_first = {half_u, inner_fraction + half_v};
                inner_second = {1.0f + half_u, inner_fraction + half_v};
            } else {
                inner_first = {inner_fraction + half_u, half_v};
                inner_second = {inner_fraction + half_u, 1.0f + half_v};
            }
            const auto inner = MakePair(
                static_cast<float>(2 * index + 1) * angle_step,
                options.first_radius - options.border_width,
                options.second_radius - options.border_width,
                top, bottom, inner_first, inner_second);
            AppendPair(pairs, inner, 2);
        }
    }
    return pairs;
}

AtomicAndFrame MakeAtomic(const CylinderVertexPair& first,
                          const CylinderVertexPair& second,
                          std::uint32_t texture,
                          IGeoCylinderRuntime& runtime) {
    const auto geometry = runtime.CreateGeometry(4u, 2u, 77u);
    if (geometry == 0u)
        throw std::runtime_error(
            "create RpGeometry failed in geoCylinder::create!");
    constexpr std::uint32_t white = 0xffffffffu;
    runtime.WriteVertex(geometry, 0u, first.first, first.first_uv, white);
    runtime.WriteVertex(geometry, 1u, first.second, first.second_uv, white);
    runtime.WriteVertex(geometry, 2u, second.first, second.first_uv, white);
    runtime.WriteVertex(geometry, 3u, second.second, second.second_uv, white);
    const auto material = runtime.CreateMaterial();
    if (texture != 0u) runtime.SetMaterialTexture(material, texture);
    runtime.SetTriangle(geometry, 0u, 0u, 1u, 2u, material);
    runtime.SetTriangle(geometry, 1u, 1u, 3u, 2u, material);
    runtime.DestroyMaterial(material);
    runtime.FinishGeometry(geometry);
    const auto atomic = runtime.CreateAtomic();
    const auto frame = runtime.CreateFrame();
    runtime.SetAtomicFrame(atomic, frame);
    runtime.SetAtomicGeometry(atomic, geometry);
    runtime.DestroyGeometry(geometry);
    runtime.CaptureSharedAtomicFlags(atomic);
    return {atomic, frame};
}

}  // namespace

std::uint8_t GeoCylinder::Create(
    VisualModelState& model, const CylinderCreateOptions& options,
    IGeoCylinderRuntime& runtime) {
    if (options.border_count < 3)
        throw std::runtime_error("Cylinder's border number is less than 3");
    const float half_u = options.texture == 0u
        ? 0.0f
        : 0.5f / static_cast<float>(runtime.TextureWidth(options.texture));
    const float half_v = options.texture == 0u
        ? 0.0f
        : 0.5f / static_cast<float>(runtime.TextureHeight(options.texture));
    const auto pairs = MakePairs(options, half_u, half_v);
    std::vector<AtomicAndFrame> atomics;
    atomics.reserve(pairs.size() / 2u);
    for (std::size_t index = 0; index < pairs.size(); index += 2u)
        atomics.push_back(MakeAtomic(
            pairs[index], pairs[index + 1u], options.texture, runtime));

    model.clump = runtime.CreateClump();
    model.frame = runtime.CreateFrame();
    runtime.SetClumpFrame(model.clump, model.frame);
    runtime.SetFrameIdentity(model.frame);
    for (const auto& entry : atomics) {
        runtime.AddAtomic(model.clump, entry.atomic);
        runtime.AddChildFrame(model.frame, entry.frame);
    }
    model.material_flags = 0xfeffffffu;
    model.rotation = {};
    model.scale = {1.0f, 1.0f, 1.0f};
    model.position = {};
    runtime.SetMaterialFlagsForAllAtomics(
        model.clump, model.material_flags);
    model.active = 1u;
    model.property = options.property;
    return model.active;
}

}  // namespace pal4
