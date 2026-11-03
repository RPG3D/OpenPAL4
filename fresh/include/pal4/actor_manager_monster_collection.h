#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerMonsterCollectionRuntime {
public:
    virtual ~IActorManagerMonsterCollectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateArray(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t DialogData(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    virtual void RegisterEvent(std::uint32_t router, std::uint32_t mode,
                               std::string_view name, std::uint32_t owner,
                               std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindMonsterRecord(
        std::uint32_t key) = 0;
    virtual void HandleMissingMonster(std::uint32_t key) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateActor(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructActor(
        std::uint32_t storage, std::uint32_t record) = 0;
    [[nodiscard]] virtual std::string_view ActorBaseName(
        std::uint32_t actor) = 0;
    virtual void AssignActorName(std::uint32_t actor,
                                 std::string_view name) = 0;
    virtual void ActorSlot24(std::uint32_t actor,
                             std::uint32_t argument) = 0;
    virtual void DispatchNewActor(std::uint32_t actor) = 0;
    virtual void ActorEvent268(std::uint32_t actor,
                               std::uint32_t event,
                               std::uint32_t payload) = 0;
    [[nodiscard]] virtual std::uint32_t CreateIterator(
        std::uint32_t list, std::uint32_t begin) = 0;
    [[nodiscard]] virtual std::uint32_t ActorProperty4(
        std::uint32_t actor) = 0;
    virtual void ActorSlot388(std::uint32_t actor) = 0;
    virtual void LogRelease(std::uint32_t actor,
                            std::uint32_t name) = 0;
    virtual void ActorSlot248(std::uint32_t actor,
                              std::uint32_t flags) = 0;
};

class ActorManagerMonsterCollection final {
public:
    static constexpr std::uint32_t kObjectSize = 32u;
    static constexpr std::uint32_t kEmptyName = 0x841964u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t manager, IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t manager, IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateMonster(
        std::uint32_t manager, std::uint32_t key,
        std::uint32_t state, std::uint32_t group,
        IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t FirstIterator(
        std::uint32_t manager, std::uint32_t output,
        IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t SecondIterator(
        std::uint32_t manager, std::uint32_t output,
        IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t AverageProperty4(
        std::uint32_t manager, IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t manager, IActorManagerMonsterCollectionRuntime& runtime);
    static void ReleaseMonster(
        std::uint32_t actor, IActorManagerMonsterCollectionRuntime& runtime);
    static void RegisterEventHandlers(
        std::uint32_t manager, IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnJoinStage(
        std::uint32_t manager, std::uint32_t parameter,
        IActorManagerMonsterCollectionRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t manager, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IActorManagerMonsterCollectionRuntime& runtime);
};

}  // namespace pal4
