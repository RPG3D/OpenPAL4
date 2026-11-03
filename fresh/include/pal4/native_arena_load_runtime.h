#pragma once

#include "pal4/game_arena_load_handler.h"
#include "pal4/pal_game_iv_state_transition.h"
#include "pal4/world_loading_state.h"
#include "pal4/native_arena_scene_load_services.h"
#include "pal4/native_bink_state_dispatch.h"
#include "pal4/native_combat_state_dispatch.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pal4 {

class GameRenderState;
class NativeGameArenaManagerOwner;
class NativeCameraManagerOwner;
class NativePalGameIVStateList;
class NativePalGameIVTextureStorage;
class NativeWorldLoadingStateOwner;
class ISystemMenuRuntime;
class ISmithStateRuntime;
class ITradeStateRuntime;
class NativeModalFrameStateRuntime;
class NativePuzzleStateRuntime;
class NativeWorldMapStateRuntime;
class INativeLogoStateDispatch;
class INativeGameOverStateDispatch;

class INativeArenaLoadHost : public INativeArenaSceneLoadServices {
public:
    virtual ~INativeArenaLoadHost() = default;
    [[nodiscard]] virtual std::uint32_t ActorControlAddress() = 0;
    [[nodiscard]] virtual std::uint32_t InputManagerAddress() = 0;
    [[nodiscard]] virtual std::uint32_t UiFrameManagerAddress() = 0;
    [[nodiscard]] virtual std::uint32_t Milliseconds() = 0;
    virtual void ProcessActorInput(bool clear_vector) = 0;
    virtual void InitializeInputState() = 0;
    virtual void ActivateDefaultCamera() = 0;
    virtual void ShowLoadingWindow(bool visible) = 0;
    virtual void SetDefaultCursor() = 0;
    virtual void LogStateExchange(std::string_view previous,
                                  std::string_view next,
                                  std::uint32_t source_line) = 0;
    [[nodiscard]] virtual std::uint32_t RenderLoadingUi(
        std::uint32_t frame) = 0;
    virtual void LoadSave(std::uint32_t slot) = 0;
    virtual void LeaveSceneState(std::uint32_t next_id) = 0;
    [[nodiscard]] virtual bool EnterSceneState(
        std::uint32_t previous_id, std::uint32_t argument) = 0;
    [[nodiscard]] virtual bool EnterSaveState(
        std::uint32_t previous_id, std::uint32_t argument) = 0;
    virtual void LeaveSaveState(std::uint32_t next_id) = 0;
    virtual void ProcessSaveWindowEvents() = 0;
    [[nodiscard]] virtual std::uint32_t SaveFrameAddress() = 0;
    [[nodiscard]] virtual bool SaveFrameVisible(std::uint32_t frame) = 0;
};

// Bridges giArenaLoad's temporary XML config to the registered Loading state.
class NativeArenaLoadRuntime final : public IGameArenaLoadRuntime,
                                     public INativeBinkStateHost,
                                     private IPalGameIVStateRuntime,
                                     private IWorldLoadingConfigRuntime,
                                     private IWorldLoadingFlowRuntime {
public:
    NativeArenaLoadRuntime(NativeGameArenaManagerOwner& arena,
                           NativePalGameIVTextureStorage& game,
                           NativePalGameIVStateList& states,
                           NativeWorldLoadingStateOwner& loading,
                           NativeCameraManagerOwner& cameras,
                           GameRenderState& game_state,
                           INativeArenaLoadHost& host);
    ~NativeArenaLoadRuntime() override;
    [[nodiscard]] bool LoadingActive() const;
    [[nodiscard]] bool SceneActive() const;
    [[nodiscard]] bool LoadingScenePending() const;
    [[nodiscard]] std::uint32_t AdvanceLoading();
    std::uint32_t RenderLoading(std::uint32_t frame);
    [[nodiscard]] bool TransitionToScene();
    // Logo is allowed at cold start or from Logo/Loading. Returning from a
    // live arena requires its complete release path before adding that route.
    void BindLogoState(INativeLogoStateDispatch* runtime);
    [[nodiscard]] bool TransitionToLogo();
    // After actual quit's arena release and B4 System->Scene, enter Logo.
    // Rejects a live world, record or pipeline; scope covers Scene leave/Logo entry.
    [[nodiscard]] bool ReturnReleasedSceneToLogo();
    [[nodiscard]] bool LogoActive() const;
    void RenderLogo(std::uint32_t frame, bool obey_movie_clock = true);
    [[nodiscard]] std::uint32_t UpdateLogo(float elapsed,
                                          bool suppress_screen_effect = false);
    void BindBinkState(INativeBinkStateDispatch* runtime);
    [[nodiscard]] bool TransitionToBink();
    [[nodiscard]] bool BinkActive() const;
    void RenderBink(std::uint32_t frame, bool obey_movie_clock = true);
    [[nodiscard]] std::uint32_t UpdateBink(float elapsed);
    std::uint32_t CreateBinkReturnDocument() override;
    std::uint32_t BinkStateGameAddress() const override;
    void ReturnBinkToScene(std::uint32_t document) override;
    std::uint32_t DestroyBinkReturnDocument(std::uint32_t document) override;
    [[nodiscard]] bool TransitionToSave();
    // Borrows the system UI services. Unbind before destroying them; active
    // System state must return to Scene before its services can be replaced.
    void BindSystemMenu(ISystemMenuRuntime* runtime);
    void BindSmithState(ISmithStateRuntime* runtime,ISystemMenuRuntime* render_runtime);
    void BindTradeState(ITradeStateRuntime* runtime);
    void BindModalFrameState(NativeModalFrameStateRuntime* runtime);
    void BindPuzzleState(NativePuzzleStateRuntime* runtime);
    void BindWorldMapState(NativeWorldMapStateRuntime* runtime);
    [[nodiscard]] bool TransitionToWorldMap(std::uint32_t document = 0u);
    [[nodiscard]] bool WorldMapActive() const;
    [[nodiscard]] std::uint32_t UpdateWorldMap(float elapsed);
    [[nodiscard]] std::uint32_t RenderWorldMap(std::uint32_t frame);
    bool TransitionToPuzzle(std::int32_t mode,std::int32_t type);
    bool PuzzleActive() const;
    std::uint32_t UpdatePuzzle(float elapsed);
    std::uint32_t RenderPuzzle(std::uint32_t frame);
    bool TransitionToModalFrame(std::uint32_t document);
    bool ModalFrameActive()const;
    std::uint8_t UpdateModalFrame();
    std::uint32_t RenderModalFrame(std::uint32_t frame);
    [[nodiscard]] bool TransitionToTrade(std::uint32_t argument=0u);
    [[nodiscard]] bool TradeActive() const;
    [[nodiscard]] std::uint32_t UpdateTrade(float elapsed);
    [[nodiscard]] std::uint32_t RenderTrade(std::uint32_t frame);
    [[nodiscard]] bool TransitionToSmith(std::uint32_t document=0u);
    [[nodiscard]] bool SmithActive()const;
    [[nodiscard]] std::uint32_t UpdateSmith(float elapsed);
    [[nodiscard]] std::uint32_t RenderSmith(std::uint32_t frame);
    [[nodiscard]] std::optional<std::uint32_t> ReadConfigInteger(std::uint32_t document,std::string_view key);
    [[nodiscard]] bool TransitionToSystem();
    void BindCombatState(INativeCombatStateDispatch* runtime);
    [[nodiscard]] bool TransitionToCombat(std::uint32_t document);
    [[nodiscard]] bool CombatActive() const;
    void RenderCombat(std::uint32_t frame);
    [[nodiscard]] std::uint32_t UpdateCombat(float elapsed);
    [[nodiscard]] bool ReturnCombatToScene(std::uint32_t document);
    void BindGameOverState(INativeGameOverStateDispatch*);
    bool TransitionCombatToGameOver(std::uint32_t document);
    bool GameOverActive()const;
    void RenderGameOver(std::uint32_t frame);
    void UpdateGameOver(float elapsed);
    [[nodiscard]] bool SystemActive() const;
    [[nodiscard]] std::uint32_t RenderSystem(std::uint32_t frame);
    [[nodiscard]] std::uint32_t UpdateSystem(float elapsed, std::uint32_t mode,
                                            std::uint32_t flag);
    [[nodiscard]] bool SaveActive() const;
    [[nodiscard]] std::uint8_t AdvanceSave();
    [[nodiscard]] std::size_t LiveDocumentCount() const noexcept {
        return documents_.size();
    }

private:
    struct Attribute {
        std::string key;
        std::variant<std::int32_t, std::string> value;
    };
    [[nodiscard]] std::uint32_t NewAttribute(
        std::string_view key, std::variant<std::int32_t, std::string> value);
    [[nodiscard]] const Attribute* AttributeFor(
        std::uint32_t document, std::string_view key) const;
    [[nodiscard]] bool Contains(std::uint32_t address,
                                std::uint32_t base,
                                std::uint32_t bytes) const noexcept;

    [[nodiscard]] std::uint32_t ArenaManager() override;
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t InitializeXmlDocument() override;
    [[nodiscard]] std::optional<std::uint32_t> FindAttribute(
        std::uint32_t document, std::string_view key) override;
    void SetInt(std::uint32_t attribute, std::int32_t value) override;
    void SetString(std::uint32_t attribute, std::string_view value) override;
    [[nodiscard]] std::uint32_t InitializeIntAttribute(
        std::string_view key, std::int32_t value) override;
    [[nodiscard]] std::uint32_t InitializeStringAttribute(
        std::string_view key, std::string_view value) override;
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override;
    void DestroyAttribute(std::uint32_t attribute) override;
    [[nodiscard]] std::uint32_t GameInstance() override;
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t document) override;
    [[nodiscard]] std::uint32_t ActorControl() override;
    void ProcessActorInput(std::uint32_t actor) override;
    void ProcessActorInputAndClearVector(std::uint32_t actor) override;
    [[nodiscard]] std::uint32_t InputManager() override;
    void InitializeInputState(std::uint32_t input) override;
    [[nodiscard]] std::uint32_t Milliseconds() override;
    [[nodiscard]] std::uint32_t GameState() override;
    [[nodiscard]] std::int32_t DestroyXmlDocument(
        std::uint32_t document) override;

    void RegisterAtExit(std::uint32_t callback) override;
    [[nodiscard]] std::uint32_t FindEntry(std::uint32_t game,
                                          std::uint32_t id) override;
    void Leave(std::uint32_t state, std::uint32_t next_id) override;
    [[nodiscard]] std::uint32_t StateId(std::uint32_t state) override;
    [[nodiscard]] std::uint32_t UiFrameManager() override;
    [[nodiscard]] std::uint32_t UIFrameManager() override;
    void SetCursor(std::uint32_t cursor_slot,
                   std::string_view resource) override;
    void LogExchange(std::string_view previous,
                     std::string_view next,
                     std::uint32_t source_line) override;
    [[nodiscard]] bool Enter(std::uint32_t state,
                             std::uint32_t previous_id,
                             std::uint32_t argument) override;
    [[nodiscard]] std::uint32_t TimeGetTime() override;
    void WriteGameStateWord(std::uint32_t offset,
                            std::uint32_t value) override;

    [[nodiscard]] std::uint32_t CameraManager() override;
    [[nodiscard]] std::uint32_t DefaultCameraName() override;
    void ActivateCamera(std::uint32_t manager,
                        std::uint32_t name) override;
    [[nodiscard]] std::uint32_t LoadingWindow(
        std::uint32_t manager) override;
    void SetLoadingWindowVisible(std::uint32_t window,
                                 bool visible) override;
    [[nodiscard]] std::optional<std::uint32_t> ReadType(
        std::uint32_t config, std::string_view key) override;
    [[nodiscard]] std::optional<std::string> ReadString(
        std::uint32_t config, std::string_view key) override;
    void AssignString(std::uint32_t destination,
                      std::string_view value) override;
    [[nodiscard]] std::optional<std::uint32_t> ReadTreeInteger(
        std::uint32_t config, std::string_view key) override;

    [[nodiscard]] std::uint32_t RenderUI(std::uint32_t manager,
                                         std::uint32_t frame) override;
    void LoadSave(std::uint32_t slot) override;
    [[nodiscard]] std::uint32_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) override;
    void LoadSceneResources(std::uint32_t arena,
                            std::uint32_t scene,
                            std::uint32_t area) override;
    void LoadSceneRuntime(std::uint32_t arena,
                          std::uint32_t resources) override;
    void InitializePlayerScene(std::uint32_t arena) override;

    NativeGameArenaManagerOwner& arena_;
    NativePalGameIVTextureStorage& game_;
    NativePalGameIVStateList& states_;
    NativeWorldLoadingStateOwner& loading_;
    NativeCameraManagerOwner& cameras_;
    GameRenderState& game_state_;
    INativeArenaLoadHost& host_;
    ISystemMenuRuntime* system_menu_ = nullptr;
    ISmithStateRuntime* smith_state_ = nullptr;
    ISystemMenuRuntime* smith_render_ = nullptr;
    ITradeStateRuntime* trade_state_ = nullptr;
    NativeModalFrameStateRuntime* modal_frame_ = nullptr;
    NativePuzzleStateRuntime* puzzle_ = nullptr;
    NativeWorldMapStateRuntime* world_map_ = nullptr;
    INativeLogoStateDispatch* logo_ = nullptr;
    INativeBinkStateDispatch* bink_ = nullptr;
    INativeCombatStateDispatch* combat_ = nullptr;
    INativeGameOverStateDispatch* game_over_ = nullptr;
    bool returning_released_scene_ = false;
    PalGameIVStateLabels labels_;
    std::uint32_t next_handle_ = 1u;
    std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> documents_;
    std::unordered_map<std::uint32_t, Attribute> attributes_;
};

}  // namespace pal4
