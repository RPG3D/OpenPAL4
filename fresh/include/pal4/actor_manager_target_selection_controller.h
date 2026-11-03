#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

enum class SelectionRecordKind : std::uint8_t {
    Magic,
    Stunt,
    Property,
};

// Construction/subscriptions and start/stop callbacks do not require the full
// candidate enumeration/database/input-frame interface.
class IActorManagerTargetSelectionMemoryRuntime {
public:
    virtual ~IActorManagerTargetSelectionMemoryRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};
class IActorManagerTargetModeRuntime : public virtual IActorManagerTargetSelectionMemoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t SelectionDialog(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindSelectionRecord(
        std::uint32_t database, std::uint32_t key,
        SelectionRecordKind kind) = 0;
    [[nodiscard]] virtual std::uint32_t RecordTargetType(
        std::uint32_t record, SelectionRecordKind kind) = 0;
    virtual void ReportSelectionError(std::string_view kind,
                                      std::uint32_t actor_name,
                                      std::uint32_t value) = 0;
    virtual void ReportTargetTypeError(bool known_type,
                                       std::uint32_t value) = 0;
    virtual void ReportInvalidTargetType(std::uint32_t type)=0;
};

class IActorManagerTargetSelectionBoundaryRuntime : public virtual IActorManagerTargetSelectionMemoryRuntime {
public:
    virtual ~IActorManagerTargetSelectionBoundaryRuntime() = default;
    virtual void InitializeSelectionData(std::uint32_t address,
                                         std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t EventKey(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t router, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    [[nodiscard]] virtual std::uint32_t DispatchSimple(
        std::uint32_t router, std::string_view event,
        std::uint8_t payload, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeInputState(
        std::uint32_t input) = 0;
    [[nodiscard]] virtual std::uint32_t SetTargetSelected(
        std::uint32_t target, std::uint32_t selected) = 0;
    virtual void UnsubscribeEvent(std::uint32_t router, std::uint32_t key,
                                  std::uint32_t handle) = 0;
    virtual std::uint32_t DetermineSelectionTargetMode(std::uint32_t controller) = 0;
    virtual std::uint32_t RefreshTargetSelection(std::uint32_t controller) = 0;
};

// Candidate refresh uses roster cursors and visual selection only. Keep the
// database, input-frame and projected-coordinate services in their own layers.
class IActorManagerTargetCandidatesRuntime : public virtual IActorManagerTargetSelectionMemoryRuntime {
public:
    virtual std::uint32_t SelectionDialog(std::uint32_t parameter)=0;
    virtual std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t selected)=0;
    virtual std::uint32_t CreateSelectionIterator(std::uint32_t actor,bool alternate)=0;
    virtual std::uint32_t CloneSelectionIterator(std::uint32_t temporary)=0;
    virtual void DeleteTemporaryIterator(std::uint32_t temporary,std::uint32_t flags)=0;
    virtual bool IteratorHasData(std::uint32_t iterator)=0;
    virtual bool IteratorAtEnd(std::uint32_t iterator)=0;
    virtual std::uint32_t IteratorValue(std::uint32_t iterator)=0;
    virtual void IteratorNext(std::uint32_t iterator)=0;
    virtual std::uint32_t DeleteIterator(std::uint32_t iterator,std::uint32_t flags)=0;
    virtual bool TargetStateNotThree(std::uint32_t actor)=0;
};

// Pointer picking uses roster ownership and SDK intersections, independently of
// database interpretation, keyboard state and legacy collection addresses.
class IActorManagerTargetPointerRuntime : public IActorManagerTargetCandidatesRuntime {
public:
    virtual std::uint32_t GameConfigWord(std::uint32_t index)=0;
    virtual std::uint32_t ActorRadius(std::uint32_t actor)=0;
    virtual std::uint32_t ActorManagerHelper(std::uint32_t context,std::uint32_t first,
        std::uint32_t second,std::uint32_t clump)=0;
    virtual double DistanceSquared(std::uint32_t atomic,std::uint32_t camera)=0;
    virtual std::uint32_t CreateTipsIterator(bool player)=0;
    virtual std::uint32_t EventRouter()=0;
    virtual std::uint32_t DispatchSimple(std::uint32_t router,std::string_view event,
        std::uint8_t payload,std::uint32_t mode)=0;
};

class IActorManagerTargetControlRuntime : public IActorManagerTargetSelectionBoundaryRuntime {
public:
    virtual std::uint32_t DispatchAddress(std::uint32_t router,std::string_view event,
        std::uint32_t payload,std::uint32_t mode)=0;
    virtual std::uint32_t DispatchWords(std::uint32_t router,std::string_view event,
        const std::array<std::uint32_t,4>& payload,std::uint32_t mode)=0;
};

// Required per-frame input services, independent of DB/projection storage.
class IActorManagerTargetInputRuntime : public virtual IActorManagerTargetSelectionMemoryRuntime {
public:
    virtual std::uint32_t CombatSystem()=0;
    virtual std::uint32_t CombatInactive(std::uint32_t)=0;
    virtual std::uint32_t CombatActive(std::uint32_t)=0;
    virtual std::uint32_t InputManager()=0;
    virtual std::uint32_t EventRouter()=0;
    virtual std::uint32_t DispatchAddress(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t)=0;
    virtual std::uint32_t DispatchWords(std::uint32_t,std::string_view,const std::array<std::uint32_t,4>&,std::uint32_t)=0;
    virtual void CheckTargetState(std::uint32_t)=0;
    virtual std::uint32_t StopInputSelection(std::uint32_t,const std::array<std::uint32_t,4>&)=0;
    virtual std::uint32_t FindInputClosest(std::uint32_t)=0;
};
class IActorManagerTargetHoverRuntime : public IActorManagerTargetPointerRuntime {
public:
    virtual std::uint32_t PreferredTipsIterator()=0;
    virtual std::uint32_t FallbackTipsIterator()=0;
    virtual std::uint32_t DispatchActor(std::uint32_t,std::string_view,std::uint32_t,std::uint32_t)=0;
};

class IActorManagerTargetSelectionRuntime : public IActorManagerTargetControlRuntime, public IActorManagerTargetModeRuntime, public IActorManagerTargetHoverRuntime, public IActorManagerTargetInputRuntime {
public:
    std::uint32_t CreateTipsIterator(bool player) override;
    std::uint32_t CombatSystem() override=0;
    std::uint32_t InputManager() override=0;
    std::uint32_t StopInputSelection(std::uint32_t,const std::array<std::uint32_t,4>&) override;
    std::uint32_t FindInputClosest(std::uint32_t) override;
    std::uint32_t PreferredTipsIterator() override;
    std::uint32_t FallbackTipsIterator() override;
    std::uint32_t EventRouter() override=0;
    std::uint32_t DispatchSimple(std::uint32_t,std::string_view,std::uint8_t,std::uint32_t) override=0;
    std::uint32_t SelectionDialog(std::uint32_t parameter) override=0;
    std::uint32_t SetTargetSelected(std::uint32_t actor,std::uint32_t selected) override=0;
    void ReportInvalidTargetType(std::uint32_t type) override;
    void UnsubscribeEvent(std::uint32_t router, std::uint32_t key,
                          std::uint32_t handle) override;
    std::uint32_t DetermineSelectionTargetMode(std::uint32_t controller) override;
    std::uint32_t RefreshTargetSelection(std::uint32_t controller) override;
    [[nodiscard]] virtual std::uint32_t FindEventGroup(
        std::uint32_t router, std::uint32_t key) = 0;
    virtual void ReleaseOwnedSlot(std::uint32_t slot) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t CombatInactive(
        std::uint32_t combat) = 0;
    [[nodiscard]] virtual std::uint32_t CombatActive(
        std::uint32_t combat) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchAddress(
        std::uint32_t router, std::string_view event,
        std::uint32_t payload, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchWords(
        std::uint32_t router, std::string_view event,
        const std::array<std::uint32_t, 4>& payload,
        std::uint32_t mode) = 0;
    virtual void CheckTargetState(std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchActor(
        std::uint32_t router, std::string_view event,
        std::uint32_t actor, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSelectionIterator(
        std::uint32_t dialog, bool alternate) = 0;
    [[nodiscard]] virtual std::uint32_t CloneSelectionIterator(
        std::uint32_t temporary) = 0;
    virtual void DeleteTemporaryIterator(std::uint32_t temporary,
                                         std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool IteratorHasData(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t GameConfigWord(
        std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ActorRadius(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManagerHelper(
        std::uint32_t context, std::uint32_t first,
        std::uint32_t second, std::uint32_t radius) = 0;
    [[nodiscard]] virtual double DistanceSquared(
        std::uint32_t projected, std::uint32_t reference) = 0;
    [[nodiscard]] virtual std::uint32_t ActorManagerSingleton() = 0;
    [[nodiscard]] virtual std::uint32_t PreferredIterator(
        std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t FallbackIterator(
        std::uint32_t player_collection) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorValue(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteIterator(
        std::uint32_t iterator, std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool TargetStateNotThree(
        std::uint32_t target) = 0;
};

class ActorManagerTargetSelectionController final {
public:
    static constexpr std::uint32_t kSize = 84u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t controller,
        IActorManagerTargetSelectionBoundaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t SubscribeEvents(
        std::uint32_t controller,
        IActorManagerTargetSelectionBoundaryRuntime& runtime);
    static void UnsubscribeEvents(
        std::uint32_t controller,
        IActorManagerTargetSelectionBoundaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetTargetPair(
        std::uint32_t controller,
        IActorManagerTargetSelectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetCommandStateFirst(
        std::uint32_t controller,
        IActorManagerTargetSelectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetCommandStateSecond(
        std::uint32_t controller,
        IActorManagerTargetSelectionMemoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessInput(
        std::uint32_t controller, float delta,
        IActorManagerTargetInputRuntime& runtime);
    [[nodiscard]] static std::uint32_t StartSelecting(
        std::uint32_t controller,
        const std::array<std::uint32_t, 4>& payload,
        IActorManagerTargetSelectionBoundaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t StopSelecting(
        std::uint32_t controller,
        const std::array<std::uint32_t, 4>& payload,
        IActorManagerTargetSelectionBoundaryRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateTarget(
        std::uint32_t controller, std::uint32_t argument,
        IActorManagerTargetPointerRuntime& runtime);
    [[nodiscard]] static std::uint32_t SwitchActiveActor(
        std::uint32_t controller, std::uint32_t actor,
        IActorManagerTargetCandidatesRuntime& runtime);
    [[nodiscard]] static bool CandidateAllowed(
        std::uint32_t controller, std::uint32_t actor,
        IActorManagerTargetCandidatesRuntime& runtime);
    [[nodiscard]] static std::uint32_t RefreshSelection(
        std::uint32_t controller,
        IActorManagerTargetCandidatesRuntime& runtime);
    [[nodiscard]] static std::uint32_t DetermineTargetMode(
        std::uint32_t controller,
        IActorManagerTargetModeRuntime& runtime);
    [[nodiscard]] static std::uint32_t MapTargetType(
        std::uint32_t type,
        IActorManagerTargetModeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindNearestCandidate(
        std::uint32_t controller, std::uint32_t coordinate_pair,
        IActorManagerTargetPointerRuntime& runtime);
    [[nodiscard]] static std::uint32_t CurrentActor(
        std::uint32_t controller,
        IActorManagerTargetSelectionMemoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t SwitchActiveActorThunk(
        std::uint32_t controller, std::uint32_t actor,
        IActorManagerTargetSelectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t DispatchSelectedTarget(
        std::uint32_t controller,
        IActorManagerTargetControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t RestartUiControl(
        std::uint32_t controller,
        IActorManagerTargetControlRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindClosestActor(
        std::uint32_t controller,
        IActorManagerTargetHoverRuntime& runtime);
};

}  // namespace pal4
