#include "pal4/effect_system_auxiliary_tree_lookup.h"

#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t LowerBound(std::uint32_t tree, std::uint32_t key_address,
                         std::uint32_t nil,
                         IEffectSystemAuxiliaryLookupRuntime& runtime) {
    const auto key = runtime.ReadWord(key_address);
    auto result = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(result + 4u);
    while (node != nil) {
        if (runtime.ReadWord(node + 12u) >= key) {
            result = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return result;
}

std::uint32_t Next(std::uint32_t& cursor, std::uint32_t nil,
                   IEffectSystemAuxiliaryLookupRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        cursor, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    cursor = step.next;
    return step.result;
}

}  // namespace

std::uint32_t EffectSystemAuxiliaryTreeLookup::LowerBoundAtomicInto(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t key_address,
    IEffectSystemAuxiliaryLookupRuntime& runtime) {
    runtime.WriteWord(result, LowerBound(tree, key_address,
                                          runtime.Nil(EffectRegistrationTree::Atomics), runtime));
    return result;
}

std::uint32_t EffectSystemAuxiliaryTreeLookup::LowerBoundClump(
    std::uint32_t tree, std::uint32_t key_address,
    IEffectSystemAuxiliaryLookupRuntime& runtime) {
    return LowerBound(tree, key_address, runtime.Nil(EffectRegistrationTree::Clumps), runtime);
}

std::uint32_t EffectSystemAuxiliaryTreeLookup::LowerBoundAtomic(
    std::uint32_t tree, std::uint32_t key_address,
    IEffectSystemAuxiliaryLookupRuntime& runtime) {
    return LowerBound(tree, key_address, runtime.Nil(EffectRegistrationTree::Atomics), runtime);
}

std::uint32_t EffectSystemAuxiliaryTreeLookup::NextClump(
    std::uint32_t& cursor,
    IEffectSystemAuxiliaryLookupRuntime& runtime) {
    return Next(cursor, runtime.Nil(EffectRegistrationTree::Clumps), runtime);
}

std::uint32_t EffectSystemAuxiliaryTreeLookup::NextAtomic(
    std::uint32_t& cursor,
    IEffectSystemAuxiliaryLookupRuntime& runtime) {
    return Next(cursor, runtime.Nil(EffectRegistrationTree::Atomics), runtime);
}

}  // namespace pal4
