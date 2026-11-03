#include "pal4/portable/combat_runtime_owner.h"

#include <stdexcept>
#include <exception>
#include <utility>

namespace pal4::portable {

class CombatRuntimeOwner::StartServices final
    : public ICombatSystemStartServices {
public:
    StartServices(CombatWorldResources& world,
                  ICombatSystemStartServices& platform)
        : world_(world), platform_(platform) {}

    void InitializeRequest(std::uint32_t mode) override {
        platform_.InitializeRequest(mode);
    }
    std::int32_t ConfigChildCount(std::uint32_t config) override {
        return platform_.ConfigChildCount(config);
    }
    std::optional<std::string> FindAttributeString(
        std::uint32_t config, std::string_view key) override {
        return platform_.FindAttributeString(config, key);
    }
    std::int32_t GetConfigInt(std::uint32_t config, std::string_view key,
                              std::int32_t fallback) override {
        return platform_.GetConfigInt(config, key, fallback);
    }
    bool ColdInitialized() override { return platform_.ColdInitialized(); }
    void InitializeCold() override { platform_.InitializeCold(); }
    void ResetCollections() override { platform_.ResetCollections(); }
    void ClearRosters() override { platform_.ClearRosters(); }
    void ResetConsole() override { platform_.ResetConsole(); }
    bool Inactive() override { return platform_.Inactive(); }
    void LoadWorld(std::string_view scene, std::string_view area) override {
        if (world_lease_)
            throw std::logic_error("CombatWorld lease is still owned by the active session");
        world_.Load(area);
        world_lease_.emplace(world_.Borrow());
        try {
            platform_.LoadWorld(scene, area);
        } catch (...) {
            const auto failure = std::current_exception();
            world_lease_.reset();
            try {
                world_.Reset();
            } catch (...) {
            }
            std::rethrow_exception(failure);
        }
    }
    void InitializeGameManager() override {
        platform_.InitializeGameManager();
    }
    void ShowUi() override { platform_.ShowUi(); }
    void CreateMonsters(std::uint32_t config, std::int32_t count) override {
        platform_.CreateMonsters(config, count);
    }
    void CreatePlayers() override { platform_.CreatePlayers(); }
    bool HasPlayers() override { return platform_.HasPlayers(); }
    bool CommitReady(std::int32_t start_type) override {
        return platform_.CommitReady(start_type);
    }

    [[nodiscard]] bool ColdIsInitialized() {
        return platform_.ColdInitialized();
    }

    void ReleaseWorld() {
        world_lease_.reset();
        if (world_.Loaded()) world_.Reset();
    }

private:
    CombatWorldResources& world_;
    ICombatSystemStartServices& platform_;
    std::optional<CombatWorldResources::Lease> world_lease_;
};

class CombatRuntimeOwner::SceneServices final : public ICombatSceneServices {
public:
    SceneServices(ICombatSceneServices& platform,
                  std::function<bool(std::uint32_t, std::uint32_t)> start,
                  RequestConfigResolver request_config)
        : platform_(platform), start_(std::move(start)),
          request_config_(std::move(request_config)) {}

    void EnableUi() override { platform_.EnableUi(); }
    void HideConsole() override { platform_.HideConsole(); }
    void HideScenarioDialog() override { platform_.HideScenarioDialog(); }
    void CloseHint() override { platform_.CloseHint(); }
    std::uint32_t GameFlag(std::uint32_t offset) override {
        return platform_.GameFlag(offset);
    }
    void SetGameFlag(std::uint32_t offset, std::uint32_t value) override {
        platform_.SetGameFlag(offset, value);
    }
    std::uint32_t SavedFlag(std::uint32_t index) override {
        return platform_.SavedFlag(index);
    }
    void SetSavedFlag(std::uint32_t index, std::uint32_t value) override {
        platform_.SetSavedFlag(index, value);
    }
    void AssignRequest(std::uint32_t document) override {
        platform_.AssignRequest(document);
    }
    void SetupCamera() override { platform_.SetupCamera(); }
    bool ForceNormal() override { return platform_.ForceNormal(); }
    std::uint32_t ChoiceMode() override { return platform_.ChoiceMode(); }
    void SetWaitingState() override { platform_.SetWaitingState(); }
    void ShowChoice() override { platform_.ShowChoice(); }
    void ActivateCamera(bool quick) override {
        platform_.ActivateCamera(quick);
    }
    std::uint8_t BeginCombat(std::uint32_t mode) override {
        const auto config = request_config_();
        if (!config)
            throw std::logic_error("assigned Combat request has no config owner");
        return start_(config, mode) ? 1u : 0u;
    }
    void CloseCombatUi() override { platform_.CloseCombatUi(); }

private:
    ICombatSceneServices& platform_;
    std::function<bool(std::uint32_t, std::uint32_t)> start_;
    RequestConfigResolver request_config_;
};

CombatRuntimeOwner::CombatRuntimeOwner(
    std::uint32_t logical_system_owner,
    CombatStateOwnerGeneration state_generation,
    std::filesystem::path game_data,
    ICombatStateReadiness& readiness,
    ICombatSystemColdRuntime& cold_platform,
    ICombatSystemStartServices& start_platform,
    ICombatSceneServices& scene_platform,
    ICombatSceneFrameRuntime& frame,
    ICombatSystemFrameRenderer& renderer,
    ICombatSceneUpdateServices& update,
    RequestConfigResolver request_config)
    : world_(game_data),
      cold_(logical_system_owner, std::move(game_data), cold_platform),
      start_services_(std::make_unique<StartServices>(world_, start_platform)),
      system_(logical_system_owner, cold_, *start_services_),
      scene_services_(std::make_unique<SceneServices>(
          scene_platform,
          [this](std::uint32_t config, std::uint32_t mode) {
              return StartFromScene(config, mode);
          }, request_config)),
      lifecycle_(state_generation, readiness, *scene_services_, frame,
                 renderer, update) {
    if (!request_config)
        throw std::invalid_argument("Combat owner requires an assigned-request resolver");
}

CombatRuntimeOwner::~CombatRuntimeOwner() = default;

bool CombatRuntimeOwner::Start(std::uint32_t config, std::uint32_t mode) {
    return StartInternal(config, mode, false);
}

bool CombatRuntimeOwner::StartFromScene(std::uint32_t config,
                                        std::uint32_t mode) {
    return StartInternal(config, mode, true);
}

bool CombatRuntimeOwner::StartInternal(std::uint32_t config,
                                       std::uint32_t mode,
                                       bool allow_active_state) {
    if (lifecycle_.CurrentPhase() == CombatStateLifecycleBridge::Phase::Retired)
        throw std::logic_error("Combat owner is retired");
    if (lifecycle_.CurrentPhase() == CombatStateLifecycleBridge::Phase::Faulted)
        throw std::logic_error("Combat owner failed and must be rebuilt");
    if (lifecycle_.Active() && !allow_active_state)
        throw std::logic_error("direct Combat Start is not allowed during an active state");
    if (world_.Loaded() || start_attempted_)
        throw std::logic_error("Combat Start was already attempted for this owner session");
    start_attempted_ = true;
    try {
        return system_.Start(config, mode);
    } catch (...) {
        const auto failure = std::current_exception();
        bool cold_initialized{};
        try {
            cold_initialized = start_services_->ColdIsInitialized();
        } catch (...) {
        }
        if (!cold_initialized) cold_.ResetTranslator();
        try {
            start_services_->ReleaseWorld();
        } catch (...) {
        }
        std::rethrow_exception(failure);
    }
}

bool CombatRuntimeOwner::StartAssignedRequest(std::uint32_t mode) {
    if (!lifecycle_.Active() || world_.Loaded() || start_attempted_)
        throw std::logic_error("assigned Combat Start requires a waiting state");
    return CombatSceneCoordinator::StartWithServices(mode, *scene_services_) != 0u;
}

bool CombatRuntimeOwner::Enter(CombatStateOwnerGeneration caller,
                               std::uint32_t previous_state,
                               std::uint32_t request_document) {
    if (lifecycle_.CurrentPhase() != CombatStateLifecycleBridge::Phase::Ready ||
        world_.Loaded())
        throw std::logic_error("Combat Entry requires a clean owner session");
    start_attempted_ = false;
    return lifecycle_.Enter(caller, previous_state, request_document);
}

void CombatRuntimeOwner::Leave(CombatStateOwnerGeneration caller,
                               std::uint32_t next_state) {
    lifecycle_.Leave(caller, next_state);
    start_services_->ReleaseWorld();
    start_attempted_ = false;
}

void CombatRuntimeOwner::Render(CombatStateOwnerGeneration caller,
                                std::uint32_t frame) {
    lifecycle_.Render(caller, frame);
}

std::uint32_t CombatRuntimeOwner::Update(CombatStateOwnerGeneration caller,
                                        float elapsed) {
    return lifecycle_.Update(caller, elapsed);
}

void CombatRuntimeOwner::Retire(CombatStateOwnerGeneration caller) {
    if (world_.Loaded())
        throw std::logic_error("CombatWorld resources must be released before retirement");
    lifecycle_.Retire(caller);
}

CombatStateLifecycleBridge::Phase CombatRuntimeOwner::StatePhase() const noexcept {
    return lifecycle_.CurrentPhase();
}

bool CombatRuntimeOwner::WorldLoaded() const noexcept {
    return world_.Loaded();
}

CombatWorldResources::Lease CombatRuntimeOwner::BorrowWorld() {
    return world_.Borrow();
}

void CombatRuntimeOwner::ReleaseWorldResources() {
    if (lifecycle_.NeedsLeave() || lifecycle_.Active())
        throw std::logic_error("Combat world can be released after a successful Leave");
    start_services_->ReleaseWorld();
}

bool CombatRuntimeOwner::TranslatorLoaded() const noexcept {
    return cold_.TranslatorLoaded();
}

std::size_t CombatRuntimeOwner::TranslationCount() const {
    return cold_.TranslationCount();
}

std::optional<std::string_view> CombatRuntimeOwner::FindTranslation(
    std::string_view english) const {
    return cold_.FindTranslation(english);
}

void CombatRuntimeOwner::ReleaseTranslator() {
    if (lifecycle_.CurrentPhase() != CombatStateLifecycleBridge::Phase::Retired)
        throw std::logic_error("Combat translator can be released after owner retirement");
    cold_.ResetTranslator();
}

} // namespace pal4::portable
