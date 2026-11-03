#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct ScriptBuiltinDescriptor {
    std::uint32_t description{};
    std::uint32_t description_size{};
    std::uint32_t signature{};
    std::uint32_t signature_size{};
    std::array<std::uint32_t, 8> callback_data{};
};
static_assert(sizeof(ScriptBuiltinDescriptor) == 48u);

enum class ScriptBuiltinExtension : std::uint8_t {
    CameraSetMode, GoodsGetOpenCondition, GoodsSetOpenCondition,
    NpcPauseBehavior, NpcResumeBehavior, OpenWeather, CloseWeather,
    MinimapExpMode, GetRandomNumber, SetTempGameState,
    FlushTailYAngle, AddCombatMonster, ConfigCombatParam,
    ConfigCombatBgm, ConfigCombatCamera, StartCombat,
    SetObjectVisible, AddProperty, DelProperty, PlayerInTeam,
    PlayerOutTeam, GomTouch, CameraSetCollide, CameraSeekToPlayer,
    CameraAutoSeek, PlayerSetAttr, PlayerGetLeader, SetPlayerLevel,
    AddPlayerEquip, OpenMovieFlag, AddQuestComplatePercentage,
    AddEquipment, PlayerCurrentSetVisible, SetFullHP, SetFullMP,
    GetPlayerLevel, GotoLogo, PlayerUnHoldAct, NpcUnHoldAct,
    CameraSetDistOptEnable,
    MonsterSetHide, GameObjectSetResearch, SetPortrait,
    BGMConfigSetMusic, BGMConfigIsInArea, ScriptMusicMute,
    ScriptMusicPlay, ScriptMusicStop, ArenaMusicStop,
    WorldMapSetState, GetPuzzleGameResult, AlwaysJump,
    Sound2DPlay, Sound2DStop, Sound2DStopID,
    CGEffPlay, CGEffStop, EffectPlay, EffectPlayWithPlayer,
    EffectPlayWithCurrentPlayer,
    EffectPlayWithNPC, EffectPlayWithOBJ, EffectStopWithOBJ,
    ShowHint, PlayerAddSkill, MonsterSetVisible,
    PlayerRandomPosition, PlayerCurrentRandomPosition, EventVolumeVisible,
    AllPlayerGarb2, PlayerGarb2, AllPlayerGarb1,
    PlayerGarb1, GetVisibleObject, GetVisibleMonster,
    CheckPackProperty, GrantSystemUi, OpenSystemUi,
    GrantSmithSystem, GrantMagicSystem, CheckMagicMastered,
    SelectDialogAddItem, SelectDialogGetLastSelect, GOBAttachToPlayer,
    GOBAttachToCurrentPlayer,
    GOBDetachFromPlayer, GOBDetachFromCurrentPlayer, EffectAttachToPlayer,
    EffectAttachToCurrentPlayer, EffectDetachFromPlayer, EffectDetachFromCurrentPlayer,
    EffectAttachToNpc, EffectDetachFromNpc, GOBAttachToNpc,
    GOBDetachFromNPC, GOBSetPosition, ScriptClearCTXButCurrent,
    AddMoney, PayMoney, HideGASkillObject,
    ShowSignpost, PlayerSetEmotion, PlayerResetEmotion,
    PlayerCurrentSetEmotion, PlayerCurrentResetEmotion, LINGSHALegsInjured,
    LINGSHALegsHealing, NpcSetEmotion, NpcResetEmotion,
    GetPropertyNumb,
    PlayerCurrentGetPosX, PlayerCurrentGetPosY, PlayerCurrentGetPosZ,
    ArenaGetName, ArenaGetArea, ArenaSkillEnable,
    ShowInnDialog, GetInnDialogResult, PlayerTakeARest,
    IsNightTime, PlayerSetPosRotById, PlayerSetPosRotByName,
    StartUiTimer, PlayerForbidenSkill, GetMoney,
    SelectDialogSetDefaultSelect, ShowQuestDialog, GetQuestDialogResult,
    ResetPlayerToJumpStart, GOBReset, CheckEquipInInventory,
    RemoveEquipment, AddPrescription, EnableShadow,
    AddRoundTimes, TimeScript, TimeScriptTerminate,
    ConfigCombatGroundCamera, AddPlayerFavor, GetPalTestResult,
    SetMinimapLevel, PetShow, NpcAttachEffect,
    NpcDetachEffect, MstAttachEffect, MstDetachEffect,
    PlayerHookEffect, PlayerDetachEffect, CommonDialogGetLastSelect,
    ConfigCombatVipMonster,
    EnableSTS, ClearUiTimer, PauseUiTimer,
    ResumeUiTimer, SetFullRage, UiTimerGetSaveData,
    ScriptMusicPause, ScriptMusicResume
};

// The static initializers at 0x5b7430 use a second descriptor for each builtin.
enum class ScriptBuiltinStaticCommand : std::uint8_t {
    ImmBegin, ImmEnd, NewGame, CameraYpr, CameraDistance,
    CameraYprd, CameraGetDistance, CameraGetYaw,
    CameraGetPitch, CameraGetRoll, ArenaLoad, ArenaReady,
    ArenaReadyRestore, ArenaHint, ArenaComeFromHere,
    PlayerSetLeader, PlayerSetVisible, PlayerAttachCollision,
    PlayerDetachCollision, PlayerLock, PlayerUnlock,
    NpcSetVisible, NpcCreate, NpcDelete, SystemExchange,
    MonsterStopPursuit
};

class IScriptBuiltinRuntime {
public:
    virtual ~IScriptBuiltinRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size,
                                                  std::uint32_t allocation_type,
                                                  std::uint32_t protection) = 0;
    virtual void CopyString(std::uint32_t destination,
                            std::string_view source_with_nul) = 0;
    virtual void Release(std::uint32_t address, std::uint32_t size,
                         std::uint32_t free_type) = 0;
    virtual void Register(std::uint32_t descriptor_address,
                          const ScriptBuiltinDescriptor& descriptor) = 0;
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback_address) = 0;
};

class ScriptBuiltinRuntimeStub : public IScriptBuiltinRuntime {
public:
    std::uint32_t Allocate(std::uint32_t, std::uint32_t,
                           std::uint32_t) override;
    void CopyString(std::uint32_t, std::string_view) override;
    void Release(std::uint32_t, std::uint32_t, std::uint32_t) override;
    void Register(std::uint32_t,
                  const ScriptBuiltinDescriptor&) override;
    int RegisterAtExit(std::uint32_t) override;
};

class ScriptBuiltinRegistry final {
public:
    static constexpr std::array<std::uint32_t, 26> kStaticDescriptors{
        0x901370u, 0x901338u, 0x901300u, 0x9012c8u, 0x901290u,
        0x901258u, 0x901220u, 0x9011e8u, 0x9011b0u, 0x901178u,
        0x901140u, 0x901108u, 0x9010d0u, 0x901098u, 0x901060u,
        0x901028u, 0x900ff0u, 0x900fb8u, 0x900f80u, 0x900f48u,
        0x900f10u, 0x900ed8u, 0x900ea0u, 0x900e68u, 0x900e30u,
        0x900df8u};
    static constexpr std::array<std::uint32_t, 26> kStaticCleanupCallbacks{
        0x5b7460u, 0x5b74a0u, 0x5b74e0u, 0x5b7520u, 0x5b7560u,
        0x5b75a0u, 0x5b75e0u, 0x5b7620u, 0x5b7660u, 0x5b76a0u,
        0x5b76e0u, 0x5b7720u, 0x5b7760u, 0x5b77a0u, 0x5b77e0u,
        0x5b7820u, 0x5b7860u, 0x5b78a0u, 0x5b78e0u, 0x5b7920u,
        0x5b7960u, 0x5b79a0u, 0x5b79e0u, 0x5b7a20u, 0x5b7a60u,
        0x5b7aa0u};
    static constexpr std::uint32_t kImmBeginDescriptor = 0x8e77e0u;
    static constexpr std::uint32_t kImmEndDescriptor = 0x8e77a8u;
    static constexpr std::uint32_t kNewGameDescriptor = 0x8e7770u;
    static constexpr std::uint32_t kCameraYprDescriptor = 0x8e7738u;
    static constexpr std::uint32_t kCameraDistanceDescriptor = 0x8e7700u;
    static constexpr std::uint32_t kCameraYprdDescriptor = 0x8e76c8u;
    static constexpr std::uint32_t kCameraGetDistanceDescriptor = 0x8e7690u;
    static constexpr std::uint32_t kCameraGetYawDescriptor = 0x8e7658u;
    static constexpr std::uint32_t kCameraGetPitchDescriptor = 0x8e7620u;
    static constexpr std::uint32_t kCameraGetRollDescriptor = 0x8e75e8u;
    static constexpr std::uint32_t kArenaLoadDescriptor = 0x8e75b0u;
    static constexpr std::uint32_t kArenaReadyDescriptor = 0x8e7578u;
    static constexpr std::uint32_t kArenaReadyRestoreDescriptor = 0x8e7540u;
    static constexpr std::uint32_t kArenaHintDescriptor = 0x8e7508u;
    static constexpr std::uint32_t kArenaComeFromHereDescriptor = 0x8e74d0u;
    static constexpr std::uint32_t kPlayerSetLeaderDescriptor = 0x8e7498u;
    static constexpr std::uint32_t kPlayerSetVisibleDescriptor = 0x8e7460u;
    static constexpr std::uint32_t kPlayerAttachCollisionDescriptor = 0x8e7428u;
    static constexpr std::uint32_t kPlayerDetachCollisionDescriptor = 0x8e73f0u;
    static constexpr std::uint32_t kPlayerLockDescriptor = 0x8e73b8u;
    static constexpr std::uint32_t kPlayerUnlockDescriptor = 0x8e7380u;
    static constexpr std::uint32_t kNpcSetVisibleDescriptor = 0x8e7348u;
    static constexpr std::uint32_t kNpcCreateDescriptor = 0x8e7310u;
    static constexpr std::uint32_t kNpcDeleteDescriptor = 0x8e72d8u;
    static constexpr std::uint32_t kSystemExchangeDescriptor = 0x8e72a0u;
    static constexpr std::uint32_t kMonsterStopPursuitDescriptor = 0x8e7268u;
    static constexpr std::uint32_t kImmBeginHandler = 0x5d2fb0u;
    static constexpr std::uint32_t kImmEndHandler = 0x5d2fc0u;
    static constexpr std::uint32_t kNewGameHandler = 0x5d2fd0u;
    static constexpr std::uint32_t kCameraYprHandler = 0x5d3000u;
    static constexpr std::uint32_t kCameraDistanceHandler = 0x5d3030u;
    static constexpr std::uint32_t kCameraYprdHandler = 0x5d3060u;
    static constexpr std::uint32_t kCameraGetDistanceHandler = 0x5d30a0u;
    static constexpr std::uint32_t kCameraGetYawHandler = 0x5d30b0u;
    static constexpr std::uint32_t kCameraGetPitchHandler = 0x5d30c0u;
    static constexpr std::uint32_t kCameraGetRollHandler = 0x5d30d0u;
    static constexpr std::uint32_t kArenaLoadHandler = 0x5d30e0u;
    static constexpr std::uint32_t kArenaReadyHandler = 0x5d3320u;
    static constexpr std::uint32_t kArenaReadyRestoreHandler = 0x5d38c0u;
    static constexpr std::uint32_t kArenaHintHandler = 0x5d38e0u;
    static constexpr std::uint32_t kArenaComeFromHereHandler = 0x5d3910u;
    static constexpr std::uint32_t kPlayerSetLeaderHandler = 0x5d3a00u;
    static constexpr std::uint32_t kPlayerSetVisibleHandler = 0x5d3a20u;
    static constexpr std::uint32_t kPlayerAttachCollisionHandler = 0x5d3a80u;
    static constexpr std::uint32_t kPlayerDetachCollisionHandler = 0x5d3ad0u;
    static constexpr std::uint32_t kPlayerLockHandler = 0x5d3b10u;
    static constexpr std::uint32_t kPlayerUnlockHandler = 0x5d3b70u;
    static constexpr std::uint32_t kNpcSetVisibleHandler = 0x5d3bc0u;
    static constexpr std::uint32_t kNpcCreateHandler = 0x5d3c20u;
    static constexpr std::uint32_t kNpcDeleteHandler = 0x5d3d20u;
    static constexpr std::uint32_t kSystemExchangeHandler = 0x5d3d80u;
    static constexpr std::uint32_t kMonsterStopPursuitHandler = 0x5d3e10u;
    static constexpr std::uint32_t kImmBeginCleanup = 0x45ae80u;
    static constexpr std::uint32_t kImmEndCleanup = 0x45afe0u;
    static constexpr std::uint32_t kNewGameCleanup = 0x45b140u;
    static constexpr std::uint32_t kCameraYprCleanup = 0x45b2a0u;
    static constexpr std::uint32_t kCameraDistanceCleanup = 0x45b400u;
    static constexpr std::uint32_t kCameraYprdCleanup = 0x45b560u;
    static constexpr std::uint32_t kCameraGetDistanceCleanup = 0x45b6c0u;
    static constexpr std::uint32_t kCameraGetYawCleanup = 0x45b820u;
    static constexpr std::uint32_t kCameraGetPitchCleanup = 0x45b980u;
    static constexpr std::uint32_t kCameraGetRollCleanup = 0x45bae0u;
    static constexpr std::uint32_t kArenaLoadCleanup = 0x45bc40u;
    static constexpr std::uint32_t kArenaReadyCleanup = 0x45bda0u;
    static constexpr std::uint32_t kArenaReadyRestoreCleanup = 0x45bf00u;
    static constexpr std::uint32_t kArenaHintCleanup = 0x45c060u;
    static constexpr std::uint32_t kArenaComeFromHereCleanup = 0x45c1c0u;
    static constexpr std::uint32_t kPlayerSetLeaderCleanup = 0x45c320u;
    static constexpr std::uint32_t kPlayerSetVisibleCleanup = 0x45c4d0u;
    static constexpr std::uint32_t kPlayerAttachCollisionCleanup = 0x45c630u;
    static constexpr std::uint32_t kPlayerDetachCollisionCleanup = 0x45c790u;
    static constexpr std::uint32_t kPlayerLockCleanup = 0x45c8f0u;
    static constexpr std::uint32_t kPlayerUnlockCleanup = 0x45ca50u;
    static constexpr std::uint32_t kNpcSetVisibleCleanup = 0x45cbb0u;
    static constexpr std::uint32_t kNpcCreateCleanup = 0x45cd10u;
    static constexpr std::uint32_t kNpcDeleteCleanup = 0x45ce70u;
    static constexpr std::uint32_t kSystemExchangeCleanup = 0x45cfd0u;
    static constexpr std::uint32_t kMonsterStopPursuitCleanup = 0x45d130u;
    static constexpr std::array<std::uint32_t, 158> kExtensionDescriptors{
        0x8e67b0u, 0x8e6778u, 0x8e6740u, 0x8e6708u, 0x8e66d0u,
        0x8e6698u, 0x8e6660u, 0x8e6628u, 0x8e65f0u, 0x8e65b8u,
        0x8e6580u, 0x8e6548u, 0x8e6510u, 0x8e64d8u, 0x8e64a0u,
        0x8e6468u, 0x8e6430u, 0x8e63f8u, 0x8e63c0u, 0x8e6388u,
        0x8e6350u, 0x8e6318u, 0x8e62e0u, 0x8e62a8u, 0x8e6270u,
        0x8e6238u, 0x8e6200u, 0x8e61c8u, 0x8e6190u, 0x8e6158u,
        0x8e6120u, 0x8e60e8u, 0x8e60b0u, 0x8e6078u, 0x8e6040u,
        0x8e6008u, 0x8e5fd0u, 0x8e5f98u, 0x8e5f60u, 0x8e5f28u,
        0x8e5ef0u, 0x8e5eb8u, 0x8e5e80u, 0x8e5e48u, 0x8e5e10u,
        0x8e5dd8u, 0x8e5da0u, 0x8e5d68u, 0x8e5d30u, 0x8e5cf8u,
        0x8e5cc0u, 0x8e5c88u, 0x8e5c50u, 0x8e5c18u, 0x8e5be0u,
        0x8e5ba8u, 0x8e5b70u, 0x8e5b38u, 0x8e5b00u, 0x8e5ac8u,
        0x8e5a90u, 0x8e5a58u, 0x8e5a20u, 0x8e59e8u, 0x8e59b0u,
        0x8e5978u, 0x8e5940u, 0x8e5908u, 0x8e58d0u, 0x8e5898u,
        0x8e5860u, 0x8e5828u, 0x8e57f0u, 0x8e57b8u, 0x8e5780u,
        0x8e5748u, 0x8e5710u, 0x8e56d8u, 0x8e56a0u, 0x8e5668u,
        0x8e5630u, 0x8e55f8u, 0x8e55c0u, 0x8e5588u, 0x8e5550u,
        0x8e5518u, 0x8e54e0u, 0x8e54a8u, 0x8e5470u, 0x8e5438u,
        0x8e5400u, 0x8e53c8u, 0x8e5390u, 0x8e5358u, 0x8e5320u,
        0x8e52e8u, 0x8e52b0u, 0x8e5278u, 0x8e5240u, 0x8e5208u,
        0x8e51d0u, 0x8e5198u, 0x8e5160u, 0x8e5128u, 0x8e50f0u,
        0x8e50b8u, 0x8e5080u, 0x8e5048u, 0x8e5010u, 0x8e4fd8u,
        0x8e4fa0u, 0x8e4f68u, 0x8e4f30u, 0x8e4ef8u, 0x8e4ec0u,
        0x8e4e88u, 0x8e4e50u, 0x8e4e18u, 0x8e4de0u, 0x8e4da8u,
        0x8e4d70u, 0x8e4d38u, 0x8e4d00u, 0x8e4cc8u, 0x8e4c90u,
        0x8e4c58u, 0x8e4c20u, 0x8e4be8u, 0x8e4bb0u, 0x8e4b78u,
        0x8e4b40u, 0x8e4b08u, 0x8e4ad0u, 0x8e4a98u, 0x8e4a60u,
        0x8e4a28u, 0x8e49f0u, 0x8e49b8u, 0x8e4980u, 0x8e4948u,
        0x8e4910u, 0x8e48d8u, 0x8e48a0u, 0x8e4868u, 0x8e4830u,
        0x8e47f8u, 0x8e47c0u, 0x8e4788u, 0x8e4750u, 0x8e4718u,
        0x8e46e0u, 0x8e46a8u, 0x8e4670u, 0x8e4638u, 0x8e4600u,
        0x8e45c8u, 0x8e4590u, 0x8e4558u};
    static constexpr std::array<std::uint32_t, 158> kExtensionHandlers{
        0x5d3e30u, 0x5d3ed0u, 0x5d3ee0u, 0x5d3f00u, 0x5d3f60u,
        0x5d3fb0u, 0x5bcf90u, 0x5d3fd0u, 0x5d4000u, 0x5d4020u,
        0x5d4050u, 0x5d4060u, 0x5d42c0u, 0x5d4480u, 0x5d4490u,
        0x5d44a0u, 0x5d4640u, 0x5d46f0u, 0x5d4820u, 0x5d4950u,
        0x5d4b60u, 0x5d4d50u, 0x5d4dd0u, 0x5d4e40u, 0x5d4ef0u,
        0x5d4f10u, 0x5d5120u, 0x5d5180u, 0x5d5200u, 0x5d5280u,
        0x5d52c0u, 0x5d52f0u, 0x5d5460u, 0x5d54c0u, 0x5d5510u,
        0x5d5560u, 0x5d5590u, 0x5d55b0u, 0x5d55e0u, 0x5d5620u,
        0x5d5640u, 0x5d5720u, 0x5d5760u, 0x5d57a0u, 0x5d57c0u,
        0x5d57f0u, 0x5d5810u, 0x5d5840u, 0x5d5860u, 0x5d5880u,
        0x5d5890u, 0x5d58a0u, 0x5d58c0u, 0x5d5a30u, 0x5d5a60u,
        0x5d5a80u, 0x5d5aa0u, 0x5d5ab0u, 0x5d5b10u, 0x5d5b70u,
        0x5d5bd0u, 0x5d5c30u, 0x5d5d20u, 0x5d5d80u, 0x5d5f00u,
        0x5d6060u, 0x5d61a0u, 0x5d6400u, 0x5d6670u, 0x5d66d0u,
        0x5d68e0u, 0x5bcfa0u, 0x5d6aa0u, 0x5d6c70u, 0x5d6cb0u,
        0x5d6cf0u, 0x5d6d20u, 0x5d6d50u, 0x5d6da0u, 0x5d6e10u,
        0x5d6e50u, 0x5d6e90u, 0x5d6eb0u, 0x5d6ed0u, 0x5d7010u,
        0x5d7160u, 0x5d7180u, 0x5d71b0u, 0x5d7200u, 0x5d7260u,
        0x5d7280u, 0x5d72b0u, 0x5d7310u, 0x5d7340u, 0x5d7490u,
        0x5d74c0u, 0x5d7510u, 0x5d7530u, 0x5d7620u, 0x5d7700u,
        0x5d7710u, 0x5d7720u, 0x5d7750u, 0x5d7770u, 0x5d77b0u,
        0x5d77e0u, 0x5d7810u, 0x5d7830u, 0x5d7870u, 0x5d78a0u,
        0x5d78c0u, 0x5d78f0u, 0x5d7920u, 0x5d7950u, 0x5d7a20u,
        0x5d7af0u, 0x5d7b10u, 0x5d7c60u, 0x5d7c70u, 0x5d7ce0u,
        0x5d7d90u, 0x5d7e00u, 0x5d7e80u, 0x5d7f90u, 0x5d7fc0u,
        0x5d7fd0u, 0x5d7ff0u, 0x5d8130u, 0x5d8140u, 0x5d8150u,
        0x5d8180u, 0x5d8240u, 0x5d8420u, 0x5d8590u, 0x5d85f0u,
        0x5d8610u, 0x5d8640u, 0x5d8650u, 0x5d8670u, 0x5d86e0u,
        0x5d8710u, 0x5d8760u, 0x5d8820u, 0x5d8870u, 0x5d88a0u,
        0x5d88f0u, 0x5d8920u, 0x5d8960u, 0x5d8980u, 0x5d89a0u,
        0x5d8b40u, 0x5d8b60u, 0x5d8b70u, 0x5d8b80u, 0x5d8b90u,
        0x5d8bd0u, 0x5d8be0u, 0x5d8bf0u};
    static constexpr std::array<std::uint32_t, 158> kExtensionCleanups{
        0x460e90u, 0x460ff0u, 0x461150u, 0x4612b0u, 0x461410u,
        0x461570u, 0x4616d0u, 0x461830u, 0x461990u, 0x461af0u,
        0x461c50u, 0x461db0u, 0x461f10u, 0x462070u, 0x4621d0u,
        0x462330u, 0x462490u, 0x4625f0u, 0x462750u, 0x4628b0u,
        0x462a10u, 0x462b70u, 0x462cd0u, 0x462e30u, 0x462f90u,
        0x4630f0u, 0x463250u, 0x4633b0u, 0x463510u, 0x463670u,
        0x4637d0u, 0x463930u, 0x463a90u, 0x463bf0u, 0x463d50u,
        0x463eb0u, 0x464010u, 0x464170u, 0x4642d0u, 0x464430u,
        0x464590u, 0x4646f0u, 0x464850u, 0x4649b0u, 0x464b10u,
        0x464c70u, 0x464dd0u, 0x464f30u, 0x465090u, 0x4651f0u,
        0x465350u, 0x4654b0u, 0x465610u, 0x465770u, 0x4658d0u,
        0x465a30u, 0x465b90u, 0x465cf0u, 0x465e50u, 0x465fb0u,
        0x466110u, 0x466270u, 0x4663d0u, 0x466530u, 0x466690u,
        0x4667f0u, 0x466950u, 0x466ab0u, 0x466c10u, 0x466d70u,
        0x466ed0u, 0x467030u, 0x467190u, 0x4672f0u, 0x467450u,
        0x4675b0u, 0x467710u, 0x467870u, 0x4679d0u, 0x467b30u,
        0x467c90u, 0x467df0u, 0x467f50u, 0x4680b0u, 0x468210u,
        0x468370u, 0x4684d0u, 0x468630u, 0x468790u, 0x4688f0u,
        0x468a50u, 0x468bb0u, 0x468d10u, 0x468e70u, 0x468fd0u,
        0x469130u, 0x469290u, 0x4693f0u, 0x469550u, 0x4696b0u,
        0x469810u, 0x469970u, 0x469ad0u, 0x469c30u, 0x469d90u,
        0x469ef0u, 0x46a050u, 0x46a1b0u, 0x46a310u, 0x46a470u,
        0x46a5d0u, 0x46a730u, 0x46a890u, 0x46a9f0u, 0x46ab50u,
        0x46acb0u, 0x46ae10u, 0x46af70u, 0x46b0d0u, 0x46b230u,
        0x46b390u, 0x46b4f0u, 0x46b650u, 0x46b7b0u, 0x46b910u,
        0x46ba70u, 0x46bbd0u, 0x46bd30u, 0x46be90u, 0x46bff0u,
        0x46c150u, 0x46c2b0u, 0x46c410u, 0x46c570u, 0x46c6d0u,
        0x46c830u, 0x46c990u, 0x46caf0u, 0x46cc50u, 0x46cdb0u,
        0x46cf10u, 0x46d070u, 0x46d1d0u, 0x46d330u, 0x46d490u,
        0x46d5f0u, 0x46d750u, 0x46d8b0u, 0x46da10u, 0x46db70u,
        0x46dcd0u, 0x46de30u, 0x46df90u, 0x46e0f0u, 0x46e250u,
        0x46e3b0u, 0x46e510u, 0x46e670u};

    explicit ScriptBuiltinRegistry(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    [[nodiscard]] int InitializeImmBegin();
    void InitializeImmBeginDescriptor();
    void RegisterImmBegin();
    void CleanupImmBegin();

    // 0x5b7430..0x5b7aa0: 26 initializer/registration/cleanup triples.
    [[nodiscard]] int InitializeStatic(ScriptBuiltinStaticCommand command);
    void RegisterStatic(ScriptBuiltinStaticCommand command);
    void CleanupStatic(ScriptBuiltinStaticCommand command);
    [[nodiscard]] const ScriptBuiltinDescriptor& StaticDescriptor(
        ScriptBuiltinStaticCommand command) const;

    [[nodiscard]] int InitializeImmEnd();
    void RegisterImmEnd();
    void CleanupImmEnd();

    [[nodiscard]] int InitializeNewGame();
    void RegisterNewGame();
    void CleanupNewGame();

    [[nodiscard]] int InitializeCameraYpr();
    void RegisterCameraYpr();
    void CleanupCameraYpr();
    [[nodiscard]] int InitializeCameraDistance();
    void RegisterCameraDistance();
    void CleanupCameraDistance();
    [[nodiscard]] int InitializeCameraYprd();
    void RegisterCameraYprd();
    void CleanupCameraYprd();

    [[nodiscard]] int InitializeCameraGetDistance();
    void RegisterCameraGetDistance();
    void CleanupCameraGetDistance();
    [[nodiscard]] int InitializeCameraGetYaw();
    void RegisterCameraGetYaw();
    void CleanupCameraGetYaw();
    [[nodiscard]] int InitializeCameraGetPitch();
    void RegisterCameraGetPitch();
    void CleanupCameraGetPitch();
    [[nodiscard]] int InitializeCameraGetRoll();
    void RegisterCameraGetRoll();
    void CleanupCameraGetRoll();

    [[nodiscard]] int InitializeArenaLoad();
    void RegisterArenaLoad();
    void CleanupArenaLoad();
    [[nodiscard]] int InitializeArenaReady();
    void RegisterArenaReady();
    void CleanupArenaReady();
    [[nodiscard]] int InitializeArenaReadyRestore();
    void RegisterArenaReadyRestore();
    void CleanupArenaReadyRestore();
    [[nodiscard]] int InitializeArenaHint();
    void RegisterArenaHint();
    void CleanupArenaHint();
    [[nodiscard]] int InitializeArenaComeFromHere();
    void RegisterArenaComeFromHere();
    void CleanupArenaComeFromHere();
    [[nodiscard]] int InitializePlayerSetLeader();
    void RegisterPlayerSetLeader();
    void CleanupPlayerSetLeader();

    [[nodiscard]] int InitializePlayerSetVisible();
    void RegisterPlayerSetVisible();
    void CleanupPlayerSetVisible();
    [[nodiscard]] int InitializePlayerAttachCollision();
    void RegisterPlayerAttachCollision();
    void CleanupPlayerAttachCollision();
    [[nodiscard]] int InitializePlayerDetachCollision();
    void RegisterPlayerDetachCollision();
    void CleanupPlayerDetachCollision();
    [[nodiscard]] int InitializePlayerLock();
    void RegisterPlayerLock();
    void CleanupPlayerLock();
    [[nodiscard]] int InitializePlayerUnlock();
    void RegisterPlayerUnlock();
    void CleanupPlayerUnlock();
    [[nodiscard]] int InitializeNpcSetVisible();
    void RegisterNpcSetVisible();
    void CleanupNpcSetVisible();
    [[nodiscard]] int InitializeNpcCreate();
    void RegisterNpcCreate();
    void CleanupNpcCreate();
    [[nodiscard]] int InitializeNpcDelete();
    void RegisterNpcDelete();
    void CleanupNpcDelete();
    [[nodiscard]] int InitializeSystemExchange();
    void RegisterSystemExchange();
    void CleanupSystemExchange();
    [[nodiscard]] int InitializeMonsterStopPursuit();
    void RegisterMonsterStopPursuit();
    void CleanupMonsterStopPursuit();
    [[nodiscard]] int InitializeExtension(ScriptBuiltinExtension command);
    void RegisterExtension(ScriptBuiltinExtension command);
    void CleanupExtension(ScriptBuiltinExtension command);
    [[nodiscard]] const ScriptBuiltinDescriptor& Extension(
        ScriptBuiltinExtension command) const;

    void DeallocateVirtualMemory(ScriptBuiltinDescriptor& descriptor);

    [[nodiscard]] const ScriptBuiltinDescriptor& ImmBegin() const noexcept {
        return imm_begin_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ImmEnd() const noexcept {
        return imm_end_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& NewGame() const noexcept {
        return new_game_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraYpr() const noexcept {
        return camera_ypr_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraDistance() const noexcept {
        return camera_distance_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraYprd() const noexcept {
        return camera_yprd_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraGetDistance() const noexcept {
        return camera_get_distance_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraGetYaw() const noexcept {
        return camera_get_yaw_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraGetPitch() const noexcept {
        return camera_get_pitch_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& CameraGetRoll() const noexcept {
        return camera_get_roll_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ArenaLoad() const noexcept {
        return arena_load_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ArenaReady() const noexcept {
        return arena_ready_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ArenaReadyRestore() const noexcept {
        return arena_ready_restore_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ArenaHint() const noexcept {
        return arena_hint_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& ArenaComeFromHere() const noexcept {
        return arena_come_from_here_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerSetLeader() const noexcept {
        return player_set_leader_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerSetVisible() const noexcept {
        return player_set_visible_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerAttachCollision() const noexcept {
        return player_attach_collision_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerDetachCollision() const noexcept {
        return player_detach_collision_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerLock() const noexcept {
        return player_lock_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& PlayerUnlock() const noexcept {
        return player_unlock_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& NpcSetVisible() const noexcept {
        return npc_set_visible_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& NpcCreate() const noexcept {
        return npc_create_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& NpcDelete() const noexcept {
        return npc_delete_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& SystemExchange() const noexcept {
        return system_exchange_;
    }
    [[nodiscard]] const ScriptBuiltinDescriptor& MonsterStopPursuit() const noexcept {
        return monster_stop_pursuit_;
    }

private:
    void RegisterDescriptor(ScriptBuiltinDescriptor& descriptor,
                            std::uint32_t descriptor_address,
                            std::string_view description,
                            std::string_view signature,
                            std::uint32_t handler);

    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor imm_begin_{};
    std::array<ScriptBuiltinDescriptor, 26> static_descriptors_{};
    ScriptBuiltinDescriptor imm_end_{};
    ScriptBuiltinDescriptor new_game_{};
    ScriptBuiltinDescriptor camera_ypr_{};
    ScriptBuiltinDescriptor camera_distance_{};
    ScriptBuiltinDescriptor camera_yprd_{};
    ScriptBuiltinDescriptor camera_get_distance_{};
    ScriptBuiltinDescriptor camera_get_yaw_{};
    ScriptBuiltinDescriptor camera_get_pitch_{};
    ScriptBuiltinDescriptor camera_get_roll_{};
    ScriptBuiltinDescriptor arena_load_{};
    ScriptBuiltinDescriptor arena_ready_{};
    ScriptBuiltinDescriptor arena_ready_restore_{};
    ScriptBuiltinDescriptor arena_hint_{};
    ScriptBuiltinDescriptor arena_come_from_here_{};
    ScriptBuiltinDescriptor player_set_leader_{};
    ScriptBuiltinDescriptor player_set_visible_{};
    ScriptBuiltinDescriptor player_attach_collision_{};
    ScriptBuiltinDescriptor player_detach_collision_{};
    ScriptBuiltinDescriptor player_lock_{};
    ScriptBuiltinDescriptor player_unlock_{};
    ScriptBuiltinDescriptor npc_set_visible_{};
    ScriptBuiltinDescriptor npc_create_{};
    ScriptBuiltinDescriptor npc_delete_{};
    ScriptBuiltinDescriptor system_exchange_{};
    ScriptBuiltinDescriptor monster_stop_pursuit_{};
    std::array<ScriptBuiltinDescriptor, 158> extensions_{};
};

}  // namespace pal4
