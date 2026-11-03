#include "pal4/combat_property_action_creation.h"

#include "pal4/cast_property_action_task.h"
#include "pal4/property_action_task.h"
#include "pal4/throw_property_action_task.h"
#include "pal4/use_property_task.h"

namespace pal4 {
namespace {

enum class TailKind { Cast, Throw, Direct };

struct ConstructionGuard {
    ICombatPropertyActionCreationRuntime& runtime;
    std::uint32_t object;
    TailKind kind;
    bool base = false;
    bool combat = false;
    bool message = false;
    std::uint32_t effects = 0u;
    bool first_name = false;
    bool thrown = false;
    bool embedded = false;
    bool second_name = false;

    ~ConstructionGuard() {
        if (object == 0u) return;
        if (kind == TailKind::Cast) {
            if (second_name) runtime.ResetNameNode(object + 312u);
            if (first_name) {
                runtime.CleanupCastEmbedded(object + 232u);
            } else if (embedded) {
                runtime.DestroyGenericBase(object + 232u);
            }
        } else if (kind == TailKind::Throw) {
            if (second_name) runtime.ResetNameNode(object + 400u);
            if (embedded) runtime.DestroyGenericBase(object + 328u);
            if (thrown) runtime.DestroyThrowProperty(object + 240u);
            if (first_name) runtime.ResetNameNode(object + 232u);
        } else {
            if (second_name) runtime.ResetNameNode(object + 312u);
            if (embedded) runtime.DestroyGenericBase(object + 240u);
            if (first_name) runtime.ResetNameNode(object + 232u);
        }
        if (message) {
            runtime.DestroyMessageBase(object);
        } else if (combat) {
            for (auto count = effects; count > 0u; --count)
                runtime.DestroyEffectSlot(object + 96u +
                                          24u * (count - 1u));
            runtime.DestroyCombatBase(object);
        } else if (base) {
            runtime.DestroyGenericBase(object);
        }
        runtime.Free(object);
    }
};

void ConstructCommonPrefix(const std::uint32_t object,
                           ConstructionGuard& guard) {
    auto& runtime = guard.runtime;
    runtime.ConstructBase(object);
    guard.base = true;
    runtime.WriteWord(object + 60u, 0u);
    // The native constructors copy an unwritten stack byte.
    runtime.WriteByte(object + 64u, 0u);
    runtime.SetTextErrorFlag(object + 64u, 0u);
    runtime.WriteWord(object, 0x845a24u);
    guard.combat = true;
    for (std::uint32_t offset = 80u; offset < 96u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        (void)CombatEffectSlot::Construct(object + 96u + 24u * index,
                                          runtime);
        ++guard.effects;
    }
    runtime.WriteWord(object, 0x8456c4u);
    guard.message = true;
    for (std::uint32_t offset = 216u; offset < 232u; offset += 4u)
        runtime.WriteWord(object + offset, 0u);
}

}  // namespace

std::uint32_t CastPropertyActionTask::Create(
    ICombatPropertyActionCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object, TailKind::Cast};
    ConstructCommonPrefix(object, guard);
    runtime.ConstructBase(object + 232u);
    guard.embedded = true;
    runtime.ConstructNameNode(object + 304u);
    guard.first_name = true;
    runtime.WriteWord(object + 232u, UsePropertyTask::kVtable);
    runtime.ConstructNameNode(object + 312u);
    guard.second_name = true;
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t ThrowPropertyActionTask::Create(
    ICombatPropertyActionCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object, TailKind::Throw};
    ConstructCommonPrefix(object, guard);
    runtime.ConstructNameNode(object + 232u);
    guard.first_name = true;
    (void)ThrowPropertyTask::Construct(object + 240u, runtime);
    guard.thrown = true;
    runtime.ConstructBase(object + 328u);
    guard.embedded = true;
    runtime.ConstructNameNode(object + 400u);
    guard.second_name = true;
    runtime.WriteWord(object + 328u, UsePropertyTask::kVtable);
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t PropertyActionTask::Create(
    ICombatPropertyActionCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    ConstructionGuard guard{runtime, object, TailKind::Direct};
    ConstructCommonPrefix(object, guard);
    runtime.ConstructNameNode(object + 232u);
    guard.first_name = true;
    runtime.ConstructBase(object + 240u);
    guard.embedded = true;
    runtime.ConstructNameNode(object + 312u);
    guard.second_name = true;
    runtime.WriteWord(object + 240u, UsePropertyTask::kVtable);
    runtime.WriteWord(object, kVtable);
    guard.object = 0u;
    return object;
}

std::uint32_t UsePropertyTask::Create(
    ICombatPropertyActionCreationRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    bool base = false;
    try {
        runtime.ConstructBase(object);
        base = true;
        runtime.ConstructNameNode(object + 72u);
        runtime.WriteWord(object, kVtable);
    } catch (...) {
        if (base) runtime.DestroyGenericBase(object);
        runtime.Free(object);
        throw;
    }
    return object;
}

}  // namespace pal4
