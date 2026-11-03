#pragma once

#include <cstdint>
#include <string_view>
#include <span>

namespace pal4 {

// Creation needs no legacy vector/iterator storage or combat-outcome services.
class IActorManagerPlayerAddRuntime {
public:
    virtual ~IActorManagerPlayerAddRuntime()=default;
    virtual void ReportNullPlayer(std::string_view context,std::string_view message)=0;
    virtual std::uint32_t CreateActor(std::uint32_t player)=0;
    virtual std::string_view ActorBaseName(std::uint32_t actor)=0;
    virtual void AssignActorName(std::uint32_t actor,std::string_view name)=0;
    virtual void AppendActor(std::uint32_t collection,std::uint32_t actor)=0;
    virtual std::uint32_t PlayerActionState(std::uint32_t player)=0;
    virtual void SetActorState(std::uint32_t actor,std::uint32_t state)=0;
    virtual void DispatchNewActor(std::string_view event,std::uint32_t actor,std::uint32_t mode)=0;
};
class IActorManagerPlayerSyncRuntime {
public:
    virtual ~IActorManagerPlayerSyncRuntime()=default;
    virtual std::uint32_t ReadWord(std::uint32_t address)=0;
    virtual std::uint32_t PlayerManager()=0;
    virtual std::uint32_t FindPlayer(std::uint32_t manager,std::uint32_t index)=0;
    virtual void CopyActorToPlayer(std::uint32_t actor,std::uint32_t player)=0;
};
class IActorManagerPlayerCollectionRuntime : public IActorManagerPlayerAddRuntime,
    public IActorManagerPlayerSyncRuntime {
public:
    virtual ~IActorManagerPlayerCollectionRuntime() = default;
    std::uint32_t CreateActor(std::uint32_t player) override;
    void AppendActor(std::uint32_t collection,std::uint32_t actor) override;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void ReportNullPlayer(std::string_view context,
                                  std::string_view message) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructActor(
        std::uint32_t storage, std::uint32_t player) = 0;
    [[nodiscard]] virtual std::string_view ActorBaseName(
        std::uint32_t actor) = 0;
    virtual void AssignActorName(std::uint32_t actor,
                                 std::string_view name) = 0;
    virtual void InsertActor(std::uint32_t collection,
                             std::uint32_t position,
                             std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerActionState(
        std::uint32_t player) = 0;
    virtual void SetActorState(std::uint32_t actor,
                               std::uint32_t state) = 0;
    virtual void DispatchNewActor(std::string_view event,
                                  std::uint32_t actor,
                                  std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::uint32_t index) = 0;
    virtual void CopyActorToPlayer(std::uint32_t actor,
                                   std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t ActorValue20(
        std::uint32_t actor) = 0;
    virtual void CallChildSlot56(std::uint32_t actor,
                                 std::uint32_t argument) = 0;
    virtual void UpdateAnimationList(std::uint32_t list,
                                     std::uint32_t time_bits) = 0;
    virtual void ActorSlot388(std::uint32_t actor) = 0;
    virtual void LogRelease(std::uint32_t actor,
                            std::uint32_t name) = 0;
    virtual void ActorSlot248(std::uint32_t actor,
                              std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    virtual void RegisterEvent(std::uint32_t router,
                               std::uint32_t mode,
                               std::string_view event,
                               std::uint32_t owner,
                               std::uint32_t callback) = 0;
};

class ActorManagerPlayerCandidateIterator final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x844fc4u;
    static constexpr std::uint32_t kVtable = 0x84529cu;

    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t descriptor,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t FromFields(
        std::uint32_t position, std::uint32_t source,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Count(
        std::uint32_t iterator,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static bool AtBegin(
        std::uint32_t iterator,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static bool AtEnd(
        std::uint32_t iterator,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t iterator,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t iterator,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t iterator, std::uint8_t flags,
        IActorManagerPlayerCollectionRuntime& runtime);
};

class ActorManagerPlayerCollection final {
public:
    static constexpr std::uint32_t kSize = 32u;
    static constexpr std::uint32_t kEmptyName = 0x841964u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t collection,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t collection,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddPlayer(
        std::uint32_t collection, std::uint32_t player,
        std::uint32_t state,
        IActorManagerPlayerAddRuntime& runtime);
    [[nodiscard]] static std::uint32_t SyncPlayers(
        std::uint32_t collection,
        IActorManagerPlayerCollectionRuntime& runtime);
    // Host first-roster view must stay pinned for the synchronous callbacks.
    static void SyncPlayers(std::span<const std::uint32_t> actors,IActorManagerPlayerSyncRuntime&);
    [[nodiscard]] static std::uint32_t FirstCandidateIterator(
        std::uint32_t collection, std::uint32_t output,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SecondCandidateIterator(
        std::uint32_t collection, std::uint32_t output,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindSecondActor(
        std::uint32_t collection, std::uint32_t value,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateActors(
        std::uint32_t collection, std::uint32_t time_bits,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t collection,
        IActorManagerPlayerCollectionRuntime& runtime);
    static void ReleaseActor(
        std::uint32_t actor,
        IActorManagerPlayerCollectionRuntime& runtime);
    static void RegisterEventHandlers(
        std::uint32_t collection,
        IActorManagerPlayerCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnLeaveStage(
        std::uint32_t collection, std::uint32_t parameter,
        IActorManagerPlayerCollectionRuntime& runtime);
};

}  // namespace pal4
