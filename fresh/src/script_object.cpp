#include "pal4/script_object.h"

#include <bit>

namespace pal4 {

void ScriptObjectRuntimeStub::InitializeTaskContainer(std::uint32_t) {}
std::uint32_t ScriptObjectRuntimeStub::DestroyTaskContainer(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventNodeRuntimeStub::NilAddress() { return 0u; }
std::uint32_t ScriptEventNodeRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint32_t ScriptEventNodeRuntimeStub::CallFactory(std::uint32_t) {
    return 0u;
}
void ScriptEventNodeRuntimeStub::AddTaskAndInitBase(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t ScriptObject::Initialize(
    std::uint32_t address, IScriptObjectRuntime& runtime) {
    runtime.InitializeTaskContainer(address);
    return address;
}

std::uint32_t ScriptObject::Destroy(
    std::uint32_t address, IScriptObjectRuntime& runtime) {
    return runtime.DestroyTaskContainer(address);
}

std::uint32_t ScriptObject::SearchEventNode(
    std::uint32_t tree, std::uint32_t class_id_address,
    IScriptEventNodeRuntime& runtime) {
    auto candidate = runtime.ReadDword(tree + 4u);
    const auto nil = runtime.NilAddress();
    auto node = runtime.ReadDword(candidate + 4u);
    if (node == nil)
        return candidate;

    const auto requested = std::bit_cast<std::int32_t>(
        runtime.ReadDword(class_id_address));
    do {
        const auto node_id = std::bit_cast<std::int32_t>(
            runtime.ReadDword(node + 12u));
        if (node_id >= requested) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    } while (node != nil);
    return candidate;
}

std::uint32_t ScriptObject::ProcessResolvedFactory(std::uint32_t owner,std::optional<std::uint32_t> factory,
    std::uint32_t source,std::uint32_t timing,IScriptEventNodeRuntime& runtime){
    const auto task=factory?runtime.CallFactory(*factory):0u;
    runtime.AddTaskAndInitBase(owner,task,source,timing);return task;
}

std::uint32_t ScriptObject::ProcessEventData(
    std::uint32_t owner, std::uint32_t class_id_address,
    std::uint32_t event_data, std::uint32_t source,
    std::uint32_t timing, IScriptEventNodeRuntime& runtime) {
    const auto candidate = SearchEventNode(
        event_data + 4u, class_id_address, runtime);
    const auto header = runtime.ReadDword(event_data + 8u);
    auto chosen = candidate;
    if (candidate != header &&
        std::bit_cast<std::int32_t>(runtime.ReadDword(class_id_address)) <
        std::bit_cast<std::int32_t>(runtime.ReadDword(candidate + 12u)))
        chosen = header;

    std::optional<std::uint32_t> factory;
    if (chosen != runtime.ReadDword(event_data + 8u))
        factory = runtime.ReadDword(chosen + 16u);
    return ProcessResolvedFactory(owner,factory,source,timing,runtime);
}

}  // namespace pal4
