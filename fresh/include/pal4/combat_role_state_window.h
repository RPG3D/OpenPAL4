#pragma once

#include "pal4/combat_role_state_registration.h"
#include "pal4/combat_role_state_elements.h"
#include "pal4/combat_role_state_display.h"
#include "pal4/timer_queue_task.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatRoleStateWindowRuntime : public ICombatRoleStateElementsRuntime,
                                      public ICombatRoleStateVisibilityRuntime,
                                      public IEventSubscriptionRuntime,
                                      public ITimerQueueTaskRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) override = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) override = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) override = 0;
    virtual void LoadLayout(std::uint32_t window,
                            std::uint32_t layout_name) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view image) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindowByName(
        std::uint32_t window, const std::string& utf8_name) = 0;
    virtual void SetFont(std::uint32_t control,
                         std::string_view font) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t control) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClick(
        std::uint32_t control, CombatRoleStateClickEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryString() = 0;
    virtual void SetParent(std::uint32_t window, bool visible,
                           std::uint32_t parent) = 0;
    virtual void SetImage(std::uint32_t control, std::uint32_t image) = 0;
    virtual void SetWidth(std::uint32_t control, float width) = 0;
    virtual void SetText(std::uint32_t control,
                         std::string_view text) = 0;
    virtual void SetControlVisible(std::uint32_t control, bool visible) = 0;
    virtual void SetControlEnabled(std::uint32_t control, bool enabled) = 0;
    // Legacy name retained for existing runtimes;575490 actually gates states4/5.
    [[nodiscard]] virtual bool IsCombatStateEnding() = 0;
    bool IsCommandOrTargetSelection() override { return IsCombatStateEnding(); }
    [[nodiscard]] virtual std::uint32_t OpenCandidateIterator() = 0;
    [[nodiscard]] virtual std::uint32_t OpenActiveRoleIterator() = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentActor(
        std::uint32_t iterator) = 0;
    virtual void AdvanceIterator(std::uint32_t iterator) = 0;
    virtual void ReleaseIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseActiveRoleIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual bool ActorCheckState3(std::uint32_t actor) = 0;
    [[nodiscard]] virtual bool ActorIsStateZero(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorActionCount(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorStat(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::int32_t ActorStatMaximum(
        std::uint32_t actor, std::uint32_t stat) = 0;
    [[nodiscard]] virtual std::uint32_t GetIconImage(
        std::uint32_t portrait, std::uint32_t category,
        std::uint32_t variant) = 0;
    virtual void SetBackgroundImage(std::uint32_t control,
                                    std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t OpenStatusIterator(
        std::uint32_t actor, bool secondary) = 0;
    [[nodiscard]] virtual bool StatusIteratorAtEnd(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::int32_t StatusValue(
        std::uint32_t iterator) = 0;
    virtual void AdvanceStatusIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseStatusIterator(
        std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t GetDialogDataFromParam(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManager() = 0;
    std::int32_t PlayerRosterCount() override;
    std::uint8_t StartColourTimer(std::uint32_t) override;
    std::uint8_t StopColourTimer(std::uint32_t) override;
    [[nodiscard]] virtual std::uint32_t ActorAtRoleSlot(
        std::uint32_t slot) = 0;
    [[nodiscard]] virtual std::int32_t ActorStatusKind(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::int32_t ActorElement(
        std::uint32_t actor) = 0;
    virtual void SetImageColours(
        std::uint32_t control,
        const std::array<std::uint32_t, 4>& rgba_bits) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t CreateName(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupName(
        std::uint32_t name) = 0;
    virtual void RemoveTimerByName(std::uint32_t queue,
                                   std::uint32_t name) = 0;
};

class CombatRoleStateWindow final {
public:
    static constexpr std::uint32_t kObjectSize = 0x20cu;
    static constexpr std::uint32_t kVtable = 0x845590u;
    static constexpr std::uint32_t kTimerCallbackVtable = 0x8455a8u;
    static constexpr std::uint32_t kRefreshCallback = 0x551cb0u;
    static constexpr std::uint32_t kTimerPeriodBits = 0x3f000000u;
    static constexpr std::uint32_t kClickCallbackVtable = 0x8455b0u;
    static constexpr std::uint32_t kButtonCallback = 0x551c70u;
    static constexpr std::uint32_t kMouseCallback = 0x551c30u;

    // IDA 0x54f5e0 and 0x54f660.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::uint32_t layout_name,
        ICombatRoleStateWindowRuntime& runtime);
    // IDA 0x54f940.
    [[nodiscard]] static std::uint32_t InitializeControls(
        std::uint32_t window, ICombatRoleStateElementsRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetVisible(
        std::uint32_t window, bool visible, std::uint32_t parent,
        ICombatRoleStateWindowRuntime& runtime);
    static std::uint8_t SetVisibility(std::uint32_t,bool,std::uint32_t,ICombatRoleStateVisibilityRuntime&);
    // IDA 0x550f80.
    static void UpdateDisplay(std::uint32_t window, std::int32_t index,
                              ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551250.
    [[nodiscard]] static std::int32_t UpdateCharacterDisplay(
        std::uint32_t window, std::uint32_t actor,
        ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551700.
    [[nodiscard]] static std::uint8_t UpdateCharacterImages(
        std::uint32_t window, std::uint32_t actor,
        ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x5519d0.
    [[nodiscard]] static std::uint32_t UpdateAllCharacterImages(
        std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551a70.
    [[nodiscard]] static std::uint8_t OnCharacterImageEvent(
        std::uint32_t window, std::uint32_t parameter,
        ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x5518a0 and 0x5518c0.
    [[nodiscard]] static std::int32_t OnCharacterDisplayEvent(
        std::uint32_t window, std::uint32_t parameter,
        ICombatRoleStateDisplayRuntime& runtime);
    static void OnRoleDisplayEvent(
        std::uint32_t window, std::uint32_t parameter,
        ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x5518e0.
    static void RefreshSubstitutionButtons(
        std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551ab0 and 0x551ae0.
    static void DisableSubstitutionButtons(
        std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime);
    [[nodiscard]] static std::uint32_t RefreshCombatantVisibility(
        std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551be0.
    [[nodiscard]] static std::int32_t OnCombatantStatEvent(
        std::uint32_t window, std::uint32_t parameter,
        ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x551cb0.
    [[nodiscard]] static std::uint8_t RefreshCombatantColours(
        std::uint32_t window, ICombatRoleStateDisplayRuntime& runtime);
    // IDA 0x54f820.
    static void AddTimer(std::uint32_t queue, std::uint32_t name,
                         std::uint32_t period_bits, std::uint32_t callback,
                         ICombatRoleStateWindowRuntime& runtime);
};

}  // namespace pal4
