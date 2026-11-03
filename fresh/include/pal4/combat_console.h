#pragma once

#include "pal4/event_subscription_set.h"
#include "pal4/timed_event_queue.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace pal4 {

struct CombatConsolePosition {
    float x = 0.0f;
    float y = 0.0f;
};

class ICombatConsoleMainWindowRuntime {
public:
    virtual ~ICombatConsoleMainWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindowByName(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t console, std::string_view name) = 0;
    virtual void SetAutoBreak(std::uint32_t edit, bool enabled) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view source) = 0;
    virtual void SetChildVisible(std::uint32_t child, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t MouseClickEvent() = 0;
    virtual void SubscribeClick(std::uint32_t root, std::uint32_t event,
                                std::uint32_t& callback) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
};

class ICombatConsoleConstructionRuntime : public ICombatConsoleMainWindowRuntime {
public:
    ~ICombatConsoleConstructionRuntime() override = default;
    virtual void LoadLayout(std::uint32_t console, std::string_view name,
                            std::uint32_t first, std::uint32_t second) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetTextErrorFlag(std::uint32_t record,
                                  std::uint8_t flag) = 0;
    virtual void SetText(std::uint32_t record, std::string_view text) = 0;
    [[nodiscard]] virtual CombatConsolePosition GetPosition(
        std::uint32_t window, std::uint32_t mode) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window,
                                         std::uint32_t mode) = 0;
    virtual void AppendLegacyText(std::uint32_t array,
                                  std::string_view text) = 0;
    virtual void AppendArrayText(std::uint32_t array,
                                 std::string_view text) = 0;
};

// Historical destruction boundary retained for existing callers. Native owners
// keep strings/vectors in host STL and retire them through normal C++ lifetime.
class ICombatConsoleRuntime : public ICombatConsoleConstructionRuntime {
public:
    ~ICombatConsoleRuntime() override = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBase(std::uint32_t console) = 0;
};

class ICombatConsoleVisibilityServices {
public:
    virtual ~ICombatConsoleVisibilityServices() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual void RegisterGameEvents(std::uint32_t owner) = 0;
    virtual void CleanupGameEvents(std::uint32_t owner) = 0;
    virtual void SetParent(std::uint32_t console, bool visible,
                           std::uint32_t parent) = 0;
    virtual void SetChildVisible(std::uint32_t child, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t AddHandler(
        std::uint32_t handlers, std::uint32_t handler) = 0;
    virtual void RemoveHandler(std::uint32_t handlers,
                               std::uint32_t handler) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveTimer(
        std::uint32_t timers, std::uint32_t name) = 0;
};

// Compatibility adapter for shared callers with the historical router image.
// Native owners implement the narrow services above with their host router.
class ICombatConsoleVisibilityRuntime : public ICombatConsoleVisibilityServices,
                                        public IEventSubscriptionRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override = 0;
    void WriteWord(std::uint32_t address,std::uint32_t value) override = 0;
    void RegisterGameEvents(std::uint32_t owner) override;
    void CleanupGameEvents(std::uint32_t owner) override;
};

class ICombatConsoleImageRuntime {
public:
    virtual ~ICombatConsoleImageRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetChildVisible(std::uint32_t child, bool visible) = 0;
    [[nodiscard]] virtual std::string ImageSetName() = 0;
    virtual void SetImage(std::uint32_t image, std::u32string_view image_set,
                          std::u32string_view image_name) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t image, std::uint32_t mode,
        std::uint32_t rectangle) = 0;
};

class ICombatConsoleAnimationRuntime {
public:
    virtual ~ICombatConsoleAnimationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t image, std::uint32_t mode, float x, float y) = 0;
};

class ICombatConsoleActorIterator {
public:
    virtual ~ICombatConsoleActorIterator() = default;
    [[nodiscard]] virtual bool AtEnd() = 0;
    [[nodiscard]] virtual std::uint32_t Current() = 0;
    virtual void Advance() = 0;
};

class ICombatConsoleSceneRuntime : public ICombatConsoleImageRuntime,
                                   public ICombatConsoleAnimationRuntime {
public:
    virtual ~ICombatConsoleSceneRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameState() override = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromEvent(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::int32_t ActorIndex(std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ActorPositionProgress(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorState(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string ActorImageName(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleActorIterator>
        RoleActors() = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleActorIterator>
        MonsterActors() = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleActorIterator>
        CountMonsterActors() = 0;
    [[nodiscard]] virtual bool ActorStateZero(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorReady(std::uint32_t actor) = 0;
    virtual void SetCombatCountText(std::uint32_t window,
                                    std::string_view text) = 0;
};

class ICombatConsoleJointHintRuntime {
public:
    virtual ~ICombatConsoleJointHintRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromEvent(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
};
class ICombatConsoleResultRuntime : public ICombatConsoleJointHintRuntime {
public:
    [[nodiscard]] virtual bool IsVisible(std::uint32_t window,
                                          bool recursive) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromEvent(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t CreateCombatAction(
        std::uint32_t actor, std::uint32_t value, std::uint32_t mode) = 0;
    virtual void RegisterCombatAction(std::uint32_t manager,
                                      std::uint32_t action) = 0;
    [[nodiscard]] virtual std::uint32_t CompleteCombatResult(
        std::uint32_t actor, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t DispatchSelectJoinAttack(
        std::uint32_t actor, std::uint32_t value,
        std::uint32_t mode) = 0;
};

class ICombatConsoleAlphaRuntime {
public:
    virtual ~ICombatConsoleAlphaRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void SetAlpha(std::uint32_t window, float alpha) = 0;
};

class ICombatConsoleDialogIdIterator {
public:
    virtual ~ICombatConsoleDialogIdIterator() = default;
    [[nodiscard]] virtual bool AtEnd() = 0;
    [[nodiscard]] virtual std::int32_t CurrentId() = 0;
    virtual void Advance() = 0;
};

class ICombatConsoleDialogRuntime {
public:
    virtual ~ICombatConsoleDialogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromEvent(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool IsCombatEnding(std::uint32_t system) = 0;
    [[nodiscard]] virtual CombatConsolePosition GameConfigPosition() = 0;
    [[nodiscard]] virtual CombatConsolePosition FrameCoordinates(
        CombatConsolePosition position) = 0;
    [[nodiscard]] virtual CombatConsolePosition ScreenToWindow(
        std::uint32_t parent, CombatConsolePosition position) = 0;
    [[nodiscard]] virtual CombatConsolePosition AbsoluteToRelative(
        std::uint32_t parent, CombatConsolePosition position) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCombatCamera() = 0;
    [[nodiscard]] virtual CombatConsolePosition ActorScreenPosition(
        std::uint32_t actor, std::uint32_t camera) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetPosition(std::uint32_t window,
                             CombatConsolePosition position) = 0;
    [[nodiscard]] virtual std::string ReadDialogText(
        std::uint32_t actor_text_record) = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleDialogIdIterator>
        CharacterNameIds(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleDialogIdIterator>
        SpeakerNameIds(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string ReadTextRecord(
        std::uint32_t record) = 0;
    // Native Main stores markup in host STL; historical callers retain this
    // default lookup through the original console296/300 text vector.
    [[nodiscard]] virtual std::string ConsoleStatusMarkup(std::uint32_t console,std::int32_t id);
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    [[nodiscard]] virtual float GetTextHeight(
        std::uint32_t edit) = 0;
    [[nodiscard]] virtual float GetMaxLineWidth(
        std::uint32_t edit) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void SetHeight(std::uint32_t window,
                           std::uint32_t mode, float value) = 0;
    virtual void SetWidth(std::uint32_t window,
                          std::uint32_t mode, float value) = 0;
    [[nodiscard]] virtual std::int32_t DialogTotal(
        std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t DialogCurrentIndex(
        std::uint32_t actor, std::uint32_t mode,
        std::int32_t total) = 0;
    [[nodiscard]] virtual float GetXPosition(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    virtual void SetXPosition(std::uint32_t window, float x) = 0;
};

class ICombatConsolePanelRuntime {
public:
    virtual ~ICombatConsolePanelRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t ActorFromEvent(std::uint32_t)=0;
    virtual std::int32_t ActorIndex(std::uint32_t)=0;
    virtual void SetVisible(std::uint32_t,bool)=0;
    virtual void SetAlpha(std::uint32_t,float)=0;
};
class ICombatConsoleActionRuntime {
public:
    virtual ~ICombatConsoleActionRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t)=0;
    virtual std::uint32_t Allocate(std::uint32_t)=0;
    virtual void QueueRefreshTimer(std::uint32_t console)=0;
    [[nodiscard]] virtual std::uint32_t ActorFromEvent(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::int32_t ActorIndex(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string ActorImageName(
        std::uint32_t actor) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructVisualElement(
        std::uint32_t memory) = 0;
    virtual void SetElementPosition(std::uint32_t element, float x,
                                    float y) = 0;
    virtual void SetElementImage(std::uint32_t element,
                                 std::string_view imageset,
                                 std::string_view image, float scale) = 0;
    virtual void SetElementFont(std::uint32_t element, float x,
                                float y) = 0;
    virtual void SetElementMotion(std::uint32_t element, float x,
                                  float y, float width, float height) = 0;
    virtual void AddElement(std::uint32_t element) = 0;
    [[nodiscard]] virtual CombatConsolePosition TransformFrameCoordinates(
        CombatConsolePosition position) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCombatCamera() = 0;
    [[nodiscard]] virtual std::array<float, 3> TransformEffectPosition(
        std::uint32_t camera, CombatConsolePosition position,
        float depth) = 0;
    [[nodiscard]] virtual std::uint32_t CreateEffect(
        std::string_view name, const std::array<float, 3>& position) = 0;
    [[nodiscard]] virtual bool EffectNeedsUpdate(
        std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateEffectProperties(
        std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::uint32_t CombatCameraFrame() = 0;
    // Historical fixture default; native action consumers call the SDK explicitly.
    virtual void RwFrameAddChild(std::uint32_t,
                                 std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t ResetEffect(
        std::uint32_t effect) = 0;
};

// Compatibility adapter for the historical event-list image. Native action
// consumers queue through NativeGameTimerQueue instead of emulating old strings.
class ICombatConsoleDialogEventRuntime : public ITimedEventQueueRuntime,
                                        public ICombatConsolePanelRuntime,
                                        public ICombatConsoleActionRuntime {
public:
    ~ICombatConsoleDialogEventRuntime() override = default;
    std::uint32_t ReadWord(std::uint32_t) override=0;
    std::uint32_t Allocate(std::uint32_t) override=0;
    void WriteWord(std::uint32_t,std::uint32_t) override=0;
    std::uint32_t ActorFromEvent(std::uint32_t) override=0;
    std::int32_t ActorIndex(std::uint32_t) override=0;
    void SetVisible(std::uint32_t,bool) override=0;
    void QueueRefreshTimer(std::uint32_t console) override;
    virtual std::uint32_t UiFrameManager()=0;
};

class ICombatConsoleEventRuntime {
public:
    virtual ~ICombatConsoleEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void DispatchInputEvent(std::string_view name,
                                    std::uint32_t event,
                                    std::uint32_t mode) = 0;
    virtual void DispatchActorEvent(std::string_view name,
                                    std::uint32_t actor,
                                    std::uint32_t mode) = 0;
    [[nodiscard]] virtual ICombatConsoleResultRuntime& ResultRuntime() = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleActorIterator>
        RoleActors() = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    virtual void StartCombat(std::uint32_t system) = 0;
    virtual void ConfirmCombat(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentTarget(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::unique_ptr<ICombatConsoleActorIterator>
        TargetCandidates(std::uint32_t target) = 0;
    [[nodiscard]] virtual bool IsValidTarget(
        std::uint32_t system, std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorOrder(
        std::uint32_t actor) = 0;
    virtual void SelectTarget(std::uint32_t system,
                              std::uint32_t actor) = 0;
};

class CombatConsoleWindow final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8435a4u;
    static constexpr std::uint32_t kVtable = 0x84551cu;
    static constexpr std::uint32_t kFieldVtable = 0x845504u;

    // IDA CombatConsoleWindow_Construct at 0x54b4e0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t console, std::string_view layout,
        ICombatConsoleConstructionRuntime& runtime);
    static constexpr std::uint32_t kClickCallbackVtable = 0x845534u;
    static constexpr std::uint32_t kClickCallback = 0x54ce40u;
    // IDA CombatConsoleWindow_InitializeMainWindow at 0x54c810.
    [[nodiscard]] static std::uint32_t InitializeMainWindow(
        std::uint32_t console, ICombatConsoleMainWindowRuntime& runtime);
    // IDA CombatConsoleWindow_Destruct at 0x54c110.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t console, ICombatConsoleRuntime& runtime);
    // IDA CombatConsoleWindow_SetVisible at 0x54c1e0.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t console, bool visible, std::uint32_t parent,
        ICombatConsoleVisibilityServices& runtime);
    // IDA CombatConsoleWindow_SetImageAndPosition at 0x54c290.
    [[nodiscard]] static std::uint32_t SetImageAndPosition(
        std::uint32_t console, std::int32_t index,
        std::string_view image_name,
        ICombatConsoleImageRuntime& runtime);
    // IDA CombatConsoleWindow_HideElement at 0x54c4b0.
    [[nodiscard]] static std::uint32_t HideElement(
        std::uint32_t console, std::int32_t index,
        ICombatConsoleImageRuntime& runtime);
    // IDA CombatConsoleWindow_SetImageAndPositionSecond at 0x54c4f0.
    [[nodiscard]] static std::uint32_t SetImageAndPositionSecond(
        std::uint32_t console, std::int32_t index,
        std::string_view image_name,
        ICombatConsoleImageRuntime& runtime);
    // IDA CombatConsoleWindow_HideElementSecond at 0x54c710.
    [[nodiscard]] static std::uint32_t HideElementSecond(
        std::uint32_t console, std::int32_t index,
        ICombatConsoleImageRuntime& runtime);
    // IDA CombatConsoleWindow_AnimatePosition at 0x54c750.
    [[nodiscard]] static std::uint32_t AnimatePosition(
        std::uint32_t console, std::int32_t index, float progress,
        ICombatConsoleAnimationRuntime& runtime);
    // IDA CombatConsoleWindow_AnimatePositionSecond at 0x54c7b0.
    [[nodiscard]] static std::uint32_t AnimatePositionSecond(
        std::uint32_t console, std::int32_t index, float progress,
        ICombatConsoleAnimationRuntime& runtime);
    // IDA 0x54c9e0/0x54cac0: event thunk and full actor refresh.
    static void RefreshFromEvent(std::uint32_t console, std::uint32_t event,
                                 ICombatConsoleSceneRuntime& runtime);
    static void Refresh(std::uint32_t console,
                        ICombatConsoleSceneRuntime& runtime);
    // IDA 0x54c9f0/0x54ca30: event-to-animation adapters.
    [[nodiscard]] static std::uint32_t AnimateRoleEvent(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleSceneRuntime& runtime);
    [[nodiscard]] static std::uint32_t AnimateMonsterEvent(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleSceneRuntime& runtime);
    // IDA 0x54ca70/0x54ca80: event thunk and visibility reset.
    static void HideAllFromEvent(std::uint32_t console, std::uint32_t event,
                                 ICombatConsoleImageRuntime& runtime);
    static void HideAll(std::uint32_t console,
                        ICombatConsoleImageRuntime& runtime);
    // IDA 0x54ce50: count actors in the monster first collection.
    static void UpdateCombatCount(std::uint32_t console,
                                  ICombatConsoleSceneRuntime& runtime);
    // IDA 0x54cc60/0x54ccb0: result panel lifecycle.
    [[nodiscard]] static std::uint32_t ShowCombatResult(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleJointHintRuntime& runtime);
    [[nodiscard]] static std::uint32_t HideCombatResult(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleResultRuntime& runtime);
    // IDA 0x54cd50/0x54ce40: click-driven result dispatch.
    [[nodiscard]] static std::uint8_t DispatchVisibleCombatResult(
        std::uint32_t console, ICombatConsoleResultRuntime& runtime);
    [[nodiscard]] static bool OnCombatResultClick(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleResultRuntime& runtime);
    // IDA 0x54cda0: reflect the alpha at the bounds.
    static void UpdateAlpha(std::uint32_t console, double elapsed,
                            ICombatConsoleAlphaRuntime& runtime);
    // IDA ui_displayScenarioDialog at 0x54cfd0.
    [[nodiscard]] static bool DisplayDialog(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleDialogRuntime& runtime);
    // IDA 0x54e220: fit the edit control and its parent to formatted text.
    static void UpdateDialogLayout(
        std::uint32_t console, ICombatConsoleDialogRuntime& runtime);
    // IDA 0x54d670: combat-dialog avatar, effect and refresh timer.
    [[nodiscard]] static std::uint32_t HandleCombatDialog(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleActionRuntime& runtime);
    // IDA 0x54d980/0x54d9d0/0x54da00: dialog target and panel state.
    [[nodiscard]] static std::uint32_t SetDialogTargetWindow(
        std::uint32_t console, std::uint32_t event,
        ICombatConsolePanelRuntime& runtime);
    static void ClearDialogTargetWindow(
        std::uint32_t console, std::uint32_t event,
        ICombatConsolePanelRuntime& runtime);
    static void HideDialogPanel(std::uint32_t console,
                                std::uint32_t event,
                                ICombatConsolePanelRuntime& runtime);
    // IDA 0x54da20: the embedded +40 frame handler, expressed on its owner.
    [[nodiscard]] static bool ProcessCombatEvent(
        std::uint32_t console, std::uint32_t event,
        ICombatConsoleEventRuntime& runtime);
};

}  // namespace pal4
