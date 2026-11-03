#pragma once
#include "pal4/script_entity_transform.h"

namespace pal4 {
class IScriptPlayerPositionRuntime;
class IScriptPlayerRotationRuntime;
class IGameResourceManagerRuntime;

// Immediate base virtual+4/+8 transform, without the separate player +60
// camera/audio operation or queued position/rotation script handlers.
class NativeScriptEntityTransformRuntime final : public IScriptEntityTransformRuntime {
public:
    NativeScriptEntityTransformRuntime(IScriptPlayerPositionRuntime& positions,
        IScriptPlayerRotationRuntime& rotations, IGameResourceManagerRuntime& transforms);
    std::uint32_t PlayerManager() override;
    std::uint32_t FindPlayer(std::uint32_t, std::int32_t) override;
    std::uint32_t NpcManager() override;
    std::uint32_t FindNpc(std::uint32_t, std::string_view) override;
    float ReadFloat(std::uint32_t) override;
    void SetPosition(std::uint32_t, const std::array<float,3>&, std::uint32_t) override;
    std::uint32_t SetRotation(std::uint32_t, const std::array<float,3>&, std::uint32_t) override;
private:
    void RequireSelected(std::uint32_t, std::uint32_t) const;
    IScriptPlayerPositionRuntime& positions_;
    IScriptPlayerRotationRuntime& rotations_;
    IGameResourceManagerRuntime& transforms_;
    std::uint32_t selected_{};
};
}
