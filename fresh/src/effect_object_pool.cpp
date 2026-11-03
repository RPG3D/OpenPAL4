#include "pal4/effect_object_pool.h"

#include "pal4/effect_name_count_tree.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

void PrepareEffect(std::uint32_t effect,
                   IEffectObjectPoolAcquireRuntime& runtime) {
    runtime.SetRotation(effect, {0.0f, 0.0f, 0.0f});
    runtime.SetScale(effect, {1.0f, 1.0f, 1.0f});
    runtime.SetPosition(effect, {0.0f, 1.0f, 1.0f});
    runtime.ClearFacets(effect);
    runtime.CleanupXmlNode(effect + 20u);
    runtime.RemoveFromParentFrame(effect);
    runtime.ResetVisibility(effect);
}

void InsertEffectRecord(std::uint32_t tree, std::uint32_t name,
                        std::uint32_t effect,
                        IEffectObjectPoolRecordRuntime& runtime) {
    const auto temporary = runtime.TemporaryRecords();
    runtime.WriteWord(temporary.effect_slot, effect);
    (void)CombatEventRecord::CopyData(
        temporary.copied_event, name, temporary.effect_slot, runtime);
    bool copied_live = true;
    bool key_live = false;
    try {
        runtime.WriteByte(temporary.insertion_key,
                          runtime.ReadByte(temporary.copied_event));
        runtime.SetErrorFlag(temporary.insertion_key, 0u);
        key_live = true;
        runtime.AssignText(temporary.insertion_key,
                           temporary.copied_event);
        runtime.WriteWord(temporary.insertion_key + 16u,
                          runtime.ReadWord(temporary.copied_event + 16u));
        runtime.InsertCached(tree, temporary.insertion_key);
        key_live = false;
        runtime.SetErrorFlag(temporary.insertion_key, 1u);
        copied_live = false;
        runtime.SetErrorFlag(temporary.copied_event, 1u);
    } catch (...) {
        if (key_live) runtime.SetErrorFlag(temporary.insertion_key, 1u);
        if (copied_live) runtime.SetErrorFlag(temporary.copied_event, 1u);
        throw;
    }
}

std::uint32_t NextObjectNode(std::uint32_t node,
                             IEffectObjectPoolCleanupRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, runtime.SharedObjectNil(), [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }).next;
}

void ClearObjectTree(std::uint32_t pool, std::uint32_t tree_offset,
                     IEffectObjectPoolCleanupRuntime& runtime) {
    const auto tree = pool + tree_offset;
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.DestroyObjectBranch(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedObjectNil());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return;
    }

    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto next = NextObjectNode(node, runtime);
        runtime.EraseObjectNode(tree, node);
        node = next;
    }
}

}  // namespace

std::uint32_t EffectObjectPool::Destruct(
    std::uint32_t pool, std::uint32_t scratch,
    IEffectObjectPoolDestructorRuntime& runtime) {
    Deinitialize(pool, scratch, runtime);
    CleanupAllObjects(pool, scratch, runtime);

    const auto release_nil = [&](EffectObjectPoolNilKind kind) {
        const auto references = runtime.SharedNilReferences(kind) - 1u;
        runtime.SetSharedNilReferences(kind, references);
        if (references != 0u) return references;
        const auto nil = kind == EffectObjectPoolNilKind::NameCount
            ? runtime.SharedNil() : runtime.SharedObjectNil();
        runtime.ClearSharedNil(kind);
        return nil != 0u ? runtime.Free(nil) : 0u;
    };

    const auto name_head = runtime.ReadWord(pool + 44u);
    runtime.RemoveNameCountRange(pool + 40u,
                                 runtime.ReadWord(name_head), name_head);
    (void)runtime.Free(name_head);
    runtime.WriteWord(pool + 44u, 0u);
    runtime.WriteWord(pool + 52u, 0u);
    (void)release_nil(EffectObjectPoolNilKind::NameCount);

    const auto effect_head = runtime.ReadWord(pool + 28u);
    runtime.EraseEffectObjectRange(pool + 24u,
                                   runtime.ReadWord(effect_head), effect_head);
    (void)runtime.Free(effect_head);
    runtime.WriteWord(pool + 28u, 0u);
    runtime.WriteWord(pool + 36u, 0u);
    (void)release_nil(EffectObjectPoolNilKind::Object);

    const auto object_tree = pool + 8u;
    const auto object_head = runtime.ReadWord(pool + 12u);
    auto node = runtime.ReadWord(object_head);
    if (runtime.ReadWord(pool + 20u) != 0u &&
        node == runtime.FirstElement(object_tree)) {
        runtime.DestroyObjectBranch(runtime.ReadWord(object_head + 4u));
        runtime.WriteWord(object_head + 4u, runtime.SharedObjectNil());
        runtime.WriteWord(pool + 20u, 0u);
        runtime.WriteWord(object_head, object_head);
        runtime.WriteWord(object_head + 8u, object_head);
        (void)runtime.FirstElement(object_tree);
    } else {
        while (node != object_head) {
            const auto next = NextObjectNode(node, runtime);
            runtime.EraseObjectNode(object_tree, node);
            node = next;
        }
    }
    (void)runtime.Free(object_head);
    runtime.WriteWord(pool + 12u, 0u);
    runtime.WriteWord(pool + 20u, 0u);
    return release_nil(EffectObjectPoolNilKind::Object);
}

std::uint8_t EffectObjectPool::ResetAndEnable(
    std::uint32_t pool, std::uint32_t scratch,
    std::uint32_t parameter, IEffectObjectPoolRuntime& runtime) {
    Deinitialize(pool, scratch, runtime);
    runtime.WriteByte(pool, 1u);
    runtime.WriteByte(pool + 1u, 1u);
    runtime.WriteWord(pool + 4u, parameter);
    return 1u;
}

void EffectObjectPool::Deinitialize(
    std::uint32_t pool, std::uint32_t scratch,
    IEffectObjectPoolRuntime& runtime) {
    if (runtime.ReadByte(pool) == 0u) return;

    runtime.WriteByte(pool, 0u);
    runtime.WriteByte(pool + 1u, 0u);
    runtime.WriteWord(pool + 4u, 0u);
    const auto head = runtime.ReadWord(pool + 44u);
    if (runtime.ReadWord(pool + 52u) != 0u) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != runtime.SharedNil()) {
            runtime.DestroySubtree(runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            runtime.ReleaseText(node + 12u);
            runtime.FreeNode(node);
            scratch = node;
            node = left;
        }
        runtime.WriteWord(head + 4u, runtime.SharedNil());
        runtime.WriteWord(pool + 52u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } else {
        auto node = runtime.ReadWord(head);
        while (node != head) {
            const auto next = runtime.Next(node);
            runtime.EraseNode(pool + 40u, node);
            scratch = node;
            node = next;
        }
    }
    CleanupAllObjects(pool, scratch, runtime);
}

void EffectObjectPool::CleanupAllObjects(
    std::uint32_t pool, std::uint32_t scratch,
    IEffectObjectPoolCleanupRuntime& runtime) {
    if (runtime.ReadByte(pool + 1u) != 0u) return;

    const auto active_head = runtime.ReadWord(pool + 28u);
    auto node = runtime.ReadWord(active_head);
    while (node != active_head) {
        runtime.DestroyEffectObject(
            pool, scratch, runtime.ReadWord(node + 28u));
        node = NextObjectNode(node, runtime);
    }

    ClearObjectTree(pool, 8u, runtime);
    ClearObjectTree(pool, 24u, runtime);
}

std::uint32_t EffectObjectPool::AcquireEffect(
    std::uint32_t pool, std::uint32_t key,
    IEffectObjectPoolAcquireRuntime& runtime) {
    if (runtime.ReadByte(pool) == 0u) return 0u;

    std::uint32_t effect = 0u;
    const bool cache_enabled = runtime.ReadByte(pool + 1u) != 0u;
    if (cache_enabled) {
        const auto tree = pool + 8u;
        const auto node = runtime.FindCached(tree, key);
        if (node != runtime.ReadWord(pool + 12u)) {
            effect = runtime.ReadWord(node + 28u);
            runtime.RemoveCached(tree, node);
            if (effect == 0u) return 0u;
            PrepareEffect(effect, runtime);
            return effect;
        }
    }

    auto text = runtime.ReadWord(key + 4u);
    if (text == 0u) text = runtime.EmptyTextAddress();
    const auto system = runtime.InitializeEffectSystem();
    effect = runtime.LoadEffectFromFile(system + 68u, text);
    if (effect == 0u) return 0u;

    if (cache_enabled) {
        InsertEffectRecord(pool + 24u, key, effect, runtime);
    }

    PrepareEffect(effect, runtime);
    return effect;
}

void EffectObjectPool::ReturnEffect(
    std::uint32_t pool, std::uint32_t effect,
    std::uint8_t mode, IEffectObjectPoolReturnRuntime& runtime) {
    if (runtime.ReadByte(pool) == 0u) return;
    if (runtime.ReadByte(pool + 1u) == 0u) {
        runtime.RemoveFromParentFrame(effect);
        runtime.DestroyEffect(effect);
        return;
    }

    const auto name = runtime.TemporaryNameRecord();
    runtime.ReadName(effect + 20u, name);
    bool name_live = true;
    try {
        if (mode != 0u) {
            const auto tree = pool + 24u;
            const auto head = runtime.ReadWord(pool + 28u);
            auto node = runtime.FindActive(tree, name);
            while (node != head && runtime.ReadWord(node + 28u) != effect)
                node = runtime.NextActive(node);
            if (node != head) {
                runtime.RemoveActive(tree, node);
                runtime.RemoveFromParentFrame(effect);
                runtime.DestroyEffect(effect);
                IncrementNameCount(pool, name, runtime);
            }
        } else {
            runtime.WriteByte(effect + 4u, 1u);
            runtime.WriteByte(effect + 80u, 0u);
            runtime.RemoveFromParentFrame(effect);
            InsertEffectRecord(pool + 8u, name, effect, runtime);
            IncrementNameCount(pool, name, runtime);
            TrimCache(pool, runtime);
        }
        name_live = false;
        runtime.SetErrorFlag(name, 1u);
    } catch (...) {
        if (name_live) runtime.SetErrorFlag(name, 1u);
        throw;
    }
}

void EffectObjectPool::IncrementNameCount(
    std::uint32_t pool, std::uint32_t name,
    IEffectObjectPoolNameCountRuntime& runtime) {
    const auto tree = pool + 40u;
    const auto head = runtime.ReadWord(tree + 4u);
    const auto candidate = EffectNameCountTree::LowerBound(tree, name, runtime);

    if (candidate != head &&
        runtime.CompareText(name, candidate + 12u) >= 0) {
        runtime.WriteWord(candidate + 28u,
                          runtime.ReadWord(candidate + 28u) + 1u);
        return;
    }

    const auto record = runtime.TemporaryCountRecord();
    runtime.WriteByte(record, runtime.ReadByte(name));
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    try {
        runtime.AssignText(record, name);
        runtime.WriteWord(record + 16u, 1u);
        (void)EffectNameCountTree::FindOrCreate(tree, record, runtime);
    } catch (...) {
        runtime.SetErrorFlag(record, 1u);
        throw;
    }
    runtime.SetErrorFlag(record, 1u);
}

void EffectObjectPool::TrimCache(
    std::uint32_t pool, IEffectObjectPoolReturnRuntime& runtime) {
    const auto count = runtime.ReadWord(pool + 20u);
    if (count == 0u) return;
    const auto limit = runtime.ReadWord(pool + 4u);
    if (limit == 0u || count < limit) return;

    const auto cache_head = runtime.ReadWord(pool + 12u);
    auto node = runtime.ReadWord(cache_head);
    auto selected = cache_head;
    auto lowest_count = 0xffffffffu;
    while (node != cache_head) {
        const auto query = runtime.TemporaryTrimQuery();
        const auto zero = runtime.TemporaryTrimZero();
        runtime.WriteWord(zero, 0u);
        (void)CombatEventRecord::CopyData(query, node + 12u, zero, runtime);
        std::uint32_t counter = 0u;
        try {
            counter = FindOrCreateNameCount(pool + 40u, query, runtime).node;
        } catch (...) {
            runtime.SetErrorFlag(query, 1u);
            throw;
        }
        runtime.SetErrorFlag(query, 1u);
        const auto uses = runtime.ReadWord(counter + 28u);
        if (uses <= lowest_count) {
            selected = node;
            lowest_count = uses;
        }
        node = MonsterManagerTreeCursor::Successor(
            node, runtime.SharedObjectNil(), [&](std::uint32_t address) {
                return runtime.ReadWord(address);
            }).next;
    }
    if (selected == cache_head) return;

    const auto name = runtime.TemporaryTrimName();
    runtime.WriteByte(name, runtime.ReadByte(selected + 12u));
    runtime.SetErrorFlag(name, 0u);
    try {
        runtime.AssignText(name, selected + 12u);
        const auto effect = runtime.ReadWord(selected + 28u);
        runtime.RemoveCached(pool + 8u, selected);
        const auto active_head = runtime.ReadWord(pool + 28u);
        auto active = runtime.FindActive(pool + 24u, name);
        while (active != active_head &&
               runtime.ReadWord(active + 28u) != effect)
            active = MonsterManagerTreeCursor::Successor(
                active, runtime.SharedObjectNil(),
                [&](std::uint32_t address) {
                    return runtime.ReadWord(address);
                }).next;
        if (active != active_head)
            runtime.RemoveActive(pool + 24u, active);
        runtime.RemoveFromParentFrame(effect);
        runtime.DestroyEffect(effect);
    } catch (...) {
        runtime.ReleaseTrimName(name);
        throw;
    }
    runtime.ReleaseTrimName(name);
}

EffectNameCountLookup EffectObjectPool::FindOrCreateNameCount(
    std::uint32_t tree, std::uint32_t name,
    IEffectObjectPoolNameCountRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto parent = head;
    auto child = runtime.ReadWord(head + 4u);
    bool goes_left = true;
    while (child != runtime.SharedNameNil()) {
        parent = child;
        goes_left = runtime.CompareText(name, child + 12u) < 0;
        child = runtime.ReadWord(child + (goes_left ? 0u : 8u));
    }

    if (runtime.ReadByte(tree + 8u) == 0u) {
        auto candidate = parent;
        if (goes_left) {
            if (parent == runtime.ReadWord(head)) {
                return {EffectNameCountTree::InsertNode(
                            tree, child, parent, name, runtime),
                        true};
            }
            candidate = MonsterManagerTreeCursor::Predecessor(
                parent, runtime.SharedNameNil(),
                [&](std::uint32_t address) {
                    return runtime.ReadWord(address);
                }).next;
        }
        if (runtime.CompareText(candidate + 12u, name) >= 0)
            return {candidate, false};
    }
    return {EffectNameCountTree::InsertNode(
                tree, child, parent, name, runtime), true};
}

}  // namespace pal4
