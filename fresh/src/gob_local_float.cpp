#include "pal4/gob_local_float.h"
#include "pal4/gob_scene_package.h"

#include <array>
#include <cmath>
#include <numbers>

namespace pal4 {

using Vector3 = IGobLocalFloatRuntime::Vector3;

std::uint32_t GobLocalFloat::Create(IGobLocalFloatLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobLocalFloat::Construct(
    const std::uint32_t object, IGobLocalFloatLifecycleRuntime& runtime) {
    runtime.ConstructMachine(object);
    runtime.WriteWord(object + 0x2ccu, kHelperBaseVtable);
    runtime.WriteWord(object + 0x2d0u, 0u);
    runtime.WriteWord(object + 0x2d4u, 0u);
    runtime.WriteWord(object + 0x2dcu, 0u);
    runtime.WriteWord(object + 0x2e0u, 0u);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x1dcu, kMachineMemberVtable);
    runtime.WriteWord(object + 0x2ccu, kHelperVtable);
    runtime.WriteWord(object + 0x2d8u, 2u);
    return object;
}

std::uint32_t GobLocalFloat::PrimaryTaskContainer(
    const std::uint32_t object) {
    return object + 0x258u;
}

double GobLocalFloat::InteractionThreshold() { return 15.0; }

void GobLocalFloat::Destruct(
    const std::uint32_t object, IGobLocalFloatLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x1dcu, kMachineMemberVtable);
    runtime.WriteWord(object + 0x2ccu, kHelperVtable);
    runtime.WriteWord(object + 0x2ccu, kHelperBaseVtable);
    runtime.DestructMachine(object);
}

std::uint32_t GobLocalFloat::CleanupAndReset(
    const std::uint32_t object, const std::uint32_t context,
    IGobLocalFloatCleanupRuntime& runtime) {
    runtime.CleanupMachineRuntime(object, context);
    const auto buffer = runtime.ReadWord(object + 0x2dcu);
    runtime.WriteWord(object + 0x2d0u, 0u);
    std::uint32_t result = 0u;
    if (buffer != 0u) {
        result = runtime.FreePathBuffer(buffer);
        runtime.WriteWord(object + 0x2dcu, 0u);
    }
    runtime.WriteWord(object + 0x2e0u, 0u);
    return result;
}

std::uint32_t GobLocalFloat::HandleEvent(
    const std::uint32_t object, const std::uint32_t argument,
    IGobLocalFloatControlRuntime& runtime) {
    return runtime.ForwardHandleEvent(object, argument);
}

std::uint32_t GobLocalFloat::UpdateMachineAndActor(
    const std::uint32_t object, const float elapsed,
    IGobLocalFloatFrameRuntime& runtime) {
    using Vector3 = IGobLocalFloatRuntime::Vector3;
    const Vector3 before{
        runtime.ReadFloat(object + 0xacu),
        runtime.ReadFloat(object + 0xb0u),
        runtime.ReadFloat(object + 0xb4u)};
    const auto previous_angle = runtime.ReadFloat(object + 0xa4u);
    runtime.UpdateMachineTasks(object, elapsed);
    const Vector3 pivot{
        runtime.ReadFloat(object + 0xacu),
        runtime.ReadFloat(object + 0xb0u),
        runtime.ReadFloat(object + 0xb4u)};
    const Vector3 delta{pivot.x - before.x, pivot.y - before.y,
                        pivot.z - before.z};
    const auto rotation = runtime.ReadFloat(object + 0xa4u) -
                          previous_angle;
    runtime.StorePlatformDelta(delta, rotation);
    const auto moved = [](const float value) {
        return !std::isnan(value) && std::fabs(value) != 0.0f;
    };
    const std::uint32_t result =
        moved(delta.x) || moved(delta.y) || moved(delta.z) ? 1u : 0u;
    if (runtime.ReadWord(object + 0x2d0u) == 0u || result == 0u)
        return result;

    const auto access = runtime.ActorControlAccess();
    const auto actor = runtime.ReadWord(access + 4u);
    if (actor == 0u) return access;
    runtime.ApplyActorTranslation(actor, delta, 1u);

    Vector3 actor_transform{
        runtime.ReadFloat(actor + 0xa0u),
        runtime.ReadFloat(actor + 0xa4u),
        runtime.ReadFloat(actor + 0xa8u)};
    const auto frame = runtime.ReadWord(
        runtime.ReadWord(actor + 0x14u) + 4u);
    const auto matrix = frame + 0x10u;
    runtime.MatrixTranslate(matrix,
                            {-pivot.x, -pivot.y, -pivot.z}, 2u);
    runtime.MatrixRotateY(matrix, rotation, 2u);
    runtime.MatrixTranslate(matrix, pivot, 2u);
    runtime.FrameUpdateObjects(frame);

    const Vector3 frame_position{
        runtime.ReadFloat(frame + 0x40u),
        runtime.ReadFloat(frame + 0x44u),
        runtime.ReadFloat(frame + 0x48u)};
    float axis_y = 1.0f;
    float angle = 0.0f;
    runtime.MatrixQueryRotation(matrix, axis_y, angle);
    if (axis_y < 0.0f) {
        actor_transform.y = -(actor_transform.y + angle);
    } else if (axis_y > 0.0f) {
        actor_transform.y = -(actor_transform.y - angle);
    }
    runtime.SetActorPosition(actor, frame_position);
    runtime.SetActorTransform(actor, actor_transform, 1u);
    runtime.UpdateMinimapGrid(actor + 0xacu, actor + 0xa0u);
    return runtime.UpdateFloorAttributes();
}

void GobLocalFloat::RenderDebugPath(
    const std::uint32_t object, IGobLocalFloatRuntime& runtime) {
    using Vector3 = IGobLocalFloatRuntime::Vector3;
    const auto points = runtime.ReadWord(object + 0x2dcu);
    const auto count = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x2e0u));
    if (points == 0u || count <= 0) return;
    runtime.SetRenderState(6u, 1u);
    runtime.SetRenderState(7u, 1u);
    runtime.SetRenderState(1u, 0u);
    for (std::int32_t i = 0; i < count - 1; ++i) {
        const auto first = points + static_cast<std::uint32_t>(i) * 12u;
        const Vector3 start{
            runtime.ReadFloat(first),
            runtime.ReadFloat(first + 4u),
            runtime.ReadFloat(first + 8u)};
        std::array<IGobLocalFloatRuntime::DebugVertex, 21> ring{};
        for (std::uint32_t sample = 0u; sample < ring.size(); ++sample) {
            const auto angle = static_cast<float>(
                static_cast<double>(sample) * 0.1 * std::numbers::pi);
            float x = 0.0f;
            float z = 0.0f;
            runtime.SampleCircle(angle, x, z);
            ring[sample] = {{start.x + 4.0f * x, start.y,
                             start.z + 4.0f * z}, 0xefdcdc11u};
        }
        if (runtime.TransformDebugVertices(ring, 2u)) {
            runtime.RenderDebugPrimitive(2u);
            runtime.EndDebugPrimitive();
        }
        const std::array<IGobLocalFloatRuntime::DebugVertex, 2> segment{{
            {start, 0xef11dc18u},
            {{runtime.ReadFloat(first + 12u),
              runtime.ReadFloat(first + 16u),
              runtime.ReadFloat(first + 20u)}, 0xef0010dcu}}};
        if (runtime.TransformDebugVertices(segment, 0u)) {
            runtime.RenderDebugPrimitive(2u);
            runtime.EndDebugPrimitive();
        }
    }
    runtime.SetRenderState(6u, 1u);
    runtime.SetRenderState(7u, 2u);
}

std::uint32_t GobLocalFloat::RebuildMachineTaskPathPoints(
    const std::uint32_t object, IGobLocalFloatConfigRuntime& runtime) {
    const auto old_points = runtime.ReadWord(object + 0x2dcu);
    if (old_points != 0u) {
        (void)runtime.FreePathBuffer(old_points);
        runtime.WriteWord(object + 0x2dcu, 0u);
    }
    runtime.WriteWord(object + 0x2e0u, 0u);
    const auto document = runtime.GetMachineConfigNode(object);
    if (document == 0u) return 0u;
    const auto tasks = runtime.FindXmlChild(
        document, "PAL4_GameObject-machine-tasks");
    if (tasks == 0u) return 0u;
    const auto children = runtime.XmlChildCount(tasks);
    if (children <= 0) return 0u;

    std::uint32_t count = 0u;
    for (std::int32_t i = 0; i < children; ++i) {
        const auto child = runtime.XmlChildAt(tasks, i);
        if (child == 0u) return 0u;
        const auto type = runtime.ReadXmlInt(
            child, "PAL4_GOMTask_type", -1);
        if (type == 0 || type == 10 || type == 16) ++count;
    }
    runtime.WriteWord(object + 0x2e0u, count);
    if (count == 0u) return 0u;
    const auto points = runtime.Allocate(count * 12u);
    runtime.WriteWord(object + 0x2dcu, points);
    if (points == 0u) return 0u;

    std::uint32_t next = 0u;
    for (std::int32_t i = 0; i < children; ++i) {
        const auto child = runtime.XmlChildAt(tasks, i);
        if (child == 0u) return 0u;
        const auto type = runtime.ReadXmlInt(
            child, "PAL4_GOMTask_type", -1);
        std::array<std::string_view, 3> keys{};
        if (type == 16) {
            keys = {"PAL4_GOMTask_start_posX",
                    "PAL4_GOMTask_start_posY",
                    "PAL4_GOMTask_start_posZ"};
        } else if (type == 10) {
            keys = {"PAL4_GOMTask_movment_tPosX",
                    "PAL4_GOMTask_movment_tPosY",
                    "PAL4_GOMTask_movment_tPosZ"};
        } else if (type == 0) {
            keys = {"PAL4_GOMTask_move_target_X",
                    "PAL4_GOMTask_move_target_Y",
                    "PAL4_GOMTask_move_target_Z"};
        } else {
            continue;
        }
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            runtime.WriteFloat(points + next * 12u + axis * 4u,
                               runtime.ReadXmlFloat(child, keys[axis], 0.0f));
        ++next;
    }
    return static_cast<std::uint32_t>(children);
}

std::uint32_t GobLocalFloat::InitializeSceneAndConfig(
    const std::uint32_t object, const std::uint32_t first,
    const std::string_view path, const std::uint32_t fourth,
    const std::uint32_t fifth, IGobLocalFloatRuntime& runtime) {
    runtime.CleanupBeforeSceneLoad(object);
    SelectGobScenePackage(path, runtime);
    const auto initialized = runtime.InitializeMachineAndConfig(
        object, first, path, fourth, fifth);
    runtime.CleanupScriptPackages();
    if (initialized == 0u) return 0u;
    const auto document = runtime.ReadWord(object + 0x1c0u);
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    if (parameters != 0u) {
        const auto old = runtime.FindXmlChild(
            parameters, "PAL4_GAMEOBJECT_float");
        if (old != 0u) runtime.RemoveXmlChild(parameters, old);
        (void)runtime.CreateXmlChild(
            parameters, "PAL4_GAMEOBJECT_float");
    }
    runtime.WriteWord(object + 0x2d0u, 0u);
    return initialized;
}

std::uint32_t GobLocalFloat::AttachMovementAdapter(
    const std::uint32_t object, IGobLocalFloatMovementRuntime& runtime) {
    const auto clump = runtime.ReadWord(object + 0x14u);
    const auto root = runtime.MovementAdapterRoot();
    const auto clump_manager = runtime.ReadWord(root);
    const auto property = runtime.MovementProperty(object);
    runtime.SetClumpProperties(
        clump_manager, clump, runtime.ReadWord(object + 0x188u),
        1u, 0u, property);
    const auto adapter = runtime.ReadWord(runtime.MovementAdapterRoot() + 4u);
    return runtime.AttachMovementHelper(adapter, object + 0x2ccu);
}

std::uint32_t GobLocalFloat::DetachMovementAdapter(
    const std::uint32_t object, IGobLocalFloatMovementRuntime& runtime) {
    const auto clump = runtime.ReadWord(object + 0x14u);
    const auto root = runtime.MovementAdapterRoot();
    runtime.SetClumpRenderCallback(runtime.ReadWord(root), clump);
    const auto adapter = runtime.ReadWord(runtime.MovementAdapterRoot() + 4u);
    return runtime.DetachMovementHelper(adapter, object + 0x2ccu);
}

std::uint32_t GobLocalFloat::TryTriggerByTouchTypeOrUseCount(
    const std::uint32_t object, const std::uint32_t argument,
    IGobLocalFloatTriggerRuntime& runtime) {
    if (runtime.ReadWord(object + 0x2d8u) != 0u ||
        runtime.ReadWord(object + 0x198u) != 0u)
        return runtime.TryTriggerMachineTask(object, argument);
    return 0u;
}

std::uint32_t GobLocalFloat::ResetMachineAndTouchLatch(
    const std::uint32_t object, IGobLocalFloatControlRuntime& runtime) {
    const auto result = runtime.ResetTriggeredMachine(object);
    runtime.WriteWord(object + 0x2d0u, 0u);
    return result;
}

std::uint8_t GobLocalFloat::StopTaskSounds(
    const std::uint32_t object, IGobLocalFloatControlRuntime& runtime) {
    return runtime.StopTaskSounds(object);
}

std::uint8_t GobLocalFloat::ResumeTaskSounds(
    const std::uint32_t object, IGobLocalFloatControlRuntime& runtime) {
    return runtime.ResumeTaskSounds(object);
}

bool GobLocalFloat::AreTaskContainersReady(
    const std::uint32_t object, IGobLocalFloatControlRuntime& runtime) {
    return runtime.AreTaskContainersReady(object);
}

std::uint32_t GobLocalFloat::GetConfigNode(
    const std::uint32_t object, IGobLocalFloatConfigRuntime& runtime) {
    const auto document = runtime.ReadWord(object + 0x1c0u);
    if (document == 0u) return 0u;
    const auto parameters = runtime.FindXmlChild(
        document, "PAL4-GameObject-Parameters");
    return parameters == 0u ? 0u :
        runtime.FindXmlChild(parameters, "PAL4_GAMEOBJECT_float");
}

std::uint32_t GobLocalFloat::ReadConfigAndMachineFlags(
    const std::uint32_t object, IGobLocalFloatConfigRuntime& runtime) {
    runtime.LoadMachineMainConfig(object);
    const auto node = GetConfigNode(object, runtime);
    if (node == 0u) return 0u;
    runtime.WriteWord(object + 0x190u, 0u);
    const auto auto_touch = runtime.ReadXmlInt(
        node, "PAL4_GAMEOBJECT_float_auto_touch", 0);
    runtime.WriteWord(object + 0x2d4u,
                      static_cast<std::uint32_t>(auto_touch));
    const auto touch_type = runtime.ReadXmlInt(
        node, "PAL4_GAMEOBJECT_float_touch_type", 1);
    runtime.WriteWord(object + 0x2d8u,
                      static_cast<std::uint32_t>(touch_type));
    if (touch_type == 1)
        runtime.WriteWord(object + 0x198u, 0xffffffffu);
    return static_cast<std::uint32_t>(touch_type);
}

std::uint32_t GobLocalFloat::ApplyVisibleAndAutoTouch(
    const std::uint32_t object, const std::uint32_t visible,
    IGobLocalFloatVisibilityRuntime& runtime) {
    runtime.ApplyMachineVisibleFlag(object, visible);
    const auto auto_touch = runtime.ReadWord(object + 0x2d4u);
    if (auto_touch != 0u && visible != 0u)
        return runtime.InvokeMachineTrigger(object, {0.0f, 0.0f, 0.0f});
    return auto_touch;
}

std::uint32_t GobLocalFloat::HelperClump(
    const std::uint32_t object, IGobLocalFloatMemoryRuntime& runtime) {
    return runtime.ReadWord(object + 0x14u);
}

std::uint32_t GobLocalFloat::ProcessHelperTouchState(
    const std::uint32_t object, IGobLocalFloatTouchRuntime& runtime) {
    const auto latched = runtime.ReadWord(object + 0x2d0u);
    if (latched != 0u) return latched;
    const auto touch_type = runtime.ReadWord(object + 0x2d8u);
    runtime.WriteWord(object + 0x2d0u, 1u);
    if (touch_type != 0u) return touch_type;
    if (runtime.ReadWord(object + 0x27cu) == 0u)
        return runtime.InvokeMachineTrigger(
            object, {0.0f, 0.0f, 0.0f});
    const auto pending = runtime.ReadWord(object + 0x278u);
    if (pending != 0u)
        runtime.WriteWord(object + 0x278u, 0u);
    return pending;
}

std::uint32_t GobLocalFloat::ResetHelperTouchState(
    const std::uint32_t object, IGobLocalFloatMemoryRuntime& runtime) {
    const auto latched = runtime.ReadWord(object + 0x2d0u);
    if (latched != 0u)
        runtime.WriteWord(object + 0x2d0u, 0u);
    return latched;
}

std::uint32_t GobLocalFloat::CreateStateSnapshot(
    const std::uint32_t object, IGobLocalFloatStateRuntime& runtime) {
    if (runtime.ReadWord(object + 0x1a4u) == 0u) return 0u;
    const auto snapshot = runtime.Allocate(0x34u);
    if (snapshot == 0u) return 0u;
    try {
        runtime.ConstructSnapshot(snapshot);
        runtime.AssignSnapshotName(
            snapshot, runtime.ReadObjectName(
                runtime.ReadWord(object + 4u)));
        runtime.WriteWord(snapshot + 0x14u,
                          runtime.ReadWord(object + 0x184u));
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            runtime.WriteFloat(snapshot + 0x18u + axis * 4u,
                               runtime.ReadFloat(object + 0xacu + axis * 4u));
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            runtime.WriteFloat(snapshot + 0x24u + axis * 4u,
                               runtime.ReadFloat(object + 0xa0u + axis * 4u));
        runtime.WriteWord(snapshot + 0x30u,
                          runtime.ReadWord(object + 0x198u));
    } catch (...) {
        runtime.Free(snapshot);
        throw;
    }
    return snapshot;
}

std::uint32_t GobLocalFloat::DeleteHelper(
    const std::uint32_t helper, const std::uint32_t flags,
    IGobLocalFloatMemoryRuntime& runtime) {
    runtime.WriteWord(helper, kHelperBaseVtable);
    if ((flags & 1u) != 0u) runtime.Free(helper);
    return helper;
}

std::uint32_t GobLocalFloat::ApplyStateSnapshot(
    const std::uint32_t object, const std::uint32_t snapshot,
    IGobLocalFloatStateRuntime& runtime) {
    const auto stored = runtime.ReadWord(object + 0x1a4u);
    if (stored == 0u) return stored;
    const auto type = runtime.SnapshotType(snapshot);
    if (type != 2u) return type;
    runtime.SetBaseVisible(object,
                           runtime.ReadWord(snapshot + 0x14u));
    const Vector3 position{
        runtime.ReadFloat(snapshot + 0x18u),
        runtime.ReadFloat(snapshot + 0x1cu),
        runtime.ReadFloat(snapshot + 0x20u)};
    runtime.SetBasePosition(object, position, 0u);
    const Vector3 rotation{
        runtime.ReadFloat(snapshot + 0x24u),
        runtime.ReadFloat(snapshot + 0x28u),
        runtime.ReadFloat(snapshot + 0x2cu)};
    const auto result = runtime.SetBaseRotation(object, rotation, 0u);
    runtime.WriteWord(object + 0x198u,
                      runtime.ReadWord(snapshot + 0x30u));
    return result;
}

}  // namespace pal4
