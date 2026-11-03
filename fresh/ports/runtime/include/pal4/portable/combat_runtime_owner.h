#pragma once

#include "pal4/portable/combat_state_lifecycle.h"
#include "pal4/portable/combat_system_cold_runtime.h"
#include "pal4/portable/combat_system_service.h"
#include "pal4/portable/combat_world_resources.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace pal4::portable {

// Owns the portable Combat composition while borrowing platform owners.
// Platform services and the assigned-request resolver must outlive this owner.
class CombatRuntimeOwner final {
public:
    using RequestConfigResolver = std::function<std::uint32_t()>;

    CombatRuntimeOwner(
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
        RequestConfigResolver request_config);
    ~CombatRuntimeOwner();

    CombatRuntimeOwner(const CombatRuntimeOwner&) = delete;
    CombatRuntimeOwner& operator=(const CombatRuntimeOwner&) = delete;

    [[nodiscard]] bool Start(std::uint32_t config, std::uint32_t mode);
    [[nodiscard]] bool StartAssignedRequest(std::uint32_t mode);
    [[nodiscard]] bool Enter(CombatStateOwnerGeneration caller,
                             std::uint32_t previous_state,
                             std::uint32_t request_document);
    void Leave(CombatStateOwnerGeneration caller, std::uint32_t next_state);
    void Render(CombatStateOwnerGeneration caller, std::uint32_t frame);
    [[nodiscard]] std::uint32_t Update(CombatStateOwnerGeneration caller,
                                       float elapsed);
    void Retire(CombatStateOwnerGeneration caller);

    [[nodiscard]] CombatStateLifecycleBridge::Phase StatePhase() const noexcept;
    [[nodiscard]] bool WorldLoaded() const noexcept;
    [[nodiscard]] CombatWorldResources::Lease BorrowWorld();
    // Call after Leave and after host renderers have returned every borrowed lease.
    void ReleaseWorldResources();
    [[nodiscard]] bool TranslatorLoaded() const noexcept;
    [[nodiscard]] std::size_t TranslationCount() const;
    [[nodiscard]] std::optional<std::string_view> FindTranslation(
        std::string_view english) const;
    // Call only after all Combat consumers, including UI, have stopped.
    void ReleaseTranslator();

private:
    class StartServices;
    class SceneServices;

    [[nodiscard]] bool StartFromScene(std::uint32_t config,
                                      std::uint32_t mode);
    [[nodiscard]] bool StartInternal(std::uint32_t config,
                                     std::uint32_t mode,
                                     bool allow_active_state);

    CombatWorldResources world_;
    CombatSystemColdRuntimeAdapter cold_;
    std::unique_ptr<StartServices> start_services_;
    CombatSystemService system_;
    std::unique_ptr<SceneServices> scene_services_;
    CombatStateLifecycleBridge lifecycle_;
    bool start_attempted_{};
};

} // namespace pal4::portable
