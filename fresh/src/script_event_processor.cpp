#include "pal4/script_event_processor.h"

#include <bit>
#include <cstring>

namespace pal4 {
namespace {

void ReleaseName(StringBufferState& name, IScriptEventInsertRuntime& runtime) {
    if (name.data != 0u) {
        const auto header = name.data - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                static_cast<std::uint8_t>(reference - 1u));
    }
    name = {};
}

}  // namespace

std::uint32_t ScriptEventProcessorRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventProcessorRuntimeStub::InitializeDocument(
    std::uint32_t address, const char*, std::uint32_t) {
    return address;
}
void ScriptEventProcessorRuntimeStub::CleanupDocument(std::uint32_t) {}
void ScriptEventProcessorRuntimeStub::DestroyDocument(std::uint32_t) {}
std::uint32_t ScriptEventProcessorRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void ScriptEventProcessorRuntimeStub::CleanupEventData(std::uint32_t) {}
std::uint32_t ScriptEventProcessorRuntimeStub::Free(std::uint32_t) {
    return 0u;
}
bool ScriptEventInsertRuntimeStub::AllocateText(
    StringBufferState&, std::uint32_t, std::uint8_t) { return false; }
void ScriptEventInsertRuntimeStub::CopyText(
    std::uint32_t, const char*, std::uint32_t) {}
void ScriptEventInsertRuntimeStub::SetTextSize(
    StringBufferState& state, std::uint32_t length) { state.length = length; }
void ScriptEventInsertRuntimeStub::AssignName(
    StringBufferState& destination, const StringBufferState& source) {
    destination = source;
}
std::uint32_t ScriptEventInsertRuntimeStub::EmptyStringAddress() {
    return 0u;
}
std::int32_t ScriptEventInsertRuntimeStub::CompareEventString(
    std::uint32_t, std::uint32_t) { return 1; }
std::uint32_t ScriptEventInsertRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventInsertRuntimeStub::InitializeEventData(
    std::uint32_t address) { return address; }
void ScriptEventInsertRuntimeStub::AssignEventCString(
    std::uint32_t, const char*) {}
void ScriptEventInsertRuntimeStub::InsertEventPointer(
    ScriptEventProcessorState&, std::uint32_t, std::uint32_t) {}
std::uint32_t ScriptEventInsertRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint8_t ScriptEventInsertRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void ScriptEventInsertRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void ScriptEventInsertRuntimeStub::Free(std::uint32_t) {}
std::uint8_t ScriptEventSchedulerRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventSchedulerRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::int32_t ScriptEventSchedulerRuntimeStub::Random() { return 0; }
std::uint32_t ScriptEventSchedulerRuntimeStub::GetConditionDocument(
    std::uint32_t) { return 0u; }
std::uint32_t ScriptEventSchedulerRuntimeStub::CallCondition(
    std::uint32_t, std::uint32_t) { return 0u; }
void ScriptEventSchedulerRuntimeStub::InsertCandidate(
    ScriptEventProcessorState&, std::uint32_t, std::uint32_t) {}
std::uint32_t ScriptEventSchedulerRuntimeStub::DispatchEvent(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventSchedulerRuntimeStub::UpdateContainer(
    std::uint32_t, float) { return 0u; }

std::uint8_t ScriptEventProcessor::SetFlag24(
    ScriptEventProcessorState& state, std::uint8_t value) {
    state.words[6] = (state.words[6] & 0xffffff00u) | value;
    return value;
}

std::uint32_t ScriptEventProcessor::SetFirstValue28(
    ScriptEventProcessorState& state, std::uint32_t value) {
    if (state.words[7] == 0u)
        state.words[7] = value;
    return state.words[7];
}

std::uint32_t ScriptEventProcessor::Initialize(
    ScriptEventProcessorState& state, std::uint32_t object_address,
    std::uint8_t incoming_stack_byte,
    IScriptEventProcessorRuntime& runtime) {
    state.words[0] = (state.words[0] & 0xffffff00u) | incoming_stack_byte;
    state.words[1] = 0u;
    state.words[2] = 0u;
    state.words[3] = 0u;
    state.words[8] = (state.words[8] & 0xffffff00u) | incoming_stack_byte;
    state.words[9] = 0u;
    state.words[10] = 0u;
    state.words[11] = 0u;
    state.words[5] = 0u;
    state.words[6] &= 0xffffff00u;
    state.words[7] = 0u;

    const auto document = runtime.Allocate(60u);
    if (document != 0u) {
        try {
            state.words[4] = runtime.InitializeDocument(
                document, "behAIAtomicMgr-memory", 0u);
        } catch (...) {
            (void)runtime.Free(document);
            throw;
        }
    } else {
        state.words[4] = 0u;
    }
    return object_address;
}

std::uint32_t ScriptEventProcessor::ClearEvents(
    ScriptEventProcessorState& state,
    IScriptEventProcessorRuntime& runtime) {
    state.words[5] = 0u;
    state.words[10] = state.words[9];
    const auto old_end = state.words[2];
    for (auto cursor = state.words[1]; cursor != old_end; cursor += 4u) {
        const auto event_data = runtime.ReadDword(cursor);
        if (event_data != 0u) {
            runtime.CleanupEventData(event_data);
            (void)runtime.Free(event_data);
        }
    }
    state.words[2] = state.words[1];
    return old_end;
}

std::uint32_t ScriptEventProcessor::Cleanup(
    ScriptEventProcessorState& state,
    IScriptEventProcessorRuntime& runtime) {
    if (state.words[4] != 0u) {
        runtime.CleanupDocument(state.words[4]);
        const auto document = state.words[4];
        if (document != 0u) {
            runtime.DestroyDocument(document);
            (void)runtime.Free(document);
        }
        state.words[4] = 0u;
    }
    (void)ClearEvents(state, runtime);
    (void)runtime.Free(state.words[9]);
    state.words[9] = 0u;
    state.words[10] = 0u;
    state.words[11] = 0u;
    const auto result = runtime.Free(state.words[1]);
    state.words[1] = 0u;
    state.words[2] = 0u;
    state.words[3] = 0u;
    return result;
}

std::uint32_t ScriptEventProcessor::ProcessAndAddEvent(
    ScriptEventProcessorState& state, const char* source,
    IScriptEventInsertRuntime& runtime) {
    StringBufferState first{};
    StringBufferState second{};
    StringBufferState third{};
    bool first_owned = true;
    bool second_owned = false;
    bool third_owned = false;
    try {
        const auto length = static_cast<std::uint32_t>(std::strlen(source));
        if (runtime.AllocateText(first, length, 1u)) {
            runtime.CopyText(first.data, source, length);
            runtime.SetTextSize(first, length);
        }
        second_owned = true;
        runtime.AssignName(second, first);
        ReleaseName(first, runtime);
        first_owned = false;
        third_owned = true;
        runtime.AssignName(third, second);

        const auto old_end = state.words[2];
        auto cursor = state.words[1];
        while (cursor != old_end) {
            const auto query = third.data != 0u
                ? third.data : runtime.EmptyStringAddress();
            if (runtime.CompareEventString(
                    runtime.ReadDword(cursor), query) == 0)
                break;
            cursor += 4u;
        }
        ReleaseName(third, runtime);
        third_owned = false;
        if (cursor != state.words[2]) {
            ReleaseName(second, runtime);
            second_owned = false;
            return 0u;
        }

        const auto allocated = runtime.Allocate(40u);
        std::uint32_t event = 0u;
        if (allocated != 0u) {
            try {
                event = runtime.InitializeEventData(allocated);
            } catch (...) {
                runtime.Free(allocated);
                throw;
            }
        }
        runtime.AssignEventCString(event, source);
        runtime.InsertEventPointer(state, state.words[2], event);
        const auto last = runtime.ReadDword(
            state.words[1] != 0u ? state.words[2] - 4u : 0xfffffffcu);
        state.words[5] = last;
        ReleaseName(second, runtime);
        second_owned = false;
        return last;
    } catch (...) {
        if (third_owned) ReleaseName(third, runtime);
        if (second_owned) ReleaseName(second, runtime);
        if (first_owned) ReleaseName(first, runtime);
        throw;
    }
}

std::uint32_t ScriptEventProcessor::FirstEvent(
    const ScriptEventProcessorState& state) {
    return state.words[1];
}

std::uint32_t ScriptEventProcessor::SelectRandomEvent(
    const ScriptEventProcessorState& state,
    IScriptEventSchedulerRuntime& runtime) {
    const auto begin = state.words[9];
    if (begin == 0u)
        return 0u;
    const auto count = std::bit_cast<std::int32_t>(
        state.words[10] - begin) >> 2;
    if (count <= 0)
        return 0u;
    const auto index = count > 1 ? runtime.Random() % count : 0;
    return runtime.ReadDword(begin +
        4u * static_cast<std::uint32_t>(index));
}

std::uint32_t ScriptEventProcessor::HandleEventProcessing(
    ScriptEventProcessorState& state, float elapsed,
    std::uint32_t task_context, std::uint32_t argument_two,
    std::uint32_t argument_three, std::uint32_t incoming_eax,
    IScriptEventSchedulerRuntime& runtime) {
    const auto enabled = static_cast<std::uint8_t>(state.words[6]);
    if (enabled == 0u)
        return (incoming_eax & 0xffffff00u) | enabled;
    if (runtime.ReadByte(task_context + 12u) == 0u)
        return runtime.UpdateContainer(task_context, elapsed);
    const auto callback = state.words[7];
    if (callback == 0u)
        return 0u;

    for (auto cursor = state.words[1]; cursor != state.words[2]; cursor += 4u) {
        const auto event = runtime.ReadDword(cursor);
        const auto document = runtime.GetConditionDocument(event);
        if (static_cast<std::uint8_t>(
                runtime.CallCondition(callback, document)) != 0u)
            runtime.InsertCandidate(state, state.words[10], event);
    }
    const auto selected = SelectRandomEvent(state, runtime);
    if (selected == 0u)
        return 0u;
    state.words[10] = state.words[9];
    const auto result = runtime.DispatchEvent(
        selected, task_context, argument_two, argument_three);
    if (static_cast<std::uint8_t>(result) == 0u)
        return result;
    return runtime.UpdateContainer(task_context, elapsed);
}

}  // namespace pal4
