#pragma once

#include <cstdint>

namespace pal4 {

enum class StaticBuiltin : std::uint8_t {
    ImmBegin,
    ImmEnd,
    NewGame,
    CameraYpr,
    CameraCtrlDist,
    CameraCtrlYprd,
    CameraGetDist,
    CameraGetYaw,
    CameraGetPitch,
    CameraGetRoll,
    ArenaLoad,
    ArenaReady,
    ArenaReadyRestore,
    ArenaHint,
    ArenaComeFromHere,
    PlayerSetLeader,
    PlayerSetVisible,
    PlayerAttachCollision,
    PlayerDetachCollision,
    PlayerLock,
    PlayerUnlock,
    SetNpcVisible,
    NpcCreate,
    NpcDelete,
    SystemExchange,
    MonsterStopPursuit,
    GlobalVariables,
    MazeVariables,
    MazeVar2,
    MazeVar3,
    MazeVar4,
    MazeVar5,
    MazeVar6,
    MazeVar7,
    MazeVar8,
    MazeVar9,
    MazeVar10,
    VarShu,
    VarXia,
    VarWu,
    VarFeng,
    VarZi,
    VarMo,
    VarMing,
    VarYan,
    VarCha,
    VarStar,
    NpcA,
    NpcB,
    NpcC,
    NpcD,
    NpcE,
    NpcF,
    NpcG,
    VarZhu,
    VarShop031,
    VarShop010,
    VarShop032,
    VarShop021,
    scriptRegisterIntVarShop012,
    scriptRegisterIntVarShop034,
    scriptRegisterIntVarShop037,
    scriptRegisterIntVarShop004,
    scriptRegisterIntVarShop024,
    scriptRegisterIntVarShop025,
    scriptRegisterIntVarShop005,
    scriptRegisterIntVarShop035,
    scriptRegisterIntVarShop026,
    scriptRegisterIntVarShop006,
    scriptRegisterIntVarShop027,
    scriptRegisterIntVarShop007,
    scriptRegisterIntVarShop029,
    scriptRegisterIntVarShop009,
    scriptRegisterIntVarShop030,
    CameraSetMode,
    GetGoodsOpenCondition,
    SetGoodsOpenCondition,
    NpcPauseBeh,
    NpcResumeBeh,
    OpenWeather,
    CloseWeather,
    SetMinimapExpmode,
    GetRandnum,
    SetTempGameState,
    FlushTailYAngle,
    AddCombatMonster,
    ConfigCombatParam,
    ConfigCombatBgm,
    ConfigCombatCamera,
    StartCombat,
    SetObjectVisible,
    AddProperty,
    DelProperty,
    PlayerInTeam,
    PlayerOutTeam,
    GOMTouch,
    CameraSetCollide,
    CameraSeekToPlayer,
    CameraAutoSeek,
    PlayerSetAttr,
    PlayerGetLeader,
    SetPlayerLevel,
    AddPlayerEquip,
    OpenMovieFlag,
    AddQuestComplatePercentage,
    AddEquipment,
    PlayerCurrentSetVisible,
    SetFullHP,
    SetFullMP,
    GetPlayerLevel,
    GotoLogo,
    PlayerUnHoldAct,
    NpcUnHoldAct,
    CameraSetDistOptEnable,
    MonsterSetHide,
    GameObjectSetResearch,
    SetPortrait,
    BGMConfigSetMusic,
    BGMConfigIsInArea,
    ScriptMusicMute,
    ScriptMusicPlay,
    ScriptMusicStop,
    ArenaMusicStop,
    WorldMapSetState,
    GetPuzzleGameResult,
    AlwaysJump,
    Sound2DPlay,
    Sound2DStop,
    Sound2DStopID,
    CGEffPlay,
    CGEffStop,
    EffectPlay,
    EffectPlayWithPlayer,
    EffectPlayWithCurrentPlayer,
    EffectPlayWithNPC,
    EffectPlayWithOBJ,
    EffectStopWithOBJ,
    ShowHint,
    PlayerAddSkill,
    MonsterSetVisible,
    PlayerRandomPosition,
    PlayerCurrentRandomPosition,
    EventVolumeVisible,
    AllPlayerGarb2,
    PlayerGarb2,
    AllPlayerGarb1,
    PlayerGarb1,
    GetVisibleObject,
    GetVisibleMonster,
    CheckPackProperty,
    GrantSystemUi,
    OpenSystemUi,
    GrantSmithSystem,
    GrantMagicSystem,
    CheckMagicMastered,
    SelectDialogAddItem,
    SelectDialogGetLastSelect,
    GOBAttachToPlayerEntry,
    GOBAttachToCurrentPlayer,
    GOBDetachFromPlayerEntry,
    GOBDetachFromCurrentPlayer,
    EffectAttachToPlayer,
    EffectAttachToCurrentPlayer,
    EffectDetachFromPlayer,
    EffectDetachFromCurrentPlayer,
    EffectAttachToNpc,
    EffectDetachFromNpc,
    GOBAttachToNpcEntry,
    GOBDetachFromNPCEntry,
    GOBSetPosition,
    ScriptClearCTXButCurrent,
    AddMoney,
    PayMoney,
    HideGASkillObject,
    ShowSignpost,
    PlayerSetEmotion,
    PlayerResetEmotion,
    PlayerCurrentSetEmotion,
    PlayerCurrentResetEmotion,
    LINGSHALegsInjured,
    LINGSHALegsHealing,
    NpcSetEmotion,
    NpcResetEmotion,
    GetPropertyNumb,
    PlayerCurrentGetPosX,
    PlayerCurrentGetPosY,
    PlayerCurrentGetPosZ,
    ArenaGetName,
    ArenaGetArea,
    ArenaSkillEnable,
    ShowInnDialog,
    GetInnDialogResult,
    PlayerTakeARest,
    IsNightTime,
    PlayerSetPosRot,
    PlayerSetPosRotByName,
    StartUiTimer,
    PlayerForbidenSkill,
    GetMoney,
    SelectDialogSetDefaultSelect,
    ShowQuestDialog,
    GetQuestDialogResult,
    ResetPlayerToJumpStart,
    GOBResetEntry,
    CheckEquipInInventory,
    RemoveEquipment,
    AddPrescription,
    EnableShadow,
    AddRoundTimes,
    TimeScript,
    TimeScriptTerminate,
    ConfigCombatGroundCamera,
    AddPlayerFavor,
    GetPalTestResult,
    SetMinimapLevel,
    PetShow,
    NpcAttachEffect,
    NpcDetachEffect,
    MstAttachEffect,
    MstDetachEffect,
    PlayerHookEffect,
    PlayerDetachEffect,
    CommonDialogGetLastSelect,
    ConfigCombatVipMonster,
    EnableSTS,
    ClearUiTimer,
    PauseUiTimer,
    ResumeUiTimer,
    SetFullRage,
    UiTimerGetSaveData,
    ScriptMusicPause,
    ScriptMusicResume,
    Count,
};

class IScriptBuiltinStaticRuntime {
public:
    virtual ~IScriptBuiltinStaticRuntime() = default;
    virtual void RegisterBuiltin(StaticBuiltin builtin,
                                 std::uint32_t descriptor) = 0;
    virtual void RegisterBuiltinAt(StaticBuiltin builtin,
                                   std::uint32_t descriptor,
                                   std::uint32_t native_entry) {
        (void)native_entry;
        RegisterBuiltin(builtin, descriptor);
    }
    [[nodiscard]] virtual int RegisterAtExit(std::uint32_t callback) = 0;
    virtual void DeallocateDescriptor(std::uint32_t descriptor) = 0;
};

class ScriptBuiltinStaticRegistration final {
public:
    // IDA 0x486e20/30/50 through 0x486ee0/f0 and 0x486f10.
    [[nodiscard]] static int InitializeImmBegin(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterImmBegin(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupImmBegin(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeImmEnd(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterImmEnd(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupImmEnd(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeNewGame(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterNewGame(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupNewGame(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraYpr(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraYpr(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraYpr(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraCtrlDist(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraCtrlDist(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraCtrlDist(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraCtrlYprd(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraCtrlYprd(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraCtrlYprd(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraGetDist(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraGetDist(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraGetDist(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraGetYaw(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraGetYaw(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraGetYaw(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraGetPitch(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraGetPitch(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraGetPitch(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeCameraGetRoll(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterCameraGetRoll(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupCameraGetRoll(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeArenaLoad(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterArenaLoad(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupArenaLoad(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeArenaReady(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterArenaReady(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupArenaReady(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeArenaReadyRestore(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterArenaReadyRestore(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupArenaReadyRestore(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeArenaHint(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterArenaHint(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupArenaHint(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeArenaComeFromHere(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterArenaComeFromHere(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupArenaComeFromHere(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerSetLeader(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerSetLeader(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerSetLeader(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerSetVisible(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerSetVisible(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerSetVisible(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerAttachCollision(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerAttachCollision(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerAttachCollision(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerDetachCollision(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerDetachCollision(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerDetachCollision(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerLock(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerLock(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerLock(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializePlayerUnlock(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterPlayerUnlock(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupPlayerUnlock(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeSetNpcVisible(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterSetNpcVisible(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupSetNpcVisible(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeNpcCreate(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterNpcCreate(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupNpcCreate(IScriptBuiltinStaticRuntime& runtime);

    [[nodiscard]] static int InitializeNpcDelete(IScriptBuiltinStaticRuntime& runtime);
    static void RegisterNpcDelete(IScriptBuiltinStaticRuntime& runtime);
    static void CleanupNpcDelete(IScriptBuiltinStaticRuntime& runtime);

#define PAL4_STATIC_BUILTIN_DECLARATIONS(Name)                                \
    [[nodiscard]] static int Initialize##Name(IScriptBuiltinStaticRuntime&);   \
    static void Register##Name(IScriptBuiltinStaticRuntime&);                   \
    static void Cleanup##Name(IScriptBuiltinStaticRuntime&);
    PAL4_STATIC_BUILTIN_DECLARATIONS(SystemExchange)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MonsterStopPursuit)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GlobalVariables)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVariables)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar2)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar3)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar4)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar5)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar6)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar7)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar8)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar9)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MazeVar10)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarShu)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarXia)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarWu)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarFeng)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarZi)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarMo)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarMing)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarYan)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarCha)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarStar)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcA)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcB)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcC)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcD)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcE)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcF)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcG)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarZhu)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarShop031)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarShop010)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarShop032)
    PAL4_STATIC_BUILTIN_DECLARATIONS(VarShop021)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop012)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop034)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop037)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop004)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop024)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop025)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop005)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop035)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop026)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop006)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop027)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop007)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop029)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop009)
    PAL4_STATIC_BUILTIN_DECLARATIONS(scriptRegisterIntVarShop030)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CameraSetMode)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetGoodsOpenCondition)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetGoodsOpenCondition)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcPauseBeh)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcResumeBeh)
    PAL4_STATIC_BUILTIN_DECLARATIONS(OpenWeather)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CloseWeather)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetMinimapExpmode)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetRandnum)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetTempGameState)
    PAL4_STATIC_BUILTIN_DECLARATIONS(FlushTailYAngle)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddCombatMonster)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ConfigCombatParam)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ConfigCombatBgm)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ConfigCombatCamera)
    PAL4_STATIC_BUILTIN_DECLARATIONS(StartCombat)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetObjectVisible)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddProperty)
    PAL4_STATIC_BUILTIN_DECLARATIONS(DelProperty)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerInTeam)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerOutTeam)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOMTouch)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CameraSetCollide)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CameraSeekToPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CameraAutoSeek)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerSetAttr)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerGetLeader)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetPlayerLevel)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddPlayerEquip)
    PAL4_STATIC_BUILTIN_DECLARATIONS(OpenMovieFlag)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddQuestComplatePercentage)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddEquipment)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentSetVisible)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetFullHP)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetFullMP)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetPlayerLevel)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GotoLogo)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerUnHoldAct)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcUnHoldAct)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CameraSetDistOptEnable)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MonsterSetHide)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GameObjectSetResearch)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetPortrait)
    PAL4_STATIC_BUILTIN_DECLARATIONS(BGMConfigSetMusic)
    PAL4_STATIC_BUILTIN_DECLARATIONS(BGMConfigIsInArea)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptMusicMute)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptMusicPlay)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptMusicStop)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ArenaMusicStop)
    PAL4_STATIC_BUILTIN_DECLARATIONS(WorldMapSetState)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetPuzzleGameResult)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AlwaysJump)
    PAL4_STATIC_BUILTIN_DECLARATIONS(Sound2DPlay)
    PAL4_STATIC_BUILTIN_DECLARATIONS(Sound2DStop)
    PAL4_STATIC_BUILTIN_DECLARATIONS(Sound2DStopID)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CGEffPlay)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CGEffStop)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectPlay)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectPlayWithPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectPlayWithCurrentPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectPlayWithNPC)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectPlayWithOBJ)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectStopWithOBJ)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ShowHint)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerAddSkill)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MonsterSetVisible)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerRandomPosition)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentRandomPosition)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EventVolumeVisible)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AllPlayerGarb2)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerGarb2)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AllPlayerGarb1)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerGarb1)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetVisibleObject)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetVisibleMonster)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CheckPackProperty)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GrantSystemUi)
    PAL4_STATIC_BUILTIN_DECLARATIONS(OpenSystemUi)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GrantSmithSystem)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GrantMagicSystem)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CheckMagicMastered)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SelectDialogAddItem)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SelectDialogGetLastSelect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBAttachToPlayerEntry)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBAttachToCurrentPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBDetachFromPlayerEntry)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBDetachFromCurrentPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectAttachToPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectAttachToCurrentPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectDetachFromPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectDetachFromCurrentPlayer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectAttachToNpc)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EffectDetachFromNpc)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBAttachToNpcEntry)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBDetachFromNPCEntry)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBSetPosition)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptClearCTXButCurrent)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddMoney)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PayMoney)
    PAL4_STATIC_BUILTIN_DECLARATIONS(HideGASkillObject)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ShowSignpost)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerSetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerResetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentSetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentResetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(LINGSHALegsInjured)
    PAL4_STATIC_BUILTIN_DECLARATIONS(LINGSHALegsHealing)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcSetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcResetEmotion)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetPropertyNumb)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentGetPosX)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentGetPosY)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerCurrentGetPosZ)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ArenaGetName)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ArenaGetArea)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ArenaSkillEnable)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ShowInnDialog)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetInnDialogResult)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerTakeARest)
    PAL4_STATIC_BUILTIN_DECLARATIONS(IsNightTime)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerSetPosRot)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerSetPosRotByName)
    PAL4_STATIC_BUILTIN_DECLARATIONS(StartUiTimer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerForbidenSkill)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetMoney)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SelectDialogSetDefaultSelect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ShowQuestDialog)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetQuestDialogResult)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ResetPlayerToJumpStart)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GOBResetEntry)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CheckEquipInInventory)
    PAL4_STATIC_BUILTIN_DECLARATIONS(RemoveEquipment)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddPrescription)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EnableShadow)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddRoundTimes)
    PAL4_STATIC_BUILTIN_DECLARATIONS(TimeScript)
    PAL4_STATIC_BUILTIN_DECLARATIONS(TimeScriptTerminate)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ConfigCombatGroundCamera)
    PAL4_STATIC_BUILTIN_DECLARATIONS(AddPlayerFavor)
    PAL4_STATIC_BUILTIN_DECLARATIONS(GetPalTestResult)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetMinimapLevel)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PetShow)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcAttachEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(NpcDetachEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MstAttachEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(MstDetachEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerHookEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PlayerDetachEffect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(CommonDialogGetLastSelect)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ConfigCombatVipMonster)
    PAL4_STATIC_BUILTIN_DECLARATIONS(EnableSTS)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ClearUiTimer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(PauseUiTimer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ResumeUiTimer)
    PAL4_STATIC_BUILTIN_DECLARATIONS(SetFullRage)
    PAL4_STATIC_BUILTIN_DECLARATIONS(UiTimerGetSaveData)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptMusicPause)
    PAL4_STATIC_BUILTIN_DECLARATIONS(ScriptMusicResume)
#undef PAL4_STATIC_BUILTIN_DECLARATIONS
};

struct ActorManagerScriptBuiltinSpec {
    std::uint32_t descriptor;
    std::uint32_t native_entry;
    std::uint32_t initializer;
    std::uint32_t wrapper;
    std::uint32_t cleanup;
};

class ActorManagerScriptBuiltinRegistration final {
public:
    [[nodiscard]] static ActorManagerScriptBuiltinSpec SpecFor(
        StaticBuiltin builtin);
    [[nodiscard]] static int Initialize(StaticBuiltin builtin,
                                        IScriptBuiltinStaticRuntime& runtime);
    static void Register(StaticBuiltin builtin,
                         IScriptBuiltinStaticRuntime& runtime);
    static void Cleanup(StaticBuiltin builtin,
                        IScriptBuiltinStaticRuntime& runtime);
};

}  // namespace pal4
