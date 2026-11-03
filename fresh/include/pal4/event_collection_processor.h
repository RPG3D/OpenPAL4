#pragma once

#include "pal4/collection_iterator_lifecycle.h"

#include <cstdint>

namespace pal4 {

struct EventOutputState {
    std::uint32_t tag{};
    std::uint32_t begin{};
    std::uint32_t end{};
    std::uint32_t capacity{};
};
static_assert(sizeof(EventOutputState) == 16u);

struct EventVisitorState {
    std::uint32_t vtable{0x8498a8u};
    std::uint32_t output{};
};
static_assert(sizeof(EventVisitorState) == 8u);

class IEventCollectionRuntime : public ICollectionIteratorLifecycleRuntime {
public:
    [[nodiscard]] virtual bool AtEnd(std::uint32_t iterator,
                                     std::uint32_t vtable_offset) = 0;
    virtual void Advance(std::uint32_t iterator,
                         std::uint32_t vtable_offset) = 0;
    virtual void Visit(std::uint32_t element, EventVisitorState& visitor,
                       std::uint32_t vtable_offset) = 0;
    virtual void Append(EventOutputState& output, std::uint32_t value) = 0;
    virtual void IncrementTempOff() = 0;
};

class IEventCollectionBindRuntime : public IEventCollectionRuntime {
public:
    virtual void BindEffect(std::uint32_t target,
                            std::uint32_t effect) = 0;
};

class EventCollectionRuntimeStub final : public IEventCollectionRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void Free(std::uint32_t address) override;
    bool AtEnd(std::uint32_t iterator,
               std::uint32_t vtable_offset) override;
    void Advance(std::uint32_t iterator,
                 std::uint32_t vtable_offset) override;
    void Visit(std::uint32_t element, EventVisitorState& visitor,
               std::uint32_t vtable_offset) override;
    void Append(EventOutputState& output, std::uint32_t value) override;
    void IncrementTempOff() override;
};

class EventCollectionProcessor final {
public:
    // IDA 0x5ef2a0.
    [[nodiscard]] static std::uint32_t Process(
        EventOutputState& output, std::uint32_t collection,
        IEventCollectionRuntime& runtime);
    // IDA 0x5ef9d0.
    [[nodiscard]] static std::uint8_t ProcessAndBind(
        std::uint32_t target, std::uint32_t collection,
        IEventCollectionBindRuntime& runtime);
};

}  // namespace pal4
