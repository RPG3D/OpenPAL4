#pragma once

#include "pal4/script_actor_event_processor.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatProtectionSelectionRuntime {
public:
    virtual ~ICombatProtectionSelectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Select(std::uint32_t source,std::uint32_t target) = 0;
};

class ICheckProtectTaskRuntime {
public:
    virtual ~ICheckProtectTaskRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual bool IsActorReady(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t OpenIterator(
        std::uint32_t collection, std::uint32_t virtual_offset) = 0;
    [[nodiscard]] virtual bool IteratorAtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t IteratorCurrent(
        std::uint32_t iterator) = 0;
    virtual void IteratorNext(std::uint32_t iterator) = 0;
    virtual void DeleteIterator(std::uint32_t iterator,
                                std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeContainer(std::uint32_t container) = 0;
    virtual void DestroyContainer(std::uint32_t container) = 0;
    virtual void AssignName(std::uint32_t name,
                            std::string_view value) = 0;
    virtual void CleanupName(std::uint32_t name) = 0;
};

class ICheckProtectPositionRuntime {
public:
    virtual ~ICheckProtectPositionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::array<float, 3> ActorChildPosition(
        std::uint32_t actor) = 0;
    virtual void NormalizeVector(std::array<float, 3>& vector) = 0;
    [[nodiscard]] virtual std::array<float, 6> ActorChildBoundingBox(
        std::uint32_t actor) = 0;
};

class CheckProtectTask final {
public:
    static constexpr std::uint32_t kSize = 60u;
    static constexpr std::uint32_t kVtable = 0x845900u;

    // IDA 0x563c10: construct a CheckProtect task for an eligible actor pair.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t owner, std::uint32_t actor,
        ICheckProtectTaskRuntime& runtime);

    // IDA 0x563f60: begin protection and enqueue its two child tasks.
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, class ICheckProtectStartRuntime& runtime,
        IScriptActorEventRuntime& actor_runtime);
    [[nodiscard]] static std::uint32_t Start(
        std::uint32_t task, class ICheckProtectStartRuntime& runtime,
        ICombatProtectionSelectionRuntime& selection);
    // IDA 0x564310: compute the guard position from actor child geometry.
    [[nodiscard]] static std::array<float, 3>* CalculatePosition(
        std::uint32_t task, std::array<float, 3>& output,
        ICheckProtectPositionRuntime& runtime);
};

struct CheckProtectSubtaskFactorySlot {
    std::uint32_t vtable = 0u;
};

class CheckProtectSubtaskFactory final {
public:
    static constexpr std::uint32_t kPositionVtable = 0x84591cu;
    static constexpr std::uint32_t kResponseVtable = 0x845920u;

    // IDA 0x564060/0x564070: initialize stack factory wrappers.
    [[nodiscard]] static CheckProtectSubtaskFactorySlot* InitializePosition(
        CheckProtectSubtaskFactorySlot* slot);
    [[nodiscard]] static CheckProtectSubtaskFactorySlot* InitializeResponse(
        CheckProtectSubtaskFactorySlot* slot);
};

class ICheckProtectStartRuntime : public ICheckProtectPositionRuntime {
public:
    virtual ~ICheckProtectStartRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void NotifySource(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::string_view GlobalText(
        std::uint32_t global_address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateChildTask(
        const CheckProtectSubtaskFactorySlot& factory) = 0;
    virtual void AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::string_view source,
        const std::array<std::uint32_t, 3>& timing) = 0;
};

}  // namespace pal4
