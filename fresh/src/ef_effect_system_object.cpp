#include "pal4/ef_effect_system_object.h"
#include "pal4/monster_manager_tree_cursor.h"

#include <initializer_list>

namespace pal4 {

std::uint32_t EfEffectSystemObject::Construct(
    std::uint32_t object, IEfEffectSystemConstructRuntime& runtime) {
    auto flags = runtime.ScratchFlags();
    const auto call = [&](EfEffectSystemConstructCall operation,
                          std::uint32_t target) {
        (void)runtime.Call(operation, target, flags);
    };

    runtime.WriteByte(object, 0u);
    runtime.WriteByte(object + 8u, flags.first);
    runtime.WriteByte(object + 9u, flags.first);
    runtime.WriteByte(object + 16u, 0u);
    call(EfEffectSystemConstructCall::InitializeTree12, object + 8u);
    call(EfEffectSystemConstructCall::InitializeTree10, object + 24u);
    call(EfEffectSystemConstructCall::ConstructTextureDictionary40,
         object + 40u);
    call(EfEffectSystemConstructCall::ConstructNamedDictionaryTree52,
         object + 52u);
    call(EfEffectSystemConstructCall::InitializeTree5, object + 68u);
    runtime.WriteByte(object + 84u, 0u);
    runtime.WriteByte(object + 85u, 1u);
    runtime.WriteWord(object + 88u, 0u);
    call(EfEffectSystemConstructCall::InitializeTree6, object + 92u);
    call(EfEffectSystemConstructCall::InitializeTree2, object + 108u);
    call(EfEffectSystemConstructCall::InitializeTree3, object + 124u);
    runtime.WriteByte(object + 148u, flags.sixth);
    runtime.WriteByte(object + 149u, flags.sixth);
    runtime.WriteByte(object + 156u, 0u);
    call(EfEffectSystemConstructCall::InitializeTree11, object + 148u);

    const auto time = runtime.Allocate(8u);
    const auto initialized_time = time != 0u
        ? runtime.Call(EfEffectSystemConstructCall::InitializeTime,
                       time, flags)
        : 0u;
    runtime.WriteWord(object + 140u, initialized_time);

    const auto auxiliary = runtime.Allocate(44u);
    if (auxiliary != 0u) {
        if (EfEffectSystemAuxiliary::Construct(auxiliary, flags, runtime) == 0u)
            return 0u;
    }
    runtime.WriteWord(object + 144u, auxiliary);
    return object;
}

std::uint32_t EfEffectSystemAuxiliary::Construct(
    std::uint32_t auxiliary, EfEffectSystemConstructionFlags& flags,
    IEfEffectSystemConstructRuntime& runtime) {
    runtime.WriteWord(auxiliary, EfEffectSystemObject::kAuxiliaryVtableDuringInit);
    if (runtime.AuxiliarySingleton() != 0u) {
        runtime.ThrowDuplicateAuxiliary();
        return 0u;
    }
    runtime.SetAuxiliarySingleton(auxiliary);
    runtime.WriteByte(auxiliary + 4u, 0u);
    runtime.WriteWord(auxiliary + 8u, 0u);
    (void)runtime.Call(EfEffectSystemConstructCall::InitializeTree4,
                       auxiliary + 12u, flags);
    (void)runtime.Call(EfEffectSystemConstructCall::InitializeTree,
                       auxiliary + 28u, flags);
    runtime.WriteWord(auxiliary, EfEffectSystemObject::kAuxiliaryVtableReady);
    return auxiliary;
}

std::uint32_t EfEffectSystemObject::CleanupPrimaryTree(
    std::uint32_t tree, IEfEffectSystemPrimaryTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.DeleteSubtree(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.ReadFirstElement(tree);
    } else {
        while (node != head) {
            const auto next = runtime.Next(node);
            runtime.EraseNode(tree, node);
            node = next;
        }
    }

    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto references = runtime.ReleaseNilReference();
    if (references != 0u) return references;
    const auto nil = runtime.SharedNil();
    runtime.ClearSharedNil();
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t EfEffectSystemObject::Destruct(
    std::uint32_t object, IEfEffectSystemDestructRuntime& runtime) {
    const auto call = [&](EfEffectSystemDestructCall operation,
                          std::uint32_t offset) {
        return runtime.Call(operation, object + offset);
    };
    const auto release_nil = [&](EfEffectSystemOwnedTree kind) {
        if (runtime.ReleaseNilReference(kind) != 0u) return;
        const auto nil = runtime.SharedNil(kind);
        runtime.ClearSharedNil(kind);
        if (nil != 0u) (void)runtime.Free(nil);
    };

    (void)call(EfEffectSystemDestructCall::CleanupState, 0u);
    for (const auto offset : {144u, 140u}) {
        const auto owned = runtime.ReadWord(object + offset);
        if (owned != 0u) {
            runtime.DestroyOwned(owned);
            runtime.WriteWord(object + offset, 0u);
        }
    }

    const auto trailing_head = runtime.ReadWord(object + 152u);
    const auto trailing_first = runtime.FirstElement(object + 148u);
    runtime.RemoveRange(EfEffectSystemOwnedTree::At148,
                        object + 148u, trailing_first,
                        trailing_head, trailing_head);
    (void)runtime.Free(trailing_head);
    runtime.WriteWord(object + 152u, 0u);
    runtime.WriteWord(object + 160u, 0u);
    release_nil(EfEffectSystemOwnedTree::At148);

    (void)call(EfEffectSystemDestructCall::DeinitializePool, 84u);
    (void)call(EfEffectSystemDestructCall::CleanupPoolObjects, 84u);
    (void)call(EfEffectSystemDestructCall::CleanupTree124, 124u);
    (void)call(EfEffectSystemDestructCall::CleanupTree108, 108u);

    const auto active_head = runtime.ReadWord(object + 96u);
    const auto active_first = runtime.FirstElement(object + 92u);
    runtime.RemoveRange(EfEffectSystemOwnedTree::At92,
                        object + 92u, active_first, active_head, active_head);
    (void)runtime.Free(active_head);
    runtime.WriteWord(object + 96u, 0u);
    runtime.WriteWord(object + 104u, 0u);
    release_nil(EfEffectSystemOwnedTree::At92);

    (void)call(EfEffectSystemDestructCall::CleanupInstances68, 68u);
    (void)call(EfEffectSystemDestructCall::CleanupTree68, 68u);
    (void)call(EfEffectSystemDestructCall::DestroyNamedDictionaryTree52, 52u);
    (void)call(EfEffectSystemDestructCall::DestroyDictionary40, 40u);

    if (runtime.ReadWord(object + 36u) != 0u) {
        const auto head = runtime.ReadWord(object + 28u);
        runtime.RemoveRange(EfEffectSystemOwnedTree::At24,
                            object + 24u, runtime.ReadWord(head), head, 0u);
    }
    (void)call(EfEffectSystemDestructCall::CleanupTree24, 24u);
    if (runtime.ReadWord(object + 20u) != 0u) {
        const auto head = runtime.ReadWord(object + 12u);
        runtime.RemoveRange(EfEffectSystemOwnedTree::At8,
                            object + 8u, runtime.ReadWord(head), head, 0u);
    }
    return call(EfEffectSystemDestructCall::CleanupTree8, 8u);
}

void EfEffectSystemObject::CleanupRegisteredEffects(
    std::uint32_t object, IEfEffectSystemCleanupRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;
    const auto head = runtime.ReadWord(object + 152u);
    auto node = runtime.ReadWord(head);
    const auto nil = runtime.SharedNil();
    while (node != head) {
        runtime.CleanupEffect(runtime.ReadWord(node + 12u));
        node = MonsterManagerTreeCursor::Successor(
            node, nil, [&](std::uint32_t address) {
                return runtime.ReadWord(address);
            }).next;
    }
    runtime.CleanupInstances(object + 68u);
    runtime.SetPoolFlags(object + 84u, 32u);
}

std::uint8_t EfEffectSystemObject::Enable(
    std::uint32_t object, bool set_time,
    IEfEffectSystemEnableRuntime& runtime) {
    runtime.LogStart();
    runtime.CleanupState(object);
    runtime.InitializeEvents(object);
    runtime.RegisterEvents(object);
    runtime.InitializeTextureDictionary(object + 40u);
    runtime.SetPoolFlags(object + 84u, 32u);
    runtime.WriteByte(object + 85u, 1u);
    runtime.CleanupPoolObjects(object + 84u);
    if (set_time) runtime.SetTimeAndFlag(1200.0);
    runtime.WriteByte(object + 1u, 0u);
    runtime.WriteByte(object + 2u, 1u);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 164u, 0u);
    runtime.WriteByte(object, 1u);
    runtime.LogComplete();
    return 1u;
}

void EfEffectSystemObject::CleanupState(
    std::uint32_t object, IEfEffectSystemStateRuntime& runtime) {
    if (runtime.ReadByte(object) == 0u) return;

    runtime.DeinitializePool(object + 84u);
    runtime.CleanupDictionary40(object + 40u);
    runtime.CleanupNamedDictionaryTree52(object + 52u);
    runtime.CleanupInstances(object + 68u);

    const auto clear_tree = [&](EfEffectSystemStateTree kind,
                                std::uint32_t tree) {
        if (runtime.ReadWord(tree + 12u) == 0u) return;
        const auto head = runtime.ReadWord(tree + 4u);
        runtime.DeleteTreeSubtree(kind, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil(kind));
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    };
    clear_tree(EfEffectSystemStateTree::At8, object + 8u);
    clear_tree(EfEffectSystemStateTree::At24, object + 24u);

    runtime.CleanupAuxiliary(runtime.ReadWord(object + 144u));
    runtime.WriteWord(object + 164u, 0u);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteByte(object, 0u);
}

std::uint32_t EfEffectSystemAuxiliary::FinalizeSingleton(
    std::uint32_t object, IEfEffectSystemAuxiliaryRuntime& runtime) {
    runtime.WriteVtable(object,
                         EfEffectSystemObject::kAuxiliaryVtableDuringInit);
    const auto singleton = runtime.CurrentSingleton();
    if (singleton == 0u) {
        runtime.ThrowMissingSingleton();
        return 0u;
    }
    runtime.ClearSingleton();
    return singleton;
}

std::uint32_t EfEffectSystemAuxiliary::Delete(
    std::uint32_t object, std::uint8_t flags,
    IEfEffectSystemAuxiliaryRuntime& runtime) {
    (void)FinalizeSingleton(object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

std::uint32_t EfEffectSystemAuxiliary::Destruct(
    std::uint32_t object,
    IEfEffectSystemAuxiliaryDestructRuntime& runtime) {
    runtime.WriteVtable(object, EfEffectSystemObject::kAuxiliaryVtableReady);
    runtime.CleanupEffects(object);

    const auto clear_tree = [&](EfEffectSystemAuxiliaryTree kind,
                                std::uint32_t tree) {
        const auto head = runtime.ReadWord(tree + 4u);
        runtime.RemoveRange(kind, tree, runtime.ReadWord(head), head);
        runtime.Free(head);
        runtime.WriteWord(tree + 4u, 0u);
        runtime.WriteWord(tree + 12u, 0u);
        if (runtime.ReleaseNilReference(kind) != 0u) return;
        const auto nil = runtime.SharedNil(kind);
        runtime.ClearSharedNil(kind);
        if (nil != 0u) runtime.Free(nil);
    };
    clear_tree(EfEffectSystemAuxiliaryTree::At28, object + 28u);
    clear_tree(EfEffectSystemAuxiliaryTree::At12, object + 12u);
    return FinalizeSingleton(object, runtime);
}

}  // namespace pal4
