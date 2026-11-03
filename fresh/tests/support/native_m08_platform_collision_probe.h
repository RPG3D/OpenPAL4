#pragma once

#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_clump_collision_query.h"
#include "pal4/native_movement_secondary_helpers.h"
#include "pal4/native_rw_plugin_8452_processor.h"
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/rw_plugin_8452_native.h"

#include <rpworld.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace m08_platform_probe {
inline void Check(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}

struct Helper final : INativeMovementSecondaryHelper {
    RpClump* clump{};
    unsigned enters{}, leaves{};
    explicit Helper(RpClump* value) : clump(value) {}
    RpClump* Clump() const override { return clump; }
    double SwitchThreshold() const override { return 15.0; }
    void Enter() override { ++enters; }
    void Leave() override { ++leaves; }
};

inline std::vector<RpAtomic*> Atomics(RpClump* clump) {
    std::vector<RpAtomic*> result;
    RpClumpForAllAtomics(clump, [](RpAtomic* atomic, void* data) -> RpAtomic* {
        static_cast<std::vector<RpAtomic*>*>(data)->push_back(atomic);
        return atomic;
    }, &result);
    return result;
}

inline void SetGate(const std::vector<RpAtomic*>& atomics, std::uint32_t value) {
    const auto offset = NativeRwPlugin8452Lifecycle::State().geometry_offset;
    Check(offset > 0, "8452 atomic plugin registered before real platform load");
    for (auto* atomic : atomics) {
        auto* plugin = reinterpret_cast<std::uint32_t*>(
            reinterpret_cast<char*>(atomic) + offset);
        Check(plugin[2] == 1u, "8452 processed platform retains its active flag");
        plugin[1] = value; // Fixture-only cached frustum result, not a game patch.
    }
}

inline CollisionPoint3 FindProbePoint(RpClump* clump) {
    for (auto* atomic : Atomics(clump)) {
        auto* geometry = RpAtomicGetGeometry(atomic);
        auto* vertices = RpMorphTargetGetVertices(RpGeometryGetMorphTarget(geometry, 0));
        const auto* triangles = RpGeometryGetTriangles(geometry);
        for (RwInt32 i = 0; i < RpGeometryGetNumTriangles(geometry); ++i) {
            RwV3d point{};
            for (unsigned vertex = 0; vertex < 3u; ++vertex) {
                const auto& v = vertices[triangles[i].vertIndex[vertex]];
                point.x += v.x / 3.0f; point.y += v.y / 3.0f; point.z += v.z / 3.0f;
            }
            RwV3d world{};
            RwV3dTransformPoint(&world, &point, RwFrameGetLTM(RpAtomicGetFrame(atomic)));
            NativeClumpCollisionHit hit;
            if (NativeClumpCollisionQuery::Closest(
                {{{world.x, world.y + 1000.0f, world.z}},
                 {{world.x, world.y - 1000.0f, world.z}}}, clump, hit))
                return hit.point;
        }
    }
    throw std::runtime_error("M08/2 floor01 has no usable real SDK vertical collision");
}
}  // namespace m08_platform_probe

inline void VerifyNativeM08PlatformCollision(const std::filesystem::path& root) {
    using namespace m08_platform_probe;
    NativeSceneGobCatalog catalog;
    Check(catalog.Load(root, "M08", "2"), "M08/2 original GOB catalog");
    const auto record = std::find_if(catalog.Records().begin(), catalog.Records().end(),
        [](const auto& r) { return r.name == "floor01"; });
    Check(record != catalog.Records().end() && record->type == 2u,
          "M08/2 floor01 is the real LocalFloat boundary");
    NativeGobClumpAsset lower, upper;
    Check(lower.Load(root, record->resource_dir, record->model) &&
          upper.Load(root, record->resource_dir, record->model), "M08/2 actual platform models");
    const auto point = FindProbePoint(lower.Clump());
    const RwV3d shift{0.0f, 40.0f, 0.0f};
    RwFrameTranslate(RpClumpGetFrame(upper.Clump()), &shift, rwCOMBINEPOSTCONCAT);
    RwFrameUpdateObjects(RpClumpGetFrame(upper.Clump()));
    Helper a(lower.Clump()), b(upper.Clump()), alias(lower.Clump());
    NativeMovementSecondaryHelpers helpers;
    Check(helpers.Add(101u, a) && helpers.Add(102u, b) &&
          !helpers.Add(103u, alias) && !helpers.Add(101u, a) && helpers.Count() == 2u,
          "helper collection deduplicates actual borrowed clumps and identities");
    bool rejected{};
    try { (void)helpers.Add(101u, b); }
    catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected && helpers.Count() == 2u, "helper identity cannot silently change owner");

    CollisionPoint3 on_upper{point[0], point[1] + 40.0f, point[2]}, output{};
    Check(helpers.FindHighest(on_upper, 100.0f, 100.0f, &output) &&
          std::fabs(output[1] - on_upper[1]) < .01f,
          "real SDK collision selects highest translated platform");
    const auto upper_atomic = helpers.State().words[33];
    const auto upper_atomics = Atomics(upper.Clump());
    Check(std::any_of(upper_atomics.begin(), upper_atomics.end(), [&](auto* p) {
        return reinterpret_cast<std::uint32_t>(p) == upper_atomic;
    }) && helpers.State().words[32] != 0xffffffffu,
          "secondary collision records real atomic and triangle identity");
    Check(helpers.UpdateActive(on_upper, 100.0f, 100.0f) &&
          helpers.ActiveClump() == upper.Clump() && b.enters == 1u,
          "platform helper enters at actual collision height");
    Check(helpers.UpdateActive(on_upper, 100.0f, 100.0f) && b.enters == 1u,
          "same platform does not duplicate Enter callbacks");
    Check(helpers.HasCollision(on_upper, 100.0f, 100.0f, true) &&
          !helpers.HasCollision({on_upper[0], on_upper[1] + 11.0f, on_upper[2]},
              100.0f, 100.0f, true), "secondary height gate preserves original ten-unit limit");

    // 8452 admission is deliberately controlled only inside this fixture.
    Check(NativeRwPlugin8452Processor::ProcessClump(upper.Clump()) == 1u,
          "8452 wraps original platform atomics");
    SetGate(upper_atomics, 0u);
    Check(!NativeRwPlugin8452Processor::QueryClump(upper.Clump()),
          "processed platform with all cached results zero is rejected");
    Check(helpers.FindHighest(on_upper, 100.0f, 100.0f, &output) &&
          std::fabs(output[1] - point[1]) < .01f,
          "8452 gate excludes the higher SDK collision from selection");
    Check(!helpers.UpdateActive(on_upper, 100.0f, 100.0f) &&
          helpers.ActiveClump() == upper.Clump() && !b.leaves && !a.enters,
          "platform switch beyond fifteen units preserves existing helper");
    Check(helpers.UpdateActive(point, 100.0f, 100.0f) &&
          helpers.ActiveClump() == lower.Clump() && b.leaves == 1u && a.enters == 1u,
          "real lower collision switches with old Leave and new Enter");
    SetGate(upper_atomics, 1u);
    Check(NativeRwPlugin8452Processor::QueryClump(upper.Clump()),
          "8452 cached nonzero result admits processed platform");
    NativeRwPlugin8452Processor::RestoreClump(upper.Clump());
    Check(NativeRwPlugin8452Processor::QueryClump(upper.Clump()),
          "unprocessed platform admits collision without a render pass");

    // Equal height preserves earlier insertion order, matching strict comparison.
    const RwV3d undo{0.0f, -40.0f, 0.0f};
    RwFrameTranslate(RpClumpGetFrame(upper.Clump()), &undo, rwCOMBINEPOSTCONCAT);
    RwFrameUpdateObjects(RpClumpGetFrame(upper.Clump()));
    Check(helpers.FindHighest(point, 100.0f, 100.0f, &output) &&
          helpers.State().words[33] != upper_atomic &&
          helpers.UpdateActive(point, 100.0f, 100.0f) && a.enters == 1u,
          "equal-height collisions preserve helper insertion order");
    helpers.ClearActive();
    Check(a.leaves == 1u && !helpers.ActiveClump(), "explicit active clear invokes Leave once");
    Check(helpers.UpdateActive(point, 100.0f, 100.0f) && a.enters == 2u &&
          helpers.Remove(102u) && !helpers.ActiveClump() && a.leaves == 1u,
          "original Remove clears active even for another helper and does not invoke Leave");
    Check(helpers.Remove(101u) && !helpers.Remove(101u) && !helpers.Count(),
          "borrowed helpers detach before model destruction");
    const auto old_atomic = helpers.State().words[33];
    output = {7.0f, 8.0f, 9.0f};
    Check(!helpers.FindHighest(point, 100.0f, 100.0f, &output) &&
          helpers.State().words[32] == 0xffffffffu &&
          helpers.State().words[33] == old_atomic && output == CollisionPoint3{7.0f, 8.0f, 9.0f},
          "empty query resets only triangle and retains atomic/output like original");
    std::cout << "Native M08 LocalFloat collision: original floor01 model, SDK hits, 8452 admission, highest/tie selection, height gates, helper Enter/Leave and borrowed detach pass\n";
}
}  // namespace pal4::test
