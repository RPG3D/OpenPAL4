#include "pal4/portable/combat_runtime_owner.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

template<class F> void Reject(F&& action) {
    bool rejected{};
    try { action(); }
    catch (const std::logic_error&) { rejected = true; }
    Check(rejected, "invalid repeated Combat Start was not rejected");
}

struct Services final : pal4::ICombatSystemColdRuntime,
                        pal4::ICombatSystemStartServices,
                        pal4::ICombatSceneServices,
                        pal4::ICombatSceneFrameRuntime,
                        pal4::ICombatSystemFrameRenderer,
                        pal4::ICombatSceneUpdateServices,
                        pal4::portable::ICombatStateReadiness {
    static constexpr std::uint32_t kSystem = 0x2000u;
    static constexpr std::uint64_t kGeneration = 0x12345678u;

    std::vector<std::string> calls;
    bool cold{}, registered{true};
    bool has_config_child{true};
    std::uint32_t assigned_config{}, collection_resets{}, world_loads{};

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kSystem + 517u && value == 1u,
              "original cold marker address and value");
        calls.emplace_back("cold-commit"); cold = true;
    }
    void LogInitialization(bool beginning) override {
        calls.emplace_back(beginning ? "cold-begin" : "cold-end");
    }
    std::uint32_t LogInitialized() override { return 0u; }
    void GetSkillSystem() override { calls.emplace_back("skill"); }
    void LoadTranslator(std::uint32_t translator, std::string_view path) override {
        Check(translator == kSystem + 380u &&
              path == "gamedata\\database\\combatDb.db",
              "original translator child and package path");
        calls.emplace_back("platform-translator");
    }
    std::uint32_t GetConsole() override { return 7u; }
    void SubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 7u, "cold console owner");
    }
    void InitializeMatFx(std::uint32_t owner) override {
        Check(owner == kSystem, "cold MatFX owner");
    }

    void InitializeRequest(std::uint32_t) override { calls.emplace_back("request"); }
    std::int32_t ConfigChildCount(std::uint32_t config) override {
        Check(config == assigned_config, "state-owned Combat request reaches Start");
        calls.emplace_back("config"); return has_config_child ? 1 : 0;
    }
    std::optional<std::string> FindAttributeString(
        std::uint32_t, std::string_view key) override {
        if (key == "CombatScene") return "M01";
        if (key == "CombatArea") return "M01";
        return std::nullopt;
    }
    std::int32_t GetConfigInt(std::uint32_t, std::string_view,
                              std::int32_t fallback) override { return fallback; }
    bool ColdInitialized() override { return cold; }
    void InitializeCold() override { throw std::logic_error("shared cold init was bypassed"); }
    void ResetCollections() override { ++collection_resets; calls.emplace_back("collections"); }
    void ClearRosters() override { calls.emplace_back("rosters"); }
    void ResetConsole() override { calls.emplace_back("console-reset"); }
    bool Inactive() override { return false; }
    void LoadWorld(std::string_view scene, std::string_view area) override {
        Check(scene == "M01" && area == "M01", "Start world route");
        ++world_loads; calls.emplace_back("world");
    }
    void InitializeGameManager() override { calls.emplace_back("manager"); }
    void ShowUi() override { calls.emplace_back("start-ui"); }
    void CreateMonsters(std::uint32_t, std::int32_t) override { calls.emplace_back("monsters"); }
    void CreatePlayers() override { calls.emplace_back("players"); }
    bool HasPlayers() override { return true; }
    bool CommitReady(std::int32_t) override { calls.emplace_back("ready"); return true; }

    void EnableUi() override { calls.emplace_back("entry-ui"); }
    void HideConsole() override {}
    void HideScenarioDialog() override {}
    void CloseHint() override {}
    std::uint32_t GameFlag(std::uint32_t) override { return 1u; }
    void SetGameFlag(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SavedFlag(std::uint32_t) override { return 1u; }
    void SetSavedFlag(std::uint32_t, std::uint32_t) override {}
    void AssignRequest(std::uint32_t document) override { assigned_config = document; }
    void SetupCamera() override {}
    bool ForceNormal() override { return true; }
    std::uint32_t ChoiceMode() override { return 0u; }
    void SetWaitingState() override {}
    void ShowChoice() override {}
    void ActivateCamera(bool) override {}
    std::uint8_t BeginCombat(std::uint32_t) override {
        throw std::logic_error("owner must connect BeginCombat to CombatSystemService");
    }
    void CloseCombatUi() override {}

    std::uint32_t ReadWord(std::uint32_t) override { return 1u; }
    std::uint32_t CombatSystem() override { return kSystem; }
    bool IsCombatSystemInactive(std::uint32_t) override { return false; }
    std::uint32_t ArenaManager() override { return 1u; }
    void ProcessArenaFrame(std::uint32_t, std::uint32_t) override {}
    void Render(std::uint32_t) override {}

    void AdvanceCombat(float) override {}
    void UpdateUI(double) override {}
    std::uint32_t State() override { return 0u; }
    bool Alternate() override { return false; }
    std::uint32_t Result() override { return 0u; }
    void CleanupScriptCache() override {}
    std::uint32_t ConstructXmlDocument() override { return 1u; }
    std::uint32_t DestroyXmlDocument(std::uint32_t) override { return 0u; }
    void WriteResult(std::uint32_t, std::uint32_t, bool) override {}
    void Transition(std::uint32_t, std::uint32_t) override {}

    void ValidateRegisteredCombatOwner(
        pal4::portable::CombatStateOwnerGeneration generation,
        std::uint32_t state_id) const override {
        if (!registered || generation != kGeneration || state_id != 3u)
            throw std::logic_error("unregistered Combat state");
    }
    bool IsRegisteredCombatOwner(
        pal4::portable::CombatStateOwnerGeneration generation,
        std::uint32_t state_id) const override {
        return registered && generation == kGeneration && state_id == 3u;
    }
};

void RepeatedStartCannotDisturbActiveOwners(const std::filesystem::path& game_data) {
    Services services;
    pal4::portable::CombatRuntimeOwner owner(
        Services::kSystem, Services::kGeneration, game_data, services, services,
        services, services, services, services, services,
        [&] { return services.assigned_config; });

    Check(owner.Enter(Services::kGeneration, 1u, 0x5000u),
          "original PALIV_COMBAT Entry is accepted");
    Check(owner.StatePhase() ==
              pal4::portable::CombatStateLifecycleBridge::Phase::Active &&
          owner.WorldLoaded() && owner.TranslatorLoaded() &&
          owner.TranslationCount() == 229u && services.cold &&
          services.world_loads == 1u && services.collection_resets == 1u,
          "Entry runs cold init, real Start and CombatWorld package load");

    const auto calls = services.calls.size();
    const auto collections = services.collection_resets;
    Reject([&] { (void)owner.Start(services.assigned_config, 0u); });
    Reject([&] { (void)owner.StartAssignedRequest(2u); });
    Check(services.calls.size() == calls &&
          services.collection_resets == collections &&
          services.world_loads == 1u && owner.WorldLoaded() &&
          owner.TranslatorLoaded() && owner.TranslationCount() == 229u,
          "duplicate Start preserves active world, translator and platform owners");

    owner.Leave(Services::kGeneration, 1u);
    Check(!owner.WorldLoaded(), "Leave releases the owner's CombatWorld lease and data");
    services.registered = false;
    owner.Retire(Services::kGeneration);
    owner.ReleaseTranslator();
    Check(!owner.TranslatorLoaded(), "retirement ends translator ownership");
}

void MissingConfigChildCannotEnterCombat(const std::filesystem::path& game_data) {
    Services services;
    services.has_config_child = false;
    services.assigned_config = 0x5000u;
    pal4::portable::CombatRuntimeOwner owner(
        Services::kSystem, Services::kGeneration, game_data, services, services,
        services, services, services, services, services,
        [&] { return services.assigned_config; });

    Check(!owner.Start(services.assigned_config, 0u),
          "shared Start rejects a request without CombatParam children");
    Check(owner.StatePhase() ==
              pal4::portable::CombatStateLifecycleBridge::Phase::Ready &&
          !owner.WorldLoaded() && !owner.TranslatorLoaded() && !services.cold &&
          services.collection_resets == 0u && services.world_loads == 0u,
          "missing CombatParam is a Start rejection, not a ready or fake battle");

    const auto calls = services.calls.size();
    Reject([&] { (void)owner.Start(services.assigned_config, 0u); });
    Reject([&] { (void)owner.StartAssignedRequest(2u); });
    Check(services.calls.size() == calls && services.collection_resets == 0u &&
          services.world_loads == 0u,
          "repeated rejected Start cannot mutate platform collections");

    services.registered = false;
    owner.Retire(Services::kGeneration);
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 1) {
            MissingConfigChildCannotEnterCombat(
                std::filesystem::temp_directory_path() /
                "pal4-combat-owner-no-assets");
            std::cout << "SKIP original CPK/catalog branch: pass --original-gamedata <dir>\n";
            return 0;
        }
        if (argc != 3 || std::string(argv[1]) != "--original-gamedata")
            throw std::runtime_error("expected --original-gamedata <dir>");
        RepeatedStartCannotDisturbActiveOwners(std::filesystem::path(argv[2]));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
