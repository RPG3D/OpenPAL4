#include "pal4/game_resource_manager.h"
#include "pal4/resource_rotation_angle.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

float ReadFloat(const GameResourceManager& manager, std::size_t index) {
    return std::bit_cast<float>(manager.words[index]);
}

void WriteVec(GameResourceManager& manager, std::size_t offset,
              const ResourceVec3& input, bool relative) {
    const float values[3] = {input.x, input.y, input.z};
    for (std::size_t i = 0; i < 3; ++i) {
        manager.words[offset + i] = std::bit_cast<std::uint32_t>(
            relative ? ReadFloat(manager, offset + i) + values[i] : values[i]);
    }
}


std::uint32_t RebuildFrame(GameResourceManager& manager,
                           IGameResourcePoseRuntime& runtime,
                           bool update_main) {
    const auto frame = runtime.ClumpFrame(manager.words[5]);
    runtime.FrameSetIdentity(frame);
    const float scale[3] = {ReadFloat(manager, 46), ReadFloat(manager, 47),
                            ReadFloat(manager, 48)};
    runtime.FrameScale(frame, scale, 0);
    runtime.FrameRotate(frame, 0x84b870u, ReadFloat(manager, 40), 2);
    runtime.FrameRotate(frame, 0x84b87cu, ReadFloat(manager, 41), 2);
    runtime.FrameRotate(frame, 0x84b888u, ReadFloat(manager, 42), 2);
    const float position[3] = {ReadFloat(manager, 43), ReadFloat(manager, 44),
                               ReadFloat(manager, 45)};
    runtime.FrameTranslate(frame, position, 2);
    if (update_main) runtime.FrameUpdateObjects(frame);
    return frame;
}

void UpdateSecondary(GameResourceManager& manager,
                     IGameResourcePoseRuntime& runtime,
                     std::uint32_t frame) {
    if (manager.words[12] == 0) return;
    const auto secondary = runtime.ClumpFrame(manager.words[12]);
    runtime.ApplyDialogTransform(secondary, runtime.ObjectTransform(frame), 0);
    runtime.FrameUpdateObjects(secondary);
}

void UpdateCorners(GameResourceManager& manager,
                   IGameResourcePoseRuntime& runtime,
                   std::uint32_t frame) {
    if (manager.words[8] == 0) return;
    ResourceCorners corners;
    for (std::size_t i = 0; i < corners.size(); ++i) {
        corners[i] = {
            ReadFloat(manager, (i & 1) ? 60 : 63),
            ReadFloat(manager, (i & 2) ? 61 : 64),
            ReadFloat(manager, (i & 4) ? 62 : 65)};
    }
    runtime.TransformPoints(corners, runtime.ObjectTransform(frame));
    runtime.CalculateBBox(manager.words.data() + 66, corners);
}

std::uint32_t RefreshBounds(GameResourceManager& manager,
                            IGameResourcePoseRuntime& runtime) {
    if (manager.words[8] != 0) {
        for (std::size_t i = 66; i <= 68; ++i) manager.words[i] = 0xff7fffffu;
        for (std::size_t i = 69; i <= 71; ++i) manager.words[i] = 0x7f7fffffu;
        runtime.ProcessAtomicBounds(manager.words[5], 1, manager.words.data() + 66);
    }
    for (std::size_t i = 60; i <= 62; ++i) manager.words[i] = 0xff7fffffu;
    for (std::size_t i = 63; i <= 65; ++i) manager.words[i] = 0x7f7fffffu;
    runtime.ProcessAtomicBounds(manager.words[5], 0, manager.words.data() + 60);
    const auto result = runtime.ProcessAtomicBounds2(
        manager.words[5], 0, 1, manager.words.data() + 52);
    manager.words[59] = manager.words[55];
    return result;
}

void RefreshScaledFrame(GameResourceManager& manager,
                        IGameResourcePoseRuntime& runtime) {
    const auto frame = RebuildFrame(manager, runtime, false);
    UpdateSecondary(manager, runtime, frame);
    (void)RefreshBounds(manager, runtime);
}

}  // namespace

std::uint32_t GameResourceManager::CalculateBoundingBox(
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0) return 0;
    for (std::size_t i = 60; i <= 62; ++i) words[i] = 0xff7fffffu;
    for (std::size_t i = 63; i <= 65; ++i) words[i] = 0x7f7fffffu;
    runtime.ProcessAtomicBounds(words[5], 0, words.data() + 60);

    for (std::size_t i = 66; i <= 68; ++i) words[i] = 0xff7fffffu;
    for (std::size_t i = 69; i <= 71; ++i) words[i] = 0x7f7fffffu;
    runtime.ProcessAtomicBounds(words[5], 1, words.data() + 66);
    (void)runtime.ProcessAtomicBounds2(words[5], 1, 1,
                                       words.data() + 56);
    return runtime.ProcessAtomicBounds2(words[5], 0, 1,
                                        words.data() + 52);
}

std::uint32_t GameResourceManager::UpdateClumpTransform(
    const ResourceVec3& position, const bool relative,
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0) return 0;
    WriteVec(*this, 43, position, relative);
    const auto frame = RebuildFrame(*this, runtime, false);
    UpdateSecondary(*this, runtime, frame);
    UpdateCorners(*this, runtime, frame);
    return runtime.CalculateCameraPosition(words.data() + 56,
                                           words.data() + 52, frame);
}

void GameResourceManager::UpdateClumpRotation(
    const ResourceVec3& rotation, const bool relative,
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0) return;
    WriteVec(*this, 40, rotation, relative);
    for (std::size_t i = 40; i <= 42; ++i) {
        words[i] = std::bit_cast<std::uint32_t>(NormalizeResourceRotationAngle(ReadFloat(*this, i)));
    }
    const auto frame = RebuildFrame(*this, runtime, true);
    UpdateSecondary(*this, runtime, frame);
    UpdateCorners(*this, runtime, frame);
}

void GameResourceManager::UpdateClumpPosition(
    const ResourceVec3& scale, const bool relative,
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0) return;
    WriteVec(*this, 46, scale, relative);
    if (!(ReadFloat(*this, 46) > 0.0f || ReadFloat(*this, 47) > 0.0f ||
          ReadFloat(*this, 48) > 0.0f)) return;
    RefreshScaledFrame(*this, runtime);
}

std::uint32_t GameResourceManager::UpdateClumpScale(
    const float uniform, const bool relative,
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0 || !(uniform > 0.0f)) return 0;
    WriteVec(*this, 46, {uniform, uniform, uniform}, relative);
    RefreshScaledFrame(*this, runtime);
    return words[55];
}

std::uint32_t GameResourceManager::ResetClumpTransform(
    IGameResourcePoseRuntime& runtime) {
    if (words[5] == 0) return 0;
    words[46] = words[47] = words[48] = 0x3f800000u;
    const auto frame = runtime.ClumpFrame(words[5]);
    runtime.FrameSetIdentity(frame);
    runtime.FrameRotate(frame, 0x84b870u, ReadFloat(*this, 40), 0);
    runtime.FrameRotate(frame, 0x84b87cu, ReadFloat(*this, 41), 2);
    runtime.FrameRotate(frame, 0x84b888u, ReadFloat(*this, 42), 2);
    const float scale[3] = {1.0f, 1.0f, 1.0f};
    runtime.FrameScale(frame, scale, 2);
    const float position[3] = {ReadFloat(*this, 43), ReadFloat(*this, 44),
                               ReadFloat(*this, 45)};
    runtime.FrameTranslate(frame, position, 2);
    UpdateSecondary(*this, runtime, frame);
    return RefreshBounds(*this, runtime);
}

std::uint32_t GameResourceManager::SetClumpMaterialFlagsForAllAtomics(
    const std::uint32_t mode, IGameResourceManagerRuntime& runtime) {
    if (words[5] == 0) return 0;
    return runtime.SetClumpMaterialFlagsForAllAtomics(words[5], mode);
}

std::uint32_t GameResourceManager::SetCurrentClump(const std::uint32_t clump) {
    words[72] = clump;
    return clump;
}

}  // namespace pal4
