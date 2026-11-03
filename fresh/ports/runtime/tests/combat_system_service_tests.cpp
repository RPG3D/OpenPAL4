#include "pal4/portable/combat_system_service.h"
#include "pal4/portable/combat_system_cold_runtime.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatSystemColdRuntime,
                       pal4::ICombatSystemStartServices {
    static constexpr std::uint32_t kOwner = 0x1000u;
    std::vector<std::string> calls;
    bool cold{}, has_players{true}, ready{true}, fail_reset{};

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kOwner + 517u && value == 1u,
              "cold init commits the original initialized byte");
        cold = true;
        calls.emplace_back("cold-commit");
    }
    void LogInitialization(bool beginning) override {
        calls.emplace_back(beginning ? "cold-log-begin" : "cold-log-end");
    }
    std::uint32_t LogInitialized() override {
        calls.emplace_back("cold-log-initialized");
        return 0u;
    }
    void GetSkillSystem() override { calls.emplace_back("skill-system"); }
    void LoadTranslator(std::uint32_t address, std::string_view path) override {
        Check(address == kOwner + 380u &&
              path == "gamedata\\database\\combatDb.db",
              "cold init loads the original translator resource");
        calls.emplace_back("translator");
    }
    std::uint32_t GetConsole() override {
        calls.emplace_back("get-console");
        return 7u;
    }
    void SubscribeConsoleEvents(std::uint32_t console) override {
        Check(console == 7u, "cold init subscribes the owned console");
        calls.emplace_back("subscribe-console");
    }
    void InitializeMatFx(std::uint32_t owner) override {
        Check(owner == kOwner, "cold init applies MatFX to its owner");
        calls.emplace_back("matfx");
    }

    void InitializeRequest(std::uint32_t mode) override {
        Check(mode == 0u, "request mode is preserved");
        calls.emplace_back("request");
    }
    std::int32_t ConfigChildCount(std::uint32_t config) override {
        Check(config == 11u, "start config is preserved");
        calls.emplace_back("config-count");
        return 1;
    }
    std::optional<std::string> FindAttributeString(
        std::uint32_t, std::string_view key) override {
        calls.emplace_back(key == "CombatScene" ? "scene" : "area");
        if (key == "CombatScene") return "M01";
        if (key == "CombatArea") return "1";
        return std::nullopt;
    }
    std::int32_t GetConfigInt(std::uint32_t, std::string_view key,
                              std::int32_t fallback) override {
        Check(key == "StartType" && fallback == 0,
              "original start type field is queried");
        calls.emplace_back("start-type");
        return 2;
    }
    bool ColdInitialized() override { return cold; }
    void InitializeCold() override { throw std::logic_error("must use shared cold initializer"); }
    void ResetCollections() override { calls.emplace_back("collections"); }
    void ClearRosters() override { calls.emplace_back("rosters"); }
    void ResetConsole() override { calls.emplace_back("console-reset"); }
    bool Inactive() override { calls.emplace_back("inactive"); return false; }
    void LoadWorld(std::string_view scene, std::string_view area) override {
        Check(scene == "M01" && area == "1", "configured combat world forwarded");
        calls.emplace_back("world");
    }
    void InitializeGameManager() override { calls.emplace_back("game-manager"); }
    void ShowUi() override { calls.emplace_back("show-ui"); }
    void CreateMonsters(std::uint32_t config, std::int32_t count) override {
        Check(config == 11u && count == 1, "configured monster list forwarded");
        calls.emplace_back("monsters");
    }
    void CreatePlayers() override { calls.emplace_back("players"); }
    bool HasPlayers() override { calls.emplace_back("has-players"); return has_players; }
    bool CommitReady(std::int32_t start_type) override {
        Check(start_type == 2, "start type reaches original ready commit");
        calls.emplace_back("commit-ready");
        return ready;
    }
};

void UsesOriginalColdAndStartSequences() {
    Runtime runtime;
    pal4::portable::CombatSystemService service(Runtime::kOwner, runtime, runtime);
    Check(service.Start(11u, 0u), "ready commit is reported as a successful start");
    Check(runtime.calls == std::vector<std::string>({
        "request", "config-count", "scene", "area", "start-type",
        "cold-log-begin", "skill-system", "translator", "get-console",
        "subscribe-console", "matfx", "skill-system", "cold-log-initialized",
        "cold-commit", "collections", "rosters", "console-reset", "inactive",
        "world", "game-manager", "show-ui", "monsters", "players",
        "has-players", "commit-ready"}),
        "service delegates the original cold init and 5756D0 order");
}

void RejectionAndOwnerValidationStayHonest() {
    bool invalid_owner{};
    try {
        Runtime runtime;
        pal4::portable::CombatSystemService service(0u, runtime, runtime);
    } catch (const std::invalid_argument&) {
        invalid_owner = true;
    }
    Check(invalid_owner, "zero is not a valid logical owner handle");

    Runtime runtime;
    runtime.has_players = false;
    pal4::portable::CombatSystemService service(Runtime::kOwner, runtime, runtime);
    Check(!service.Start(11u, 0u), "missing players is an actual failed start");
    Check(runtime.calls.back() == "has-players" &&
          std::find(runtime.calls.begin(), runtime.calls.end(), "commit-ready") ==
              runtime.calls.end(),
          "failed player gate never commits ready state");
}

void ColdInitOwnsVerifiedOriginalTranslator(const std::filesystem::path& game_data) {
    Runtime runtime;
    pal4::portable::CombatSystemColdRuntimeAdapter cold(
        Runtime::kOwner, game_data, runtime);
    pal4::portable::CombatSystemService service(Runtime::kOwner, cold, runtime);
    Check(service.Start(11u, 0u), "cold init reaches original ready commit");
    Check(cold.TranslatorLoaded() && cold.TranslationCount() == 229u &&
          cold.FindTranslation("EASI_NONE") ==
              std::string_view("\xce\xde\xd2\xec\xb3\xa3 "),
          "shared 5754C0 cold init loads the verified original catalog");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(), "translator") ==
              runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(), "cold-commit") !=
              runtime.calls.end(),
          "portable catalog replaces the x86 translator binding before cold commit");

    cold.ResetTranslator();
    Check(!cold.TranslatorLoaded(), "translator catalog releases with its owner");
    bool lookup_rejected{};
    try { (void)cold.FindTranslation("EASI_NONE"); }
    catch (const std::logic_error&) { lookup_rejected = true; }
    Check(lookup_rejected, "translation views cannot outlive translator ownership");

    Runtime failed_runtime;
    pal4::portable::CombatSystemColdRuntimeAdapter failed_cold(
        Runtime::kOwner, game_data / "missing", failed_runtime);
    bool load_failed{};
    try { (void)pal4::CombatSystemSession::Initialize(Runtime::kOwner, failed_cold); }
    catch (const std::exception&) { load_failed = true; }
    Check(load_failed && !failed_cold.TranslatorLoaded() && !failed_runtime.cold &&
          std::find(failed_runtime.calls.begin(), failed_runtime.calls.end(),
                    "cold-commit") == failed_runtime.calls.end(),
          "failed original database load cannot mark Combat cold initialization complete");
}
} // namespace

int main(int argc, char** argv) {
    try {
        UsesOriginalColdAndStartSequences();
        RejectionAndOwnerValidationStayHonest();
        if (argc > 1) ColdInitOwnsVerifiedOriginalTranslator(argv[1]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
