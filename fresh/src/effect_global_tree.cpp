#include "pal4/effect_global_tree.h"
#include "pal4/ui_red_black_tree_remove.h"

#include <stdexcept>

namespace pal4 {
namespace {

class RemovalRuntime final : public IUiRedBlackTreeRemoveRuntime {
public:
    static constexpr std::uint32_t tree_address = 0xffff0000u;
    RemovalRuntime(EffectGlobalTreeState& state,
                   IEffectGlobalTreeRuntime& runtime)
        : state_(state), runtime_(runtime) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == tree_address + 4u) return state_.head;
        if (address == tree_address + 12u) return state_.count;
        return runtime_.ReadWord(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        if (address == tree_address + 12u) {
            state_.count = value;
            return;
        }
        runtime_.WriteWord(address, value);
    }
    std::uint32_t SharedNil() override { return runtime_.SharedNil(); }
    void FreeMemory(std::uint32_t address) override { runtime_.Free(address); }
    std::uint8_t ReadByte(std::uint32_t) override {
        throw std::logic_error("effect nodes have no byte payload");
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        throw std::logic_error("effect nodes have no byte payload");
    }

private:
    EffectGlobalTreeState& state_;
    IEffectGlobalTreeRuntime& runtime_;
};

}  // namespace

EffectGlobalTreeState* EffectGlobalTree::Construct(
    EffectGlobalTreeState& state, const std::uint8_t first,
    const std::uint8_t second, IEffectGlobalTreeRuntime& runtime) {
    state.tag = first;
    state.second = second;
    state.flag = 0u;
    const auto candidate = runtime.Allocate(36u);
    runtime.WriteWord(candidate + 4u, 0u);
    runtime.WriteWord(candidate + 32u, 1u);
    if (runtime.SharedNil() == 0u) {
        runtime.SetSharedNil(candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    } else {
        runtime.Free(candidate);
    }
    runtime.SetSharedNilUsers(runtime.SharedNilUsers() + 1u);
    const auto head = runtime.Allocate(36u);
    runtime.WriteWord(head + 4u, runtime.SharedNil());
    runtime.WriteWord(head + 32u, 0u);
    state.head = head;
    state.count = 0u;
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return &state;
}

EffectGlobalTreeState* EffectGlobalTree::InitializeHelper(
    EffectGlobalTreeState& state, const std::uint32_t register_value,
    IEffectGlobalTreeRuntime& runtime) {
    return Construct(state,
                     static_cast<std::uint8_t>(register_value >> 16u),
                     static_cast<std::uint8_t>(register_value >> 24u),
                     runtime);
}

int EffectGlobalTree::Initialize(
    EffectGlobalTreeState& state, const std::uint32_t register_value,
    IEffectGlobalTreeRuntime& runtime) {
    (void)InitializeHelper(state, register_value, runtime);
    return runtime.RegisterCleanup();
}

void EffectGlobalTree::Cleanup(EffectGlobalTreeState& state,
                               IEffectGlobalTreeRuntime& runtime) {
    const auto head = state.head;
    auto node = runtime.ReadWord(head);
    if (state.count != 0u) {
        (void)DestroyRecursive(runtime.ReadWord(head + 4u), runtime);
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        state.count = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.FirstElement(state);
    } else {
        while (node != head) {
            const auto current = node;
            node = Next(current, runtime);
            (void)Erase(state, current, runtime);
        }
    }
    runtime.Free(head);
    state.head = 0u;
    state.count = 0u;
    const auto users = runtime.SharedNilUsers() - 1u;
    runtime.SetSharedNilUsers(users);
    if (users == 0u) {
        const auto nil = runtime.SharedNil();
        runtime.SetSharedNil(0u);
        if (nil != 0u) runtime.Free(nil);
    }
}

std::uint32_t EffectGlobalTree::Erase(
    EffectGlobalTreeState& state, const std::uint32_t node,
    IEffectGlobalTreeRuntime& runtime) {
    RemovalRuntime adapter(state, runtime);
    return UiRedBlackTreeRemove::RemoveEffectNode(
        RemovalRuntime::tree_address, node, adapter);
}

std::uint32_t EffectGlobalTree::DestroyRecursive(
    std::uint32_t node, IEffectGlobalTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    while (node != nil) {
        const auto left = runtime.ReadWord(node);
        (void)DestroyRecursive(runtime.ReadWord(node + 8u), runtime);
        runtime.Free(node);
        node = left;
    }
    return nil;
}

std::uint32_t EffectGlobalTree::Next(
    std::uint32_t node, IEffectGlobalTreeRuntime& runtime) {
    const auto nil = runtime.SharedNil();
    auto right = runtime.ReadWord(node + 8u);
    if (right != nil) {
        while (runtime.ReadWord(right) != nil)
            right = runtime.ReadWord(right);
        return right;
    }
    auto parent = runtime.ReadWord(node + 4u);
    while (node == runtime.ReadWord(parent + 8u)) {
        node = parent;
        parent = runtime.ReadWord(parent + 4u);
    }
    return runtime.ReadWord(node + 8u) == parent ? node : parent;
}

}  // namespace pal4
