#pragma once

#include <cstdint>
#include <vector>

namespace pal4 {

struct SystemObjectPropertyRecord {
    float first;
    float value;
};
static_assert(sizeof(SystemObjectPropertyRecord) == 8u);

class SystemObjectProperty final {
public:
    // IDA 0x529700.
    [[nodiscard]] static float Get(const SystemObjectPropertyRecord& object);
};

class ISystemManagerArrayRuntime {
public:
    virtual ~ISystemManagerArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class ISystemManagerUpdateRuntime : public ISystemManagerArrayRuntime {
public:
    virtual ~ISystemManagerUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DialogDataFromParam(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t InternalObject(
        std::uint32_t parameter) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystemAddress() = 0;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    [[nodiscard]] virtual float ObjectRate(std::uint32_t object) = 0;
    [[nodiscard]] virtual float ObjectProperty(std::uint32_t object) = 0;
    virtual void SetObjectProperty(std::uint32_t object, float value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatState() = 0;
    virtual void SetCombatState(std::uint32_t state) = 0;
    virtual void TouchSkillSystem() = 0;
    virtual void TouchGlobalSystem() = 0;
    [[nodiscard]] virtual bool ObjectReady(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BeginActors(std::uint32_t group) = 0;
    [[nodiscard]] virtual bool IteratorFinished(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorAdvance(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint8_t ActorReady(std::uint32_t actor) = 0;
    virtual void ReleaseActorIterator(std::uint32_t iterator) = 0;
    // Original vtable slot0 (5287C0): pause +124, retain registered objects.
    virtual void PauseManager(std::uint32_t manager) = 0;
    virtual void StartObject(std::uint32_t object) = 0;
    virtual void InvokeManagerSlot4(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseEvents(
        const std::vector<std::uint32_t>& ready_objects) = 0;
};

class SystemManagerUpdate final {
public:
    // IDA 0x527ce0-0x527f00. Ready objects retain array-slot order.
    [[nodiscard]] static std::uint8_t Dispatch(
        std::uint32_t manager, float seconds,
        ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t OutOfCombat(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t InCombat(
        std::uint32_t manager, float seconds,
        ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static bool AdvanceObject(
        std::uint32_t object, float elapsed,
        ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static bool ProcessEvents(
        std::uint32_t manager, const std::vector<std::uint32_t>& ready_objects,
        ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t SwapBuffers(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint8_t IsActive(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static bool CanUpdate(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
    static void Update(std::uint32_t manager,
                       ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static bool AllObjectsReady(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveObject(
        std::uint32_t manager, std::uint32_t object,
        ISystemManagerArrayRuntime& runtime);
    [[nodiscard]] static std::uint8_t HandleDialogParam(
        std::uint32_t manager, std::uint32_t parameter,
        ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetCombatStateFromParam(
        std::uint32_t parameter, ISystemManagerUpdateRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdateCombatState(
        std::uint32_t manager, ISystemManagerUpdateRuntime& runtime);
};

}  // namespace pal4
