#pragma once
#include "pal4/game_world_frame_processor.h"
#include "pal4/native_base_object_scene_frame.h"
#include "pal4/native_player_scene_frame.h"
#include <vector>
namespace pal4 {
class NativeArenaSceneResourceStage;class NativeWorldAsset;class NativeGameArenaManagerOwner;class NativeSceneGobAssets;
class NativePlayerManagerSession;class NativeArenaNpcSceneLoader;class NativeMonsterManagerOwner;
class NativePetManagerOwner;class NativePetSceneSession;class NativePalObjectManagerOwner;
class NativeEffectSceneSession;class NativePlayerTransformRuntime;class IAnimationEventQueueActivityRuntime;
struct NativeGameWorldSceneFrameResult {
    bool world_rendered{};
    std::uint32_t gob_objects{},pal_objects{},skill_actors{},scene_passes{},player_camera_passes{},npc_camera_passes{},player_queues{};
    NativePlayerSceneFrameResult players{};
    NativeBaseObjectSceneFrameResult npcs{},monsters{},pet{};
    std::vector<GameWorldFrameOperation> operations;
};
// Full original519C50 composition. All creating owners, the active begun camera
// and the actual animation activity backend must outlive synchronous dispatch.
class NativeGameWorldSceneFrame final : public IGameWorldFrameRuntime {
public:
    NativeGameWorldSceneFrame(NativeWorldAsset&,NativeArenaSceneResourceStage&,NativeGameArenaManagerOwner&,NativeSceneGobAssets&,
        NativePlayerManagerSession&,NativePlayerSceneFrame&,NativeArenaNpcSceneLoader&,NativeMonsterManagerOwner&,
        NativePetManagerOwner&,NativePetSceneSession&,NativePalObjectManagerOwner&,NativeEffectSceneSession&,
        NativePlayerTransformRuntime&,IAnimationEventQueueActivityRuntime&);
    void ProcessFrame(std::uint32_t world,std::uint32_t camera) override;
    const NativeGameWorldSceneFrameResult& LastResult() const noexcept {return result_;}
private:
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    std::uint32_t Call(GameWorldFrameOperation,std::uint32_t,std::uint32_t) override;
    void DrainGeometry();
    NativeArenaSceneResourceStage& stage_;
    NativeWorldAsset& world_;NativeGameArenaManagerOwner& arena_;NativeSceneGobAssets& gob_;
    NativePlayerManagerSession& players_;NativePlayerSceneFrame& player_frame_;NativeArenaNpcSceneLoader& npcs_;
    NativeMonsterManagerOwner& monsters_;NativePetManagerOwner& pet_;NativePetSceneSession& pet_scene_;
    NativePalObjectManagerOwner& objects_;NativeEffectSceneSession& effects_;NativePlayerTransformRuntime& transforms_;
    IAnimationEventQueueActivityRuntime& activities_;
    NativeGameWorldSceneFrameResult result_{};
    std::uint32_t camera_{};bool fog_applied_{},geometry_pending_{};
};
}
