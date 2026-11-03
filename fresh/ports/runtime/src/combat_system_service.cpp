#include "pal4/portable/combat_system_service.h"

#include <stdexcept>

namespace pal4::portable {

class CombatSystemService::StartServices final
    : public ICombatSystemStartServices {
public:
    StartServices(std::uint32_t owner, ICombatSystemColdRuntime& cold,
                  ICombatSystemStartServices& start)
        : owner_(owner), cold_(cold), start_(start) {}

    void InitializeRequest(std::uint32_t mode) override {
        start_.InitializeRequest(mode);
    }
    std::int32_t ConfigChildCount(std::uint32_t config) override {
        return start_.ConfigChildCount(config);
    }
    std::optional<std::string> FindAttributeString(
        std::uint32_t config, std::string_view key) override {
        return start_.FindAttributeString(config, key);
    }
    std::int32_t GetConfigInt(std::uint32_t config, std::string_view key,
                              std::int32_t fallback) override {
        return start_.GetConfigInt(config, key, fallback);
    }
    bool ColdInitialized() override { return start_.ColdInitialized(); }
    void InitializeCold() override {
        (void)CombatSystemSession::Initialize(owner_, cold_);
    }
    void ResetCollections() override { start_.ResetCollections(); }
    void ClearRosters() override { start_.ClearRosters(); }
    void ResetConsole() override { start_.ResetConsole(); }
    bool Inactive() override { return start_.Inactive(); }
    void LoadWorld(std::string_view scene, std::string_view area) override {
        start_.LoadWorld(scene, area);
    }
    void InitializeGameManager() override { start_.InitializeGameManager(); }
    void ShowUi() override { start_.ShowUi(); }
    void CreateMonsters(std::uint32_t config, std::int32_t count) override {
        start_.CreateMonsters(config, count);
    }
    void CreatePlayers() override { start_.CreatePlayers(); }
    bool HasPlayers() override { return start_.HasPlayers(); }
    bool CommitReady(std::int32_t start_type) override {
        return start_.CommitReady(start_type);
    }

private:
    std::uint32_t owner_;
    ICombatSystemColdRuntime& cold_;
    ICombatSystemStartServices& start_;
};

CombatSystemService::CombatSystemService(
    std::uint32_t logical_system_owner,
    ICombatSystemColdRuntime& cold_runtime,
    ICombatSystemStartServices& start_services)
    : owner_(logical_system_owner), cold_runtime_(cold_runtime),
      start_services_(start_services) {
    if (!owner_)
        throw std::invalid_argument("Combat service requires a logical owner");
}

bool CombatSystemService::Start(std::uint32_t config, std::uint32_t mode) {
    if (starting_)
        throw std::logic_error("Combat Start cannot reenter its owner");
    starting_ = true;
    struct Reset final {
        bool& starting;
        ~Reset() { starting = false; }
    } reset{starting_};

    StartServices services(owner_, cold_runtime_, start_services_);
    return CombatSystemStart::StartWithServices(config, mode, services);
}

} // namespace pal4::portable
