#include "pal4/effect_system_registration.h"

namespace pal4 {
namespace {

std::uint32_t FindExact(EffectRegistrationTree kind,
                        std::uint32_t tree, std::uint32_t key,
                        IEffectSystemRegistrationRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto candidate = runtime.FindLowerBound(kind, tree, key);
    return candidate != head &&
                   key >= runtime.ReadWord(candidate + 12u)
               ? candidate
               : head;
}

}  // namespace

std::uint8_t EffectSystemRegistration::AddEffectToTree(
    std::uint32_t auxiliary, std::uint32_t atomic,
    IEffectSystemRegistrationRuntime& runtime) {
    if (runtime.ReadByte(auxiliary + 4u) == 0u)
        return 0u;
    const auto tree = auxiliary + 28u;
    if (FindExact(EffectRegistrationTree::Atomics, tree, atomic,
                  runtime) == runtime.ReadWord(tree + 4u)) {
        const auto saved = runtime.ReadWord(atomic + 72u);
        if (runtime.InsertUnique(EffectRegistrationTree::Atomics,
                                 tree, atomic, saved)) {
            runtime.WriteWord(atomic + 72u,
                              runtime.EffectCallbackAddress());
            if (runtime.ReadWord(atomic + 72u) == 0u)
                runtime.WriteWord(atomic + 72u,
                                  runtime.DefaultCallbackAddress());
        }
    }
    return 1u;
}

std::uint8_t EffectSystemRegistration::RemoveEffectFromTree(
    std::uint32_t auxiliary, std::uint32_t atomic,
    IEffectSystemRegistrationRuntime& runtime) {
    const auto enabled = runtime.ReadByte(auxiliary + 4u);
    if (enabled == 0u)
        return enabled;
    const auto tree = auxiliary + 28u;
    const auto node = FindExact(EffectRegistrationTree::Atomics,
                                tree, atomic, runtime);
    if (node == runtime.ReadWord(tree + 4u))
        return static_cast<std::uint8_t>(node);
    const auto owner = runtime.ReadWord(node + 12u);
    runtime.WriteWord(owner + 72u, runtime.ReadWord(node + 16u));
    if (runtime.ReadWord(owner + 72u) == 0u)
        runtime.WriteWord(owner + 72u,
                          runtime.DefaultCallbackAddress());
    return static_cast<std::uint8_t>(
        runtime.Erase(EffectRegistrationTree::Atomics, tree, node));
}

bool EffectSystemRegistration::HandleEffectAddition(
    std::uint32_t auxiliary, std::uint32_t clump,
    IEffectSystemRegistrationRuntime& runtime) {
    if (runtime.ReadByte(auxiliary + 4u) == 0u)
        return false;
    const auto tree = auxiliary + 12u;
    if (FindExact(EffectRegistrationTree::Clumps, tree, clump,
                  runtime) == runtime.ReadWord(tree + 4u)) {
        (void)runtime.InsertUnique(EffectRegistrationTree::Clumps,
                                   tree, clump, 0u);
        (void)runtime.ProcessAllAtomics(clump, 1u);
    }
    return true;
}

std::uint8_t EffectSystemRegistration::RemoveClumpAndProcessAtomics(
    std::uint32_t auxiliary, std::uint32_t clump,
    IEffectSystemRegistrationRuntime& runtime) {
    const auto enabled = runtime.ReadByte(auxiliary + 4u);
    if (enabled == 0u)
        return enabled;
    const auto tree = auxiliary + 12u;
    const auto node = FindExact(EffectRegistrationTree::Clumps,
                                tree, clump, runtime);
    if (node == runtime.ReadWord(tree + 4u))
        return static_cast<std::uint8_t>(node);
    (void)runtime.Erase(EffectRegistrationTree::Clumps, tree, node);
    return static_cast<std::uint8_t>(runtime.ProcessAllAtomics(clump, 0u));
}

}  // namespace pal4
