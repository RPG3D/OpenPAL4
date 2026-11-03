#include "pal4/pal_script_executor.h"

#include <new>
#include <cstring>

namespace pal4 {

namespace {

void SetLowByte(std::uint32_t& word, const std::uint8_t value) {
    word = (word & 0xffffff00u) | value;
}

}  // namespace

std::uint32_t PalScriptExecutor::GetInstance(
    PalScriptExecutorGlobals& globals,
    IPalScriptExecutorRuntime& runtime) {
    if ((globals.singleton_guard & 1u) != 0u) {
        return globals.singleton_address;
    }
    globals.singleton_guard =
        static_cast<std::uint8_t>(globals.singleton_guard | 1u);
    const auto manager = runtime.SingletonManager();
    globals.singleton_address = runtime.RegisterSingleton(
        manager, "palScriptExe", 1011u, 0x600150u, 0x6001b0u);
    return globals.singleton_address;
}

std::uint32_t PalScriptExecutor::Create(
    PalScriptExecutorGlobals& globals, const std::uint8_t native_stack_byte,
    IPalScriptExecutorRuntime& runtime) {
    const auto address = runtime.Allocate(132u);
    if (address == 0u) return 0u;
    try {
        (void)Construct(runtime.StateAt(address), globals,
                        native_stack_byte, runtime);
    } catch (...) {
        (void)runtime.Free(address);
        throw;
    }
    return address;
}

std::uint32_t PalScriptExecutor::DestroyInstance(
    const std::uint32_t address, const std::uint32_t undefined_null_return,
    PalScriptExecutorGlobals& globals,
    IPalScriptExecutorRuntime& runtime) {
    if (address == 0u) return undefined_null_return;
    (void)CleanupInstance(runtime.StateAt(address), globals, runtime);
    return runtime.Free(address);
}

std::uint32_t PalScriptExecutor::CleanupInstance(
    PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
    IPalScriptExecutorRuntime& runtime) {
    runtime.DestroyContext(state, 84u);
    runtime.CleanupString(state, 72u);

    const auto header = state.words[11u];
    runtime.CleanupDataStructure(state, runtime.TreeWord(header), header);
    (void)runtime.Free(header);
    state.words[11u] = 0u;
    state.words[13u] = 0u;

    --globals.instance_count;
    if (globals.instance_count == 0u) {
        const auto shared = globals.shared_record;
        globals.shared_record = 0u;
        if (shared != 0u) (void)runtime.Free(shared);
    }

    const auto begin = state.words[5u];
    const auto end = state.words[6u];
    for (auto record = begin; record != end; record += 16u) {
        runtime.TidyQueuedString(record, 1u);
    }
    (void)runtime.Free(begin);
    state.words[5u] = 0u;
    state.words[6u] = 0u;
    state.words[7u] = 0u;

    const auto result = runtime.Free(state.words[1u]);
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    return result;
}

PalScriptExecutorState& PalScriptExecutor::Construct(
    PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
    const std::uint8_t native_stack_byte,
    IPalScriptExecutorRuntime& runtime) {
    SetLowByte(state.words[0u], native_stack_byte);
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    SetLowByte(state.words[4u], native_stack_byte);
    for (std::uint32_t index = 5u; index <= 9u; ++index) {
        state.words[index] = 0u;
    }
    state.words[10u] = (state.words[10u] & 0xffff0000u) |
        (static_cast<std::uint32_t>(native_stack_byte) * 0x101u);
    SetLowByte(state.words[12u], 0u);

    const auto candidate = runtime.Allocate(24u);
    if (candidate == 0u) throw std::bad_alloc{};
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 20u, 1u);
    if (globals.shared_record == 0u) {
        globals.shared_record = candidate;
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    ++globals.instance_count;
    if (candidate != globals.shared_record) {
        (void)runtime.Free(candidate);
    }

    const auto sentinel = runtime.Allocate(24u);
    if (sentinel == 0u) throw std::bad_alloc{};
    runtime.WriteWord(sentinel + 4u, globals.shared_record);
    runtime.WriteWord(sentinel + 20u, 0u);
    state.words[11u] = sentinel;
    state.words[13u] = 0u;
    runtime.WriteWord(sentinel, sentinel);
    runtime.WriteWord(sentinel + 8u, sentinel);
    runtime.InitializeString(state, 72u);
    runtime.InitializeContext(state, 84u);
    state.words[14u] = 0u;
    state.words[15u] = 0u;
    return state;
}

std::uint32_t PalScriptExecutor::ResetState(
    PalScriptExecutorState& state, const std::uint32_t state_address,
    PalScriptExecutorGlobals& globals, IPalScriptExecutorRuntime& runtime) {
    const auto begin = state.words[5u];
    const auto end = state.words[6u];
    for (auto record = begin; record != end; record += 16u) {
        runtime.TidyQueuedString(record, 1u);
    }
    state.words[6u] = begin;
    state.words[2u] = state.words[1u];
    const auto result = CleanupData(state, state_address, globals, runtime);
    state.words[15u] = 0u;
    return result;
}

std::uint32_t PalScriptExecutor::InsertData(
    PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
    const std::uint32_t object, IPalScriptExecutorRuntime& runtime) {
    if (!runtime.AudioObjectExists(object)) return 0xffffffffu;

    const auto key = state.words[14u];
    const auto header = state.words[11u];
    const auto leaf = globals.shared_record;
    auto node = runtime.TreeWord(header + 4u);
    auto parent = header;
    bool insert_left = true;
    while (node != leaf) {
        parent = node;
        insert_left = static_cast<std::int32_t>(key) <
            static_cast<std::int32_t>(runtime.TreeWord(node + 12u));
        node = runtime.TreeWord(node + (insert_left ? 0u : 8u));
    }

    bool insert = (state.words[12u] & 0xffu) != 0u;
    if (!insert) {
        if (insert_left && parent == runtime.TreeWord(header)) {
            insert = true;
        } else {
            const auto candidate = insert_left
                ? runtime.PreviousTreeNode(parent) : parent;
            insert = static_cast<std::int32_t>(
                runtime.TreeWord(candidate + 12u)) <
                static_cast<std::int32_t>(key);
        }
    }
    if (insert) {
        runtime.InsertTreeNode(state, node, parent, key, object);
    }
    const auto result = state.words[14u];
    state.words[14u] = result + 1u;
    return result;
}

std::uint32_t PalScriptExecutor::RemoveData(
    PalScriptExecutorState& state, const std::uint32_t key,
    IPalScriptExecutorRuntime& runtime) {
    const auto lower_bound = runtime.LowerBound(state, key);
    const auto header = state.words[11u];
    if (lower_bound == header ||
        static_cast<std::int32_t>(key) < static_cast<std::int32_t>(
            runtime.TreeWord(lower_bound + 12u))) {
        return header;
    }
    const auto object = runtime.TreeWord(lower_bound + 16u);
    if (runtime.AudioObjectExists(object)) {
        runtime.PlayAudioObject(object);
    }
    return runtime.RemoveTreeNode(state, lower_bound);
}

std::uint32_t PalScriptExecutor::CleanupData(
    PalScriptExecutorState& state, const std::uint32_t state_address,
    PalScriptExecutorGlobals& globals, IPalScriptExecutorRuntime& runtime) {
    const auto header = state.words[11u];
    const auto leaf = globals.shared_record;
    auto node = runtime.TreeWord(header);
    while (node != header) {
        const auto object = runtime.TreeWord(node + 16u);
        if (runtime.AudioObjectExists(object)) runtime.PlayAudioObject(object);

        const auto right = runtime.TreeWord(node + 8u);
        if (right != leaf) {
            node = right;
            auto left = runtime.TreeWord(node);
            while (left != leaf) {
                node = left;
                left = runtime.TreeWord(node);
            }
        } else {
            auto parent = runtime.TreeWord(node + 4u);
            while (node == runtime.TreeWord(parent + 8u)) {
                node = parent;
                parent = runtime.TreeWord(parent + 4u);
            }
            if (runtime.TreeWord(node + 8u) != parent) node = parent;
        }
    }

    node = runtime.TreeWord(header);
    if (state.words[13u] != 0u) {
        auto root = runtime.TreeWord(header + 4u);
        while (root != leaf) {
            runtime.FreeTreeSubtree(state, runtime.TreeWord(root + 8u));
            const auto next = runtime.TreeWord(root);
            (void)runtime.Free(root);
            root = next;
        }
        runtime.WriteWord(header + 4u, leaf);
        state.words[13u] = 0u;
        runtime.WriteWord(header, header);
        runtime.WriteWord(header + 8u, header);
        state.words[14u] = 0u;
        return state_address;
    }

    while (node != header) {
        const auto current = node;
        node = runtime.NextTreeNode(node);
        (void)runtime.RemoveTreeNode(state, current);
    }
    state.words[14u] = 0u;
    return node;
}

std::uint32_t PalScriptExecutor::InitializeTimerState(
    PalScriptExecutorState& state, const std::uint32_t delay,
    const char* callback, IPalScriptTimerStateRuntime& runtime) {
    state.words[17u] = delay;
    state.words[15u] = 1u;
    state.words[16u] = 0u;
    return runtime.InitStringBuffer(state, 72u, callback,
                                    static_cast<std::uint32_t>(
                                        std::strlen(callback)));
}

}  // namespace pal4
