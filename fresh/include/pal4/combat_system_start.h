#pragma once

#include <cstdint>
#include "pal4/combat_system_start_render.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

class ICombatSystemStartFieldsRuntime {
public:
    virtual ~ICombatSystemStartFieldsRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,std::uint8_t value) = 0;
};

class ICombatSystemStartCollectionsRuntime {
public:
    virtual ~ICombatSystemStartCollectionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void ProcessCollections(std::uint32_t collection) = 0;
    virtual void InitializeGameStats(std::uint32_t handler) = 0;
    virtual void UpdateSystemManager(std::uint32_t manager) = 0;
};

class ICombatSystemStartConsoleRuntime {
public:
    virtual ~ICombatSystemStartConsoleRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetConsole() = 0;
    virtual void ResetConsole(std::uint32_t console) = 0;
    virtual void SetupCameraDefaults() = 0;
    virtual void UnsubscribeConsoleEvents(std::uint32_t console) = 0;
    virtual void SubscribeConsoleEvents(std::uint32_t console) = 0;
    virtual void DispatchScriptEvent(std::string_view name,std::int32_t value) = 0;
};

class ICombatSystemStartRuntime : public ICombatSystemStartCollectionsRuntime,
                                  public ICombatSystemStartConsoleRuntime,
                                  public ICombatSystemStartFieldsRuntime,
                                  public ICombatSystemStartRenderRuntime {
public:
    ~ICombatSystemStartRuntime() override = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) override = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void GetSkillSystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    virtual void ResetTaskFlags(std::uint32_t queue) = 0;
    virtual void CleanupTaskQueue(std::uint32_t queue) = 0;
    [[nodiscard]] virtual std::optional<std::string> FindAttributeString(
        std::uint32_t node, std::string_view key) = 0;
    [[nodiscard]] virtual std::int32_t GetConfigInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    virtual void InitializeCold(std::uint32_t object) = 0;
    //529BD0: clear Player/Monster rosters and reset task flags; the108-byte
    //cbActorManager singleton remains alive (distinct from member424).
    virtual void ClearCombatRosters(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t GetCombatSystemInstance() = 0;
    virtual void LoadWorld(std::uint32_t object, std::string_view scene,
                           std::string_view area) = 0;
    virtual void InitializeGameManager(std::uint32_t manager) = 0;
    virtual void ShowCombatUi(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetChildByIndex(
        std::uint32_t config, std::uint32_t index) = 0;
    virtual void CreateMonster(std::uint32_t manager, std::int32_t monster_id,
                               std::uint32_t position,
                               std::int32_t ab_state) = 0;
    [[nodiscard]] virtual std::uint32_t GetPlayer(
        std::uint32_t index) = 0;
    virtual void SortPlayers(std::vector<std::uint32_t>& players) = 0;
    virtual void PreparePlayer(std::uint32_t player) = 0;
    virtual void CreatePlayerActor(std::uint32_t collection,
                                   std::uint32_t player,
                                   std::uint32_t position) = 0;
    [[nodiscard]] virtual std::uint32_t GetRenderCallback() = 0;
    virtual void SetRenderCallback(std::uint32_t callback) = 0;
    virtual void ResetAudioPosition() = 0;
};

// Same5756D0 ordering over typed production owners. The legacy runtime
// adapter below retains original field/container access for existing callers.
class ICombatSystemStartServices {
public:
    virtual ~ICombatSystemStartServices() = default;
    virtual void InitializeRequest(std::uint32_t mode) = 0;
    virtual std::int32_t ConfigChildCount(std::uint32_t config) = 0;
    virtual std::optional<std::string> FindAttributeString(std::uint32_t config,std::string_view key) = 0;
    virtual std::int32_t GetConfigInt(std::uint32_t config,std::string_view key,std::int32_t fallback) = 0;
    virtual bool ColdInitialized() = 0;
    virtual void InitializeCold() = 0;
    virtual void ResetCollections() = 0;
    virtual void ClearRosters() = 0;
    virtual void ResetConsole() = 0;
    virtual bool Inactive() = 0;
    virtual void LoadWorld(std::string_view scene,std::string_view area) = 0;
    virtual void InitializeGameManager() = 0;
    virtual void ShowUi() = 0;
    virtual void CreateMonsters(std::uint32_t config,std::int32_t count) = 0;
    virtual void CreatePlayers() = 0;
    virtual bool HasPlayers() = 0;
    virtual bool CommitReady(std::int32_t start_type) = 0;
};

class CombatSystemStart final {
public:
    static constexpr std::uint32_t kRenderCallback = CombatSystemStartRender::kRenderCallback;

    //5756D0 after task ResetFlags/Cleanup, before config validation/cold init.
    static void InitializeRequestFields(std::uint32_t object,std::uint32_t mode,
        ICombatSystemStartFieldsRuntime&);

    //5756D0 after cold init: ReleaseAll412, GameStats416, scheduler396.
    static void ResetCollections(std::uint32_t object,
                                ICombatSystemStartCollectionsRuntime& runtime);
    //5756D0 after roster clear: animation clear, camera, subscriptions, reset event.
    static void ResetConsoleServices(std::uint32_t object,
                                    ICombatSystemStartConsoleRuntime& runtime);

    [[nodiscard]] static bool StartWithServices(std::uint32_t config,
        std::uint32_t mode,ICombatSystemStartServices&);

    // IDA 0x5756d0.
    [[nodiscard]] static bool Start(std::uint32_t object,
                                    std::uint32_t config,
                                    std::uint32_t mode,
                                    ICombatSystemStartRuntime& runtime);
};

}  // namespace pal4
