#include "pal4/system_event_router_tree.h"

namespace pal4 {
namespace {

using Address = std::uint32_t;

Address Word(ISystemEventRouterTreeRuntime& runtime, const Address address) {
    return runtime.ReadWord(address);
}
void Set(ISystemEventRouterTreeRuntime& runtime, const Address address,
         const Address value) {
    runtime.WriteWord(address, value);
}
Address Left(const Address node, const Address nil,
             ISystemEventRouterTreeRuntime& runtime) {
    return node == nil ? nil : Word(runtime, node);
}
Address Right(const Address node, const Address nil,
              ISystemEventRouterTreeRuntime& runtime) {
    return node == nil ? nil : Word(runtime, node + 8u);
}
bool Black(const Address node, const Address nil,
           ISystemEventRouterTreeRuntime& runtime) {
    return node == nil || Word(runtime, node + 28u) != 0u;
}
void SetBlack(const Address node, const Address nil, const bool black,
              ISystemEventRouterTreeRuntime& runtime) {
    if (node != nil) Set(runtime, node + 28u, black ? 1u : 0u);
}
Address Minimum(Address node, const Address nil,
                ISystemEventRouterTreeRuntime& runtime) {
    while (Left(node, nil, runtime) != nil) node = Left(node, nil, runtime);
    return node;
}
Address Maximum(Address node, const Address nil,
                ISystemEventRouterTreeRuntime& runtime) {
    while (Right(node, nil, runtime) != nil) node = Right(node, nil, runtime);
    return node;
}
void CleanupList(const Address list, const Address head_offset,
                 const Address count_offset,
                 ISystemEventRouterTreeRuntime& runtime) {
    const auto head = Word(runtime, list + head_offset);
    if (!head) return;
    auto node = Word(runtime, head);
    while (node != head) {
        const auto next = Word(runtime, node);
        const auto previous = Word(runtime, node + 4u);
        Set(runtime, previous, next);
        Set(runtime, next + 4u, previous);
        runtime.CleanupPayload(node + 8u);
        runtime.Free(node);
        Set(runtime, list + count_offset,
            Word(runtime, list + count_offset) - 1u);
        node = next;
    }
    runtime.Free(head);
    Set(runtime, list + head_offset, 0);
    Set(runtime, list + count_offset, 0);
}

}  // namespace

std::uint32_t ScriptEventPayload::Create(
    const Address owner, const Address callback, const Address argument,
    Address& callback_slot) {
    callback_slot = callback;
    return owner + argument;
}

std::uint32_t ScriptEventPayload::CleanupSlot(
    const Address slot, ISystemEventRouterTreeRuntime& runtime) {
    const auto callback = Word(runtime, slot);
    const auto result = callback ? runtime.DeleteCallback(callback, 1u) : 0u;
    Set(runtime, slot, 0u);
    return result;
}

std::uint32_t SystemEventRouterTree::LowerBound(
    const Address tree, const Address key, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    auto result = head;
    auto node = Word(runtime, head + 4u);
    while (node != nil) {
        if (Word(runtime, node + 12u) >= key) {
            result = node;
            node = Left(node, nil, runtime);
        } else {
            node = Right(node, nil, runtime);
        }
    }
    return result;
}

std::uint32_t SystemEventRouterTree::RotateLeft(
    const Address tree, const Address node, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto pivot = Right(node, nil, runtime);
    const auto moved = Left(pivot, nil, runtime);
    Set(runtime, node + 8u, moved);
    if (moved != nil) Set(runtime, moved + 4u, node);
    const auto parent = Word(runtime, node + 4u);
    Set(runtime, pivot + 4u, parent);
    const auto head = Word(runtime, tree + 4u);
    if (node == Word(runtime, head + 4u))
        Set(runtime, head + 4u, pivot);
    else if (node == Left(parent, nil, runtime))
        Set(runtime, parent, pivot);
    else
        Set(runtime, parent + 8u, pivot);
    Set(runtime, pivot, node);
    Set(runtime, node + 4u, pivot);
    return pivot;
}

std::uint32_t SystemEventRouterTree::RotateRight(
    const Address tree, const Address node, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto pivot = Left(node, nil, runtime);
    const auto moved = Right(pivot, nil, runtime);
    Set(runtime, node, moved);
    if (moved != nil) Set(runtime, moved + 4u, node);
    const auto parent = Word(runtime, node + 4u);
    Set(runtime, pivot + 4u, parent);
    const auto head = Word(runtime, tree + 4u);
    if (node == Word(runtime, head + 4u))
        Set(runtime, head + 4u, pivot);
    else if (node == Right(parent, nil, runtime))
        Set(runtime, parent + 8u, pivot);
    else
        Set(runtime, parent, pivot);
    Set(runtime, pivot + 8u, node);
    Set(runtime, node + 4u, pivot);
    return pivot;
}

std::uint32_t SystemEventRouterTree::InsertNode(
    const Address tree, const Address key, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto head = Word(runtime, tree + 4u);
    auto parent = head;
    auto node = Word(runtime, head + 4u);
    while (node != nil) {
        parent = node;
        const auto node_key = Word(runtime, node + 12u);
        if (key == node_key && Word(runtime, tree + 8u) == 0u) return node;
        node = Word(runtime, node + (key < node_key ? 0u : 8u));
    }
    const auto inserted = AllocateNode(parent, 0, runtime);
    if (!inserted) return 0;
    const auto list_head = CreateEmptyList(0, 0, runtime);
    if (!list_head) {
        runtime.Free(inserted);
        return 0;
    }
    Set(runtime, inserted, nil);
    Set(runtime, inserted + 8u, nil);
    Set(runtime, inserted + 12u, key);
    runtime.WriteByte(inserted + 16u, static_cast<std::uint8_t>(key));
    Set(runtime, inserted + 20u, list_head);
    Set(runtime, inserted + 24u, 0);
    if (parent == head)
        Set(runtime, head + 4u, inserted);
    else if (key < Word(runtime, parent + 12u))
        Set(runtime, parent, inserted);
    else
        Set(runtime, parent + 8u, inserted);
    Set(runtime, tree + 12u, Word(runtime, tree + 12u) + 1u);

    auto current = inserted;
    while (current != Word(runtime, head + 4u)) {
        const auto p = Word(runtime, current + 4u);
        if (Black(p, nil, runtime)) break;
        const auto grand = Word(runtime, p + 4u);
        if (p == Left(grand, nil, runtime)) {
            const auto uncle = Right(grand, nil, runtime);
            if (!Black(uncle, nil, runtime)) {
                SetBlack(p, nil, true, runtime);
                SetBlack(uncle, nil, true, runtime);
                SetBlack(grand, nil, false, runtime);
                current = grand;
            } else {
                if (current == Right(p, nil, runtime)) {
                    current = p;
                    (void)RotateLeft(tree, current, nil, runtime);
                }
                const auto new_parent = Word(runtime, current + 4u);
                const auto new_grand = Word(runtime, new_parent + 4u);
                SetBlack(new_parent, nil, true, runtime);
                SetBlack(new_grand, nil, false, runtime);
                (void)RotateRight(tree, new_grand, nil, runtime);
            }
        } else {
            const auto uncle = Left(grand, nil, runtime);
            if (!Black(uncle, nil, runtime)) {
                SetBlack(p, nil, true, runtime);
                SetBlack(uncle, nil, true, runtime);
                SetBlack(grand, nil, false, runtime);
                current = grand;
            } else {
                if (current == Left(p, nil, runtime)) {
                    current = p;
                    (void)RotateRight(tree, current, nil, runtime);
                }
                const auto new_parent = Word(runtime, current + 4u);
                const auto new_grand = Word(runtime, new_parent + 4u);
                SetBlack(new_parent, nil, true, runtime);
                SetBlack(new_grand, nil, false, runtime);
                (void)RotateLeft(tree, new_grand, nil, runtime);
            }
        }
    }
    SetBlack(Word(runtime, head + 4u), nil, true, runtime);
    const auto root = Word(runtime, head + 4u);
    Set(runtime, head, Minimum(root, nil, runtime));
    Set(runtime, head + 8u, Maximum(root, nil, runtime));
    return inserted;
}

std::uint32_t SystemEventRouterTree::AllocateNode(
    const Address parent, const Address color,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto node = runtime.Allocate(32u);
    if (!node) return 0;
    Set(runtime, node + 4u, parent);
    Set(runtime, node + 28u, color);
    return node;
}

std::uint32_t SystemEventRouterTree::FindOrCreateGroup(
    const Address tree, const Address key, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto node = LowerBound(tree, key, nil, runtime);
    const auto head = Word(runtime, tree + 4u);
    if (node != head && Word(runtime, node + 12u) == key) return node + 16u;
    const auto inserted = InsertNode(tree, key, nil, runtime);
    return inserted ? inserted + 16u : 0;
}

void SystemEventRouterTree::CleanupNestedList(
    const Address list, ISystemEventRouterTreeRuntime& runtime) {
    CleanupList(list, 8u, 12u, runtime);
}

void SystemEventRouterTree::CleanupGroupList(
    const Address group, ISystemEventRouterTreeRuntime& runtime) {
    CleanupList(group, 4u, 8u, runtime);
}

void SystemEventRouterTree::RemoveListRange(
    const Address holder, const Address result_slot, Address first,
    const Address sentinel, ISystemEventRouterTreeRuntime& runtime) {
    while (first != sentinel) {
        const auto node = first;
        first = Word(runtime, node);
        const auto previous = Word(runtime, node + 4u);
        Set(runtime, previous, first);
        Set(runtime, first + 4u, previous);
        runtime.CleanupPayload(node + 8u);
        runtime.Free(node);
        Set(runtime, holder + 8u, Word(runtime, holder + 8u) - 1u);
    }
    Set(runtime, result_slot, sentinel);
}

std::uint32_t SystemEventRouterTree::CreateEmptyList(
    const Address first_link, const Address second_link,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto list = runtime.Allocate(20u);
    if (!list) return 0;
    Set(runtime, list, first_link ? first_link : list);
    Set(runtime, list + 4u, second_link ? second_link : list);
    return list;
}

std::uint32_t SystemEventRouterTree::Predecessor(
    const Address node, const Address head, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    if (node == head) return Word(runtime, head + 8u);
    const auto left = Left(node, nil, runtime);
    if (left != nil) return Maximum(left, nil, runtime);
    auto current = node;
    auto parent = Word(runtime, current + 4u);
    while (parent != head && current == Left(parent, nil, runtime)) {
        current = parent;
        parent = Word(runtime, current + 4u);
    }
    return parent;
}

std::uint32_t SystemEventRouterTree::FindExact(
    const Address tree, const Address key, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto node = LowerBound(tree, key, nil, runtime);
    return node != Word(runtime, tree + 4u) &&
                   Word(runtime, node + 12u) == key
               ? node : Word(runtime, tree + 4u);
}

std::uint32_t SystemEventRouterTree::CloneCallback(
    const Address callback, ISystemEventRouterTreeRuntime& runtime) {
    if (!callback) return 0;
    const auto copy = runtime.Allocate(12u);
    if (!copy) return 0;
    Set(runtime, copy, Word(runtime, callback));
    Set(runtime, copy + 4u, Word(runtime, callback + 4u));
    Set(runtime, copy + 8u, Word(runtime, callback + 8u));
    return copy;
}

std::uint32_t SystemEventRouterTree::ResetPayload(
    const Address payload, ISystemEventRouterTreeRuntime& runtime) {
    const auto callback = Word(runtime, payload + 4u);
    runtime.WriteByte(payload, 0);
    return callback;
}

std::uint32_t SystemEventRouterTree::LowerBoundMode(
    Address first, const Address end, const Address mode,
    ISystemEventRouterTreeRuntime& runtime) {
    auto count = 0u;
    for (auto node = first; node != end; node = Word(runtime, node)) ++count;
    while (count) {
        auto middle = first;
        const auto half = count / 2u;
        for (auto index = 0u; index < half; ++index)
            middle = Word(runtime, middle);
        if (Word(runtime, middle + 16u) < mode) {
            first = Word(runtime, middle);
            count -= half + 1u;
        } else {
            count = half;
        }
    }
    return first;
}

bool SystemEventRouterTree::CopyListRange(
    const Address destination, Address first, const Address end,
    const bool move_payload, ISystemEventRouterTreeRuntime& runtime) {
    const auto head = Word(runtime, destination + 8u);
    if (!head) return false;
    while (first != end) {
        const auto source_payload = first + 8u;
        const auto owned = runtime.ReadByte(source_payload);
        const auto source_callback = Word(runtime, source_payload + 4u);
        const auto copy_callback = move_payload
            ? source_callback : CloneCallback(source_callback, runtime);
        if (!move_payload && source_callback && !copy_callback) return false;
        const auto node = runtime.Allocate(20u);
        if (!node) {
            if (!move_payload && copy_callback) runtime.Free(copy_callback);
            return false;
        }
        const auto previous = Word(runtime, head + 4u);
        Set(runtime, node, head);
        Set(runtime, node + 4u, previous);
        Set(runtime, previous, node);
        Set(runtime, head + 4u, node);
        runtime.WriteByte(node + 8u, owned);
        Set(runtime, node + 12u, copy_callback);
        Set(runtime, node + 16u, Word(runtime, source_payload + 8u));
        if (move_payload) runtime.WriteByte(source_payload, 0);
        Set(runtime, destination + 12u, Word(runtime, destination + 12u) + 1u);
        first = Word(runtime, first);
    }
    return true;
}

bool SystemEventRouterTree::CopyGroupRecord(
    const Address destination, const Address source,
    const bool move_payload, ISystemEventRouterTreeRuntime& runtime) {
    Set(runtime, destination, Word(runtime, source));
    runtime.WriteByte(destination + 4u, runtime.ReadByte(source + 4u));
    const auto head = CreateEmptyList(0, 0, runtime);
    if (!head) return false;
    Set(runtime, destination + 8u, head);
    Set(runtime, destination + 12u, 0);
    const auto source_head = Word(runtime, source + 8u);
    if (CopyListRange(destination, Word(runtime, source_head), source_head,
                      move_payload, runtime)) return true;
    CleanupNestedList(destination, runtime);
    return false;
}

std::uint32_t SystemEventRouterTree::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    const auto group = FindOrCreateGroup(tree, key, nil, runtime);
    if (!group) return 0;
    const auto handler = runtime.Allocate(12u);
    if (handler) {
        Set(runtime, handler, 0x844910u);
        Set(runtime, handler + 4u, owner);
        Set(runtime, handler + 8u, callback);
    }
    const auto node = runtime.Allocate(20u);
    if (!node) {
        runtime.Free(handler);
        return 0;
    }
    const auto head = Word(runtime, group + 4u);
    const auto position = LowerBoundMode(Word(runtime, head), head, mode, runtime);
    const auto previous = Word(runtime, position + 4u);
    Set(runtime, node, position);
    Set(runtime, node + 4u, previous);
    Set(runtime, previous, node);
    Set(runtime, position + 4u, node);
    runtime.WriteByte(node + 8u, handler ? 1u : 0u);
    Set(runtime, node + 12u, handler);
    Set(runtime, node + 16u, mode);
    Set(runtime, group + 8u, Word(runtime, group + 8u) + 1u);
    return handler;
}

std::uint32_t SystemEventRouterTree::RegisterScriptEvent(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address argument,
    const Address nil, ISystemEventRouterTreeRuntime& runtime) {
    Address callback_slot = 0;
    const auto event_owner = ScriptEventPayload::Create(
        owner, callback, argument, callback_slot);
    return Register(tree, mode, key, event_owner, callback_slot, nil, runtime);
}

std::uint32_t ActorManagerEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t ActorManagerEventRouter::RegisterLate(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t ActorManagerEventRouter::RegisterCombatHandler(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t ActorManagerPlayerEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t WorldManagerEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatConsoleEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatActionConsoleWindowEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address argument,
    const Address nil, ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::RegisterScriptEvent(
        tree, mode, key, owner, callback, argument, nil, runtime);
}

std::uint32_t CombatMessageEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address argument,
    const Address nil, ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::RegisterScriptEvent(
        tree, mode, key, owner, callback, argument, nil, runtime);
}

std::uint32_t CombatMessageEventRouter::RegisterPlain(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatRoleStateEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatEventOutputEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatProcessMessageEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t MagicActionEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t FinalHitEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t StuntActionEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t PropertyActionEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t ThrowPropertyActionEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CastPropertyActionEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatEndSoundEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatActorEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

std::uint32_t CombatExtendedEventRouter::Register(
    const Address tree, const Address mode, const Address key,
    const Address owner, const Address callback, const Address nil,
    ISystemEventRouterTreeRuntime& runtime) {
    return SystemEventRouterTree::Register(
        tree, mode, key, owner, callback, nil, runtime);
}

}  // namespace pal4
