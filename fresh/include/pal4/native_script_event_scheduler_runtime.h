#pragma once

#include "pal4/script_event_processor.h"

namespace pal4 {

class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;
class IBehaviorAtomicDispatchRuntime;

// Borrows one object's document/task owners. Atomic task creation remains
// a required caller-supplied service; unknown condition callbacks fail.
class NativeScriptEventSchedulerRuntime final : public IScriptEventSchedulerRuntime {
public:
    NativeScriptEventSchedulerRuntime(NativeBaseObjectMemory& memory,
        NativeEventDocumentRuntime& documents, NativeScriptObjectRuntime& tasks,
        IBehaviorAtomicDispatchRuntime& atomics)
        : memory_(memory), documents_(documents), tasks_(tasks), atomics_(atomics) {}
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::int32_t Random() override;
    std::uint32_t GetConditionDocument(std::uint32_t event) override;
    std::uint32_t CallCondition(std::uint32_t callback, std::uint32_t document) override;
    void InsertCandidate(ScriptEventProcessorState& state,
                          std::uint32_t position, std::uint32_t event) override;
    std::uint32_t DispatchEvent(std::uint32_t event, std::uint32_t context,
        std::uint32_t argument_two, std::uint32_t argument_three) override;
    std::uint32_t UpdateContainer(std::uint32_t context, float elapsed) override;
private:
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    NativeScriptObjectRuntime& tasks_;
    IBehaviorAtomicDispatchRuntime& atomics_;
};

}  // namespace pal4
