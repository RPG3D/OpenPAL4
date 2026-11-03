#pragma once

#include "pal4/cegui_renderer.h"
#include "pal4/minimap_scene_ui_stats.h"
#include "pal4/cegui_system_initializer.h"
#include "pal4/cegui_vertex_processor.h"
#include "pal4/native_cegui_package_resource_provider.h"
#include "pal4/native_cegui_texture.h"
#include "pal4/native_game_save_summary.h"
#include "pal4/native_jump_hint_progress.h"
#include "pal4/load_window_lifecycle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <exception>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "pal4/scenario_dialog.h"

#include <windows.h>

namespace pal4 {

struct PalActorControlState;
struct PlayerManagerObjectState;
class IScriptPlayerActionsRuntime;
class NativeEventHandlerArray;
class NativeGameDatabase;
struct NativeMissionRecord;
class ISystemSettingStatsDataRuntime;
class NativeSystemSettingSavePage;
class DialogManagerState;
class NativeSystemSettingSaveNavigation;
class NativeSystemSettingFrame;
class NativeSystemSettingOptionsNavigation;
class NativeTutorialRuntime;
class NativePoetryRuntime;
class NativeGameTimerQueue;
class IDialogManagerSettingsWriteRuntime;
enum class NativeCombatLayout : std::uint32_t { Main, Action, Role, Ending };

// Owns the original CEGUI DLL objects while the game's RenderWare engine lives.
class NativeCeguiUiSession final {
public:
    NativeCeguiUiSession();
    ~NativeCeguiUiSession();
    NativeCeguiUiSession(const NativeCeguiUiSession&) = delete;
    NativeCeguiUiSession& operator=(const NativeCeguiUiSession&) = delete;

    [[nodiscard]] bool Start(const std::filesystem::path& game_root,
                             std::uint32_t width, std::uint32_t height);
    void Stop() noexcept;
    [[nodiscard]] bool AdvanceTime(float elapsed);
    [[nodiscard]] std::uint32_t SystemAddress() const noexcept {
        return system_ready_ ? reinterpret_cast<std::uint32_t>(system_.data()) : 0u;
    }
    [[nodiscard]] bool Render();
    // Original Image/Font calls feed the shared renderer queue directly.
    // Flush it inside the enclosing begun camera frame; clear after the phase.
    [[nodiscard]] bool RenderQueuedOverlays();
    void ClearQueuedOverlays() noexcept;
    [[nodiscard]] std::uint32_t QueuedOverlayCount() const noexcept {
        return (renderer_.ReadDword(192u)-renderer_.ReadDword(188u))/60u;
    }
    [[nodiscard]] bool OverlayQueueEnabled() const noexcept { return renderer_.ReadByte(260u)!=0u; }
    void SetOverlayQueueEnabled(bool enabled);
    [[nodiscard]] bool LoadImageset(const std::filesystem::path& game_root,
                                    const char* filename);
    [[nodiscard]] bool LoadOriginalPuzzleFont(const std::filesystem::path& game_root);
    // Replaces GameSaveImage<slot>; release rows/cache borrowing the old image first.
    // Requires initialized widget resources. Returns0 and LastError on failure.
    [[nodiscard]] std::uint32_t LoadSaveThumbnail(std::int32_t slot,
        const std::filesystem::path& archive_root = {});
    // IDA470360: packed quest/<PICTURE>.png, cached Pal4QuestImage<PICTURE>.
    // Empty picture is a successful null image; other failures set LastError.
    [[nodiscard]] std::uint32_t LoadMissionImage(const NativeMissionRecord&);
    [[nodiscard]] bool InitializeWidgetRendering(
        const std::filesystem::path& game_root);
    [[nodiscard]] bool ShowDiagnosticImageWidget(
        const std::filesystem::path& game_root, const char* image_file);
    [[nodiscard]] bool LoadOriginalMainLayout();
    [[nodiscard]] bool HideMainLayout();
    [[nodiscard]] bool ShowMainLayout();
    [[nodiscard]] bool ShowOriginalLoadLayout(
        const std::filesystem::path& game_root);
    [[nodiscard]] std::uint32_t LoadRowCount() const;
    [[nodiscard]] std::uint32_t LoadColumnCount() const;
    [[nodiscard]] LoadWindowState LoadControls() const;
    void SetNumberedLoadPage(std::int32_t page, const std::array<std::uint32_t, 10>& items);
    [[nodiscard]] bool HideLoadLayout();
    [[nodiscard]] bool ShowOriginalSaveLayout(
        const std::filesystem::path& game_root);
    [[nodiscard]] std::uint32_t SaveRowCount() const;
    [[nodiscard]] std::uint32_t SaveColumnCount() const;
    [[nodiscard]] bool HideSaveLayout();
    [[nodiscard]] bool ShowOriginalLoadingLayout(
        const std::filesystem::path& game_root);
    [[nodiscard]] bool HideLoadingLayout();
    [[nodiscard]] bool LoadOriginalTutorialLayout();
    [[nodiscard]] bool ShowOriginalTutorial(const NativeGameDatabase& database, std::uint32_t key);
    void ShowTutorial(const NativeGameDatabase& database, DialogManagerState& globals,
        IDialogManagerSettingsWriteRuntime& writer, std::uint32_t game, std::uint32_t key);
    [[nodiscard]] bool TutorialVisible() const;
    [[nodiscard]] std::uint32_t TutorialFrameAddress() const noexcept {
        return tutorial_frame_[9] ? reinterpret_cast<std::uint32_t>(tutorial_frame_.data()) : 0u;
    }
    [[nodiscard]] bool DispatchTutorialEvent(bool key, const void* event) noexcept;
    void RethrowTutorialError();
    [[nodiscard]] bool LoadOriginalHintLayout();
    [[nodiscard]] bool ShowOriginalHint(std::string_view text, float x, float y,
        float duration, std::uint32_t parent, std::uint8_t flag);
    [[nodiscard]] bool HintVisible() const;
    [[nodiscard]] std::uint32_t HintFrameAddress() const noexcept {
        return hint_frame_[9] ? reinterpret_cast<std::uint32_t>(hint_frame_.data()) : 0u;
    }
    void BindHintTimers(NativeGameTimerQueue* timers) noexcept { hint_timers_ = timers; }
    void CloseOriginalHint();
    [[nodiscard]] bool DispatchHintEvent() noexcept;
    void RethrowHintError();
    [[nodiscard]] bool UpdateLoadingProgress(float progress);
    [[nodiscard]] bool LoadOriginalMinimapLayout(
        const std::filesystem::path& game_root);
    [[nodiscard]] bool SetMinimapSceneText(std::string_view game_text);
    [[nodiscard]] bool SetMinimapSceneTextUtf8(std::string_view text);
    [[nodiscard]] bool SetMinimapLayoutVisible(bool visible);
    [[nodiscard]] bool SetMinimapImageVisible(bool visible);
    [[nodiscard]] bool SetMinimapButtonGroupVisible(bool visible);
    [[nodiscard]] bool LoadOriginalSceneHudLayout();
    // Resource/window preparation only; the system state and toolbar input
    // controller own visibility events, game enables and click/navigation routes.
    [[nodiscard]] bool LoadOriginalSystemToolbarLayout(const std::filesystem::path& game_root);
    [[nodiscard]] bool SetSystemToolbarLayoutVisible(bool visible);
    // Original CEGUI resources only. Game frame controllers own callbacks,
    // subscriptions, actor values and lifecycle; preparation leaves roots hidden.
    [[nodiscard]] bool LoadOriginalCombatLayouts(const std::filesystem::path& game_root);
    [[nodiscard]] std::uint32_t CombatLayoutRoot(NativeCombatLayout) const noexcept;
    [[nodiscard]] std::uint32_t CombatLayoutControl(NativeCombatLayout, std::string_view suffix);
    [[nodiscard]] bool SetCombatLayoutVisible(NativeCombatLayout, bool visible,
                                             std::uint32_t parent = 0u);
    [[nodiscard]] bool CombatLayoutVisible(NativeCombatLayout) const;
    // Original resource preparation and 47FD80 reset only; page lifecycle,
    // statistics, settings persistence and callbacks belong to its controller.
    [[nodiscard]] bool LoadOriginalSystemSettingLayout(const std::filesystem::path& game_root);
    [[nodiscard]] bool ResetSystemSettingPage();
    [[nodiscard]] bool InitializeSystemSettingSaveControls(const NativeGameDatabase& database);
    // Borrows database while rows exist. Global selection/scroll is read per call.
    [[nodiscard]] bool PopulateSystemSettingSavePage(const std::filesystem::path& game_root,
        const NativeGameDatabase& database, const DialogManagerState& globals,
        std::int32_t page, bool load_mode);
    [[nodiscard]] bool BindSystemSettingPageClickHandler(std::function<bool(std::uint32_t)> handler,
        std::function<bool(std::uint32_t)> list_handler = {});
    void ClearSystemSettingPageClickHandler() noexcept;
    [[nodiscard]] bool DispatchSystemSettingPageClick(const void* event, bool list = false) noexcept;
    void RethrowSystemSettingPageClickError();
    [[nodiscard]] bool BindSystemSettingMainClickHandler(std::function<bool(std::uint32_t)> handler);
    void ClearSystemSettingMainClickHandler() noexcept;
    [[nodiscard]] bool DispatchSystemSettingMainClick(const void* event) noexcept;
    void RethrowSystemSettingMainClickError();
    [[nodiscard]] bool BindSystemSettingOptionClickHandler(std::function<bool(std::uint32_t)> handler);
    void ClearSystemSettingOptionClickHandler() noexcept;
    [[nodiscard]] bool DispatchSystemSettingOptionClick(const void* event) noexcept;
    void RethrowSystemSettingOptionClickError();
    [[nodiscard]] bool BindSystemSettingAudioEventHandler(std::function<bool(std::uint32_t, bool)> handler);
    void ClearSystemSettingAudioEventHandler() noexcept;
    [[nodiscard]] bool DispatchSystemSettingAudioEvent(const void* event, bool preview = false) noexcept;
    void RethrowSystemSettingAudioEventError();
    // Caller must resolve the borrowed frame+64/+96 input members in activity.
    // This prepares page contents; root visibility belongs to the frame owner.
    [[nodiscard]] bool ShowSystemSettingOptions(NativeEventHandlerArray& activity);
    [[nodiscard]] bool ShowSystemSettingData(const std::filesystem::path& game_root,
        const NativeGameDatabase& database, const DialogManagerState& globals,
        NativeEventHandlerArray& activity, bool load_mode);
    [[nodiscard]] std::uint32_t SystemSettingOptionsInputAddress() const noexcept {
        return SystemSettingRootAddress() ?
            reinterpret_cast<std::uint32_t>(system_setting_frame_.data()) + 64u : 0u;
    }
    [[nodiscard]] std::uint32_t SystemSettingSaveInputAddress() const noexcept {
        return SystemSettingRootAddress() ?
            reinterpret_cast<std::uint32_t>(system_setting_frame_.data()) + 96u : 0u;
    }
    [[nodiscard]] bool RefreshSystemSettingStats(
        const NativeGameDatabase& database, ISystemSettingStatsDataRuntime& data);
    [[nodiscard]] std::uint32_t SystemSettingRootAddress() const noexcept {
        return system_setting_frame_[9];
    }
    [[nodiscard]] bool BindSystemToolbarParent(bool attach, std::uint32_t parent = 0u);
    // Borrow a DLL window owned by a native dialog controller; reuse the shared
    // DialogLayoutBase parent algorithm and this session's actual desktop.
    [[nodiscard]] bool BindDialogRootParent(std::uint32_t root,bool attach,std::uint32_t parent = 0u);
    [[nodiscard]] bool BindSystemSettingParent(bool attach, std::uint32_t parent = 0u);
    [[nodiscard]] bool BindCombatMainParent(bool attach, std::uint32_t parent = 0u);
    // The callback borrows the DLL EventArgs only during dispatch. Game event,
    // navigation and state services must be bound by the toolbar controller.
    [[nodiscard]] bool BindSystemToolbarClickHandler(std::function<bool(std::uint32_t)> handler);
    void ClearSystemToolbarClickHandler() noexcept;
    [[nodiscard]] bool DispatchSystemToolbarClick(const void* event) noexcept;
    void RethrowSystemToolbarClickError();
    [[nodiscard]] std::uint32_t SystemToolbarRootAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(system_toolbar_layout_);
    }
    [[nodiscard]] const std::array<std::uint32_t, 7>& SystemToolbarButtons() const noexcept {
        return system_toolbar_buttons_;
    }
    [[nodiscard]] bool RefreshSceneHudPlayers(PlayerManagerObjectState& players,
                                             IScriptPlayerActionsRuntime& actions);
    [[nodiscard]] bool UpdateSceneHudPlayer(PlayerManagerObjectState& players);
    [[nodiscard]] std::uint32_t SceneHudSkillButtonAddress() const noexcept {
        return scene_hud_skill_button_;
    }
    [[nodiscard]] bool UpdateSceneHudMovement(const PalActorControlState& control);
    [[nodiscard]] std::uint32_t SceneHudRunButtonAddress() const noexcept {
        return scene_hud_run_button_;
    }
    [[nodiscard]] bool UpdateSceneHudStats(IMinimapSceneUiStatsDataRuntime& players);
    [[nodiscard]] std::uint32_t SceneHudTimerWindowAddress() const noexcept {
        return scene_hud_timer_text_;
    }
    [[nodiscard]] bool SetSceneHudTimerText(std::string_view text);
    [[nodiscard]] std::string SceneHudTimerText() const;
    [[nodiscard]] const std::array<std::uint32_t, 3>& SceneHudStatsWindows() const noexcept {
        return scene_hud_stats_windows_;
    }
    // Parent phase of 0x478A00, using 0x4BB300; does not change local visibility.
    [[nodiscard]] bool BindSceneHudParents(bool attach, std::uint32_t parent = 0u);
    [[nodiscard]] bool BindSceneHudParent(bool attach, std::uint32_t parent,
                                         bool minimap_component);
    [[nodiscard]] bool SetSceneHudControlVisible(std::uint32_t window, bool visible);
    // Original HUD input uses CEGUI Window::isVisible(false), including parents.
    [[nodiscard]] bool SceneHudActionButtonVisible(std::uint32_t window) const;
    [[nodiscard]] bool ActivateSceneHudLocalWindows();
    // Geometry phase of 0x478A00. Caller owns the preceding parent/visibility phase.
    [[nodiscard]] bool PositionSceneHudWindows(
        std::int32_t screen_width, std::int32_t screen_height, bool enabled);
    [[nodiscard]] bool HasSceneHudLayout() const noexcept {
        return scene_hud_ready_ && scene_hud_layout_ != nullptr &&
               scene_jump_hint_ != nullptr;
    }
    [[nodiscard]] bool SetSceneJumpHint(
        bool visible, const std::array<float, 2>& position);
    [[nodiscard]] std::array<float, 2> SceneScreenToUi(
        const std::array<float, 2>& position) const;
    [[nodiscard]] std::array<float, 2> UiToSceneScreen(
        const std::array<float, 2>& position) const;
    [[nodiscard]] bool LoadOriginalJumpHintProgressLayout();
    [[nodiscard]] bool LoadOriginalSceneHudHintLayout();
    [[nodiscard]] bool CloseSceneHudHint();
    [[nodiscard]] std::uint32_t SceneHudHintRootAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(scene_hud_hint_layout_);
    }
    [[nodiscard]] bool SetJumpHintProgressVisible(bool visible);
    [[nodiscard]] bool AdvanceJumpHintProgress(float delta);
    [[nodiscard]] bool JumpHintProgressVisible() const;
    [[nodiscard]] std::uint32_t JumpHintProgressAddress() const noexcept {
        return jump_hint_progress_[0] != 0u ?
            reinterpret_cast<std::uint32_t>(jump_hint_progress_.data()) : 0u;
    }
    [[nodiscard]] const NativeJumpHintProgressState& JumpHintProgressState() const noexcept {
        return jump_hint_progress_;
    }
    [[nodiscard]] bool LoadOriginalScenarioDialogLayout(
        const std::filesystem::path& game_root);
    [[nodiscard]] bool EnsureScenarioDialogLayout();
    [[nodiscard]] bool ShowScenarioDialogText(std::string_view game_text,
                                              std::uint32_t display_value);
    [[nodiscard]] bool AdvanceScenarioDialog();
    [[nodiscard]] bool SetScenarioDialogPortrait(std::string_view name,
                                                 bool left);
    [[nodiscard]] bool HideScenarioDialog();
    [[nodiscard]] bool ScenarioDialogVisible() const noexcept {
        return scenario_dialog_visible_;
    }
    [[nodiscard]] bool MinimapLayoutVisible() const noexcept {
        return minimap_layout_visible_;
    }
    // Shared activity stack is borrowed and must outlive this binding.
    void BindActiveHandlers(NativeEventHandlerArray& handlers);
    void UnbindActiveHandlers();
    [[nodiscard]] std::uint32_t ScenarioActiveComponentAddress() const noexcept;
    [[nodiscard]] std::uint32_t DispatchScenarioActiveCallback(
        std::uint32_t component, std::uint32_t slot);
    [[nodiscard]] std::uint8_t DispatchScenarioActivePredicate(
        std::uint32_t component, std::uint32_t slot, std::uint32_t event);
    [[nodiscard]] bool DispatchScenarioMouseUp(const void* event) noexcept;
    using KeyboardHandler = std::function<bool(std::uint32_t)>;
    // Borrows game services through callbacks; event addresses are valid only
    // during synchronous dispatch. DLL-owned subscribers outlive rebindings.
    [[nodiscard]] bool BindKeyboardHandlers(KeyboardHandler down, KeyboardHandler up);
    void ClearKeyboardHandlers() noexcept;
    // CEGUI subscriber ABI boundary: captures errors for rethrow after injection.
    [[nodiscard]] bool DispatchKeyboardEvent(bool down, const void* event) noexcept;
    // False also means an injected event was not consumed by a window.
    // Original4BF0E0 wrapper, including conditional right-release -> Esc.
    [[nodiscard]] bool ProcessFrameMessage(std::uint32_t manager, HWND window,
        std::uint32_t message, std::uint32_t wparam, std::uint32_t lparam);
    [[nodiscard]] bool ProcessKeyboardMessage(HWND window, std::uint32_t message,
        std::uint32_t wparam, std::uint32_t lparam);
    [[nodiscard]] bool ProcessMouseMessage(HWND window, std::uint32_t message,
                                            std::uint32_t wparam,
                                            std::uint32_t lparam);
    [[nodiscard]] int HoveredMainMenuButton() const;
    [[nodiscard]] bool HoveredLoadCloseButton() const;
    [[nodiscard]] int ClickedMainMenuButton() const noexcept {
        return clicked_main_menu_button_;
    }
    [[nodiscard]] int TakeMainMenuButtonClick() noexcept;
    void RecordMainMenuButtonClick(int button) noexcept;
    [[nodiscard]] bool TakeLoadCloseClick() noexcept;
    void RecordLoadCloseClick() noexcept;
    [[nodiscard]] bool TakeSaveCloseClick() noexcept;
    void RecordSaveCloseClick() noexcept;
    [[nodiscard]] int TakeSavePageClick() noexcept;
    void RecordSavePageClick(int page) noexcept;
    [[nodiscard]] int TakeLoadPageClick() noexcept;
    void RecordLoadPageClick(int page) noexcept;
    [[nodiscard]] int TakeLoadSlotClick() noexcept;
    void RecordLoadSlotClick() noexcept;
    [[nodiscard]] int TakeSaveSlotClick() noexcept;
    void RecordSaveSlotClick() noexcept;
    [[nodiscard]] int CurrentLoadPage() const noexcept { return load_page_; }
    [[nodiscard]] int CurrentSavePage() const noexcept { return save_page_; }
    [[nodiscard]] bool Started() const noexcept { return system_ready_; }
    [[nodiscard]] bool HasDesktop() const noexcept { return desktop_ != nullptr; }
    [[nodiscard]] bool HasMainLayout() const noexcept {
        return main_layout_ != nullptr;
    }
    [[nodiscard]] bool HasMainMenuButtons() const noexcept {
        return main_menu_buttons_ready_;
    }
    [[nodiscard]] bool HasLoadLayout() const noexcept {
        return load_layout_ != nullptr;
    }
    [[nodiscard]] bool HasSaveLayout() const noexcept {
        return save_layout_ != nullptr && save_close_button_ != nullptr &&
               save_list_ != nullptr;
    }
    [[nodiscard]] bool SaveLayoutVisible() const noexcept {
        return save_layout_visible_;
    }
    [[nodiscard]] std::uint32_t SaveFrameAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(save_layout_));
    }
    [[nodiscard]] bool HasLoadingLayout() const noexcept {
        return loading_layout_ != nullptr && loading_mask_ != nullptr &&
               loading_poser_ != nullptr && loading_geometry_ready_;
    }
    [[nodiscard]] bool HasMinimapLayout() const noexcept {
        return minimap_layout_ != nullptr && minimap_scene_text_ != nullptr &&
               minimap_image_ != nullptr && minimap_button_ != nullptr &&
               minimap_marker_button_ != nullptr && minimap_button_group_ != nullptr;
    }
    [[nodiscard]] bool HasScenarioDialogLayout() const noexcept {
        return scenario_layout_ready_ && scenario_layout_ != nullptr &&
               scenario_dialog_ != nullptr &&
               scenario_edit_ != nullptr && scenario_portrait_ != nullptr &&
               scenario_click_ != nullptr;
    }
    [[nodiscard]] std::uint32_t MinimapSceneTextAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(minimap_scene_text_));
    }
    [[nodiscard]] std::uint32_t MinimapImageAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(minimap_image_));
    }
    [[nodiscard]] std::uint32_t MinimapButtonAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(minimap_button_));
    }
    [[nodiscard]] std::uint32_t MinimapMarkerButtonAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(minimap_marker_button_);
    }
    [[nodiscard]] std::uint32_t MinimapButtonGroupAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(minimap_button_group_);
    }
    [[nodiscard]] std::uint32_t SceneJumpHintAddress() const noexcept {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(scene_jump_hint_));
    }
    [[nodiscard]] std::uint32_t SceneHudRootAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(scene_hud_layout_);
    }
    [[nodiscard]] std::uint32_t MinimapRootAddress() const noexcept {
        return reinterpret_cast<std::uint32_t>(minimap_layout_);
    }
    [[nodiscard]] std::uint32_t RenderCalls() const noexcept {
        return render_calls_;
    }
    // Full System::renderGUI submissions, excluding explicit overlay flushes.
    [[nodiscard]] std::uint32_t GuiRenderCalls() const noexcept { return gui_render_calls_; }
    [[nodiscard]] std::uint32_t QuadCalls() const noexcept {
        return quad_calls_;
    }
    [[nodiscard]] std::uint32_t ResourceLoads() const noexcept {
        return resources_.LoadCount();
    }
    [[nodiscard]] std::uint32_t TextureLoads() const noexcept {
        return textures_.LoadCount();
    }
    // Borrow the renderer's actual filename/CRC cache for save PNG invalidation.
    // The caller must release this borrow before the UI session shuts down.
    [[nodiscard]] RwTextureDictionaryState& FileTextureCache() {
        return textures_.FileTextureCache();
    }
    [[nodiscard]] std::uint32_t TextureCreates() const noexcept {
        return textures_.CreateCount();
    }
    [[nodiscard]] std::uint32_t TextureDestroys() const noexcept {
        return textures_.DestroyCount();
    }
    [[nodiscard]] std::uint32_t GlyphTextureCreates() const noexcept {
        return textures_.GlyphCreateCount();
    }
    [[nodiscard]] std::uint32_t GlyphTextureUpdates() const noexcept {
        return textures_.GlyphUpdateCount();
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    [[nodiscard]] bool SetScenarioDialogParent(bool attach);
    friend class NativeTutorialRuntime;
    friend class NativePoetryRuntime;
    friend class NativePuzzleWindow;
    friend class NativeHintRuntime;
    friend class NativeCombatHintsRuntime;
    friend class NativeCombatEndingElementsRuntime;
    std::array<std::uint32_t, 18> hint_frame_{};
    std::vector<std::uint8_t> hint_clock_;
    NativeGameTimerQueue* hint_timers_{};
    std::exception_ptr hint_error_;
    std::array<std::uint32_t, 12> tutorial_frame_{};
    std::exception_ptr tutorial_error_;
    friend class NativeSystemSettingFrame;
    friend class NativeSystemMenuDecorations;
    friend class NativeRoleStateWindow;
    friend class NativeSystemSettingOptionValues;
    friend class NativeSystemSettingAudio;
    friend class NativeSystemSettingOptionsNavigation;
    NativeEventHandlerArray* active_handlers_ = nullptr;
    // Native input prefix only; no original EXE vtable is invoked on it.
    std::array<std::uint32_t, 16> scenario_input_image_{};
    KeyboardHandler key_down_handler_;
    KeyboardHandler key_up_handler_;
    std::exception_ptr keyboard_error_;
    std::exception_ptr scenario_mouse_error_;
    std::size_t keyboard_subscription_count_ = 0u;
    [[nodiscard]] bool SaveListContainsMouse() const noexcept;
    void ReleaseLoadListItems() noexcept;
    void ReleaseSaveListItems() noexcept;
    void ReleaseSlotListItems(
        void* list, std::array<void*, 10>& items) noexcept;
    static void __fastcall RenderNative(void* renderer) noexcept;
    static void __fastcall AddQuad(void* renderer, void*, const void* destination,
                                   float depth, const void* texture,
                                   const void* uv, const void* colours,
                                   unsigned int split_mode) noexcept;
    static NativeCeguiUiSession* active_;
    HMODULE module_ = nullptr;
    HMODULE look_module_ = nullptr;
    CeguiRendererState renderer_{};
    CeguiSystemState mouse_transform_{};
    std::array<std::byte, 0x1b0> system_{};
    std::array<std::uintptr_t, 30> vtable_{};
    bool renderer_ready_ = false;
    bool system_ready_ = false;
    bool wide_mode_ = false;
    bool native_mode_ = false;
    void* desktop_ = nullptr;
    void* diagnostic_image_ = nullptr;
    void* main_layout_ = nullptr;
    void* load_layout_ = nullptr;
    void* save_layout_ = nullptr;
    void* save_close_button_ = nullptr;
    void* save_list_ = nullptr;
    std::array<void*, 10> save_page_buttons_{};
    std::array<void*, 10> save_list_items_{};
    int save_page_ = -1;
    bool save_layout_visible_ = false;
    bool save_list_mouse_pressed_ = false;
    void* loading_layout_ = nullptr;
    void* loading_mask_ = nullptr;
    void* loading_poser_ = nullptr;
    void* minimap_layout_ = nullptr;
    void* minimap_scene_text_ = nullptr;
    void* minimap_image_ = nullptr;
    void* minimap_button_ = nullptr;
    void* minimap_button_group_ = nullptr;
    void* minimap_marker_button_ = nullptr;
    void* scene_hud_layout_ = nullptr;
    void* system_toolbar_layout_ = nullptr;
    std::array<void*, 4> combat_layouts_{};
    std::array<std::uint32_t, 127> system_setting_frame_{};
    std::array<std::uint32_t, 7> system_toolbar_buttons_{};
    std::function<bool(std::uint32_t)> system_toolbar_click_handler_;
    std::exception_ptr system_toolbar_click_error_;
    std::size_t system_toolbar_subscription_count_ = 0u;
    void* scene_jump_hint_ = nullptr;
    NativeJumpHintProgressState jump_hint_progress_{};
    void* jump_hint_progress_layout_ = nullptr;
    void* scene_hud_hint_layout_ = nullptr;
    // HP mask, rage mask, system button; owned by the loaded portrait layout.
    std::array<std::uint32_t, 3> scene_hud_stats_windows_{};
    bool scene_hud_stats_flash_ = false;
    std::uint32_t scene_hud_run_button_ = 0u;
    std::uint32_t scene_hud_skill_button_ = 0u;
    std::uint32_t scene_hud_timer_text_ = 0u;
    bool scene_hud_ready_ = false;
    bool minimap_layout_visible_ = false;
    void* scenario_layout_ = nullptr;
    void* scenario_dialog_ = nullptr;
    void* scenario_edit_ = nullptr;
    void* scenario_portrait_ = nullptr;
    void* scenario_click_ = nullptr;
    bool scenario_layout_ready_ = false;
    bool scenario_dialog_visible_ = false;
    std::filesystem::path scenario_game_root_;
    std::map<std::string, ScenarioPortraitCacheEntry> scenario_portrait_cache_;
    float loading_mask_x_ = 0.0f;
    float loading_mask_width_ = 0.0f;
    float loading_poser_x_ = 0.0f;
    bool loading_geometry_ready_ = false;
    void* load_close_button_ = nullptr;
    void* load_list_ = nullptr;
    std::array<void*, 10> load_page_buttons_{};
    std::array<void*, 10> load_list_items_{};
    std::array<bool, 10> load_slot_present_{};
    std::array<void*, 3> main_menu_buttons_{};
    bool main_menu_buttons_ready_ = false;
    int clicked_main_menu_button_ = -1;
    std::array<int, 16> pending_main_menu_clicks_{};
    std::size_t pending_click_head_ = 0;
    std::size_t pending_click_count_ = 0;
    bool pending_load_close_click_ = false;
    bool pending_save_close_click_ = false;
    int pending_save_page_click_ = -1;
    int pending_load_page_click_ = -1;
    int pending_load_slot_click_ = -1;
    int pending_save_slot_click_ = -1;
    int load_page_ = -1;
    bool BindPackageAssets(const std::filesystem::path& game_root);
    NativeCeguiPackageResourceProvider resources_;
    std::unique_ptr<NativeSystemSettingSavePage> system_setting_save_page_;
    friend class NativeSystemSettingSaveNavigation;
    std::function<bool(std::uint32_t)> system_setting_page_click_handler_;
    std::function<bool(std::uint32_t)> system_setting_list_click_handler_;
    std::exception_ptr system_setting_page_click_error_;
    std::size_t system_setting_page_subscription_count_ = 0u;
    std::function<bool(std::uint32_t)> system_setting_main_click_handler_;
    std::exception_ptr system_setting_main_click_error_;
    std::size_t system_setting_main_subscription_count_ = 0u;
    std::function<bool(std::uint32_t)> system_setting_option_click_handler_;
    std::exception_ptr system_setting_option_click_error_;
    std::size_t system_setting_option_subscription_count_ = 0u;
    std::function<bool(std::uint32_t, bool)> system_setting_audio_event_handler_;
    std::exception_ptr system_setting_audio_event_error_;
    std::size_t system_setting_audio_subscription_count_ = 0u;
    std::unordered_set<std::string> loaded_imagesets_;
    NativeCeguiTextureBridge textures_;
    bool assets_bound_ = false;
    std::uint32_t render_calls_ = 0;
    std::uint32_t gui_render_calls_ = 0;
    std::uint32_t quad_calls_ = 0;
    std::uint32_t render_result_ = 0;
    bool quad_failed_ = false;
    bool gui_render_list_discarded_ = false;
    std::vector<CeguiRenderVertex> vertices_;
    std::vector<std::uint16_t> indices_;
    std::filesystem::path font_stage_dir_;
    std::filesystem::path game_root_;
    std::string last_error_;
};

}  // namespace pal4
