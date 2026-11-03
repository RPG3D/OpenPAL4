#include "pal4/gob_climb.h"
#include "pal4/gob_scene_package.h"

#include <cmath>

namespace pal4 {

std::uint32_t GobClimb::TypeKey() { return 4u; }

std::uint32_t GobClimb::Create(IGobClimbRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobClimb::Construct(
    const std::uint32_t object, IGobClimbRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object, kVtable);
    return object;
}

void GobClimb::Destruct(
    const std::uint32_t object, IGobClimbRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.DestructBase(object);
}

std::uint32_t GobClimb::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobClimbRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto loaded = runtime.LoadBaseModel(
        object, first, path, fourth, fifth);
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (loaded == 0u || document == 0u) {
        runtime.CleanupScriptPackages();
        return 0u;
    }
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters != 0u) {
        const auto climb = runtime.FindXmlChild(
            parameters, "PAL4_GameObject-climb");
        if (climb != 0u)
            runtime.RemoveXmlChild(parameters, climb);
        (void)runtime.CreateXmlChild(
            parameters, "PAL4_GameObject-climb");
    }
    runtime.CleanupScriptPackages();
    return loaded;
}

std::uint32_t GobClimb::GetConfigNode(
    const std::uint32_t object, IGobClimbRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters == 0u) return 0u;
    return runtime.FindXmlChild(parameters,
                                "PAL4_GameObject-climb");
}

bool GobClimb::ReadEndpoints(
    const std::uint32_t object, Position& a, Position& b,
    IGobClimbRuntime& runtime) {
    const auto node = GetConfigNode(object, runtime);
    if (node == 0u) return false;
    const auto read_float = [&](const std::string_view name) {
        const auto attribute = runtime.FindXmlAttribute(node, name);
        return attribute == 0u ? 0.0f :
            runtime.XmlAttributeToFloat(attribute, 0.0f);
    };
    a = Position{
        read_float("PAL4_GameObject-climb-posXA"),
        read_float("PAL4_GameObject-climb-posYA"),
        read_float("PAL4_GameObject-climb-posZA")};
    b = Position{
        read_float("PAL4_GameObject-climb-posXB"),
        read_float("PAL4_GameObject-climb-posYB"),
        read_float("PAL4_GameObject-climb-posZB")};
    return true;
}

std::uint32_t GobClimb::BeginMotionFromConfig(
    const std::uint32_t object, const Position& actor_position,
    IGobClimbRuntime& runtime) {
    Position a{};
    Position b{};
    if (!ReadEndpoints(object, a, b, runtime)) return 0u;

    float heading_x = b.y > a.y ? b.x - a.x : a.x - b.x;
    float heading_z = b.y > a.y ? b.z - a.z : a.z - b.z;
    runtime.NormalizeHorizontal(heading_x, heading_z);
    auto heading = runtime.HeadingDegrees(heading_x, heading_z);
    if (std::fabs(heading) < 0.005f)
        heading = runtime.ClumpFacingDegrees(object);

    const auto squared_distance = [&](const Position& point) {
        const double dx = static_cast<double>(point.x) - actor_position.x;
        const double dy = static_cast<double>(point.y) - actor_position.y;
        const double dz = static_cast<double>(point.z) - actor_position.z;
        return dx * dx + dy * dy + dz * dz;
    };
    const bool a_is_near = squared_distance(a) <= squared_distance(b);
    return runtime.BeginActorMotion(
        a_is_near ? a : b, a_is_near ? b : a,
        heading, a_is_near ? 1u : 0u);
}

bool GobClimb::IsWithinEndpointRange(
    const std::uint32_t object, const Position& actor_position,
    float* squared_distance, IGobClimbRuntime& runtime) {
    Position a{};
    Position b{};
    if (!ReadEndpoints(object, a, b, runtime)) return false;
    const auto squared_3d = [&](const Position& point) {
        const double dx = static_cast<double>(point.x) - actor_position.x;
        const double dy = static_cast<double>(point.y) - actor_position.y;
        const double dz = static_cast<double>(point.z) - actor_position.z;
        return dx * dx + dy * dy + dz * dz;
    };
    const auto& nearest = squared_3d(a) <= squared_3d(b) ? a : b;
    const float dx = nearest.x - actor_position.x;
    const float dz = nearest.z - actor_position.z;
    // The original FPU sequence cancels the selected endpoint's Y delta.
    const float horizontal_squared = dx * dx + dz * dz;
    const auto threshold = runtime.InteractionThreshold(object);
    if (squared_distance != nullptr)
        *squared_distance = horizontal_squared;
    return !std::isunordered(threshold, horizontal_squared) &&
           threshold >= horizontal_squared;
}

std::uint32_t GobClimb::CreateStateSnapshot(
    const std::uint32_t object, IGobClimbRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x18u);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(
            snapshot, runtime.ReadCString(runtime.ReadWord(object + 4u)));
        runtime.WriteWord(snapshot + 0x14u,
                          runtime.ReadWord(object + 0x184u));
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

void GobClimb::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobClimbRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u ||
        runtime.SnapshotType(snapshot) != TypeKey()) return;
    runtime.SetBaseVisible(
        object, runtime.ReadWord(snapshot + 0x14u));
}

}  // namespace pal4
