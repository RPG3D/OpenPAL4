#include "pal4/script_skill_collection.h"

#include "pal4/game_data_object_lifecycle.h"
#include "pal4/range_no_op_callback.h"
#include "pal4/script_skill_global_record.h"

#include <array>
#include <utility>

namespace pal4 {
namespace {

void AppendRecord(std::uint32_t object, std::uint32_t begin_offset,
                  std::span<const std::uint32_t> words,
                  IScriptSkillCollectionRuntime& runtime) {
    const auto width = static_cast<std::uint32_t>(words.size() * 4u);
    const auto begin = runtime.ReadWord(object + begin_offset);
    const auto end = runtime.ReadWord(object + begin_offset + 4u);
    const auto capacity = runtime.ReadWord(object + begin_offset + 8u);
    if (capacity >= end && capacity - end >= width) {
        for (std::uint32_t i = 0u; i < words.size(); ++i)
            runtime.WriteWord(end + i * 4u, words[i]);
        runtime.WriteWord(object + begin_offset + 4u, end + width);
        return;
    }

    const auto old_size = begin == 0u ? 0u : (end - begin) / width;
    const auto growth = old_size > 1u ? old_size : 1u;
    const auto new_capacity = old_size + growth;
    const auto allocated = runtime.Allocate(new_capacity * width);
    for (std::uint32_t offset = 0u; offset < old_size * width;
         offset += 4u)
        runtime.WriteWord(allocated + offset,
                          runtime.ReadWord(begin + offset));
    for (std::uint32_t i = 0u; i < words.size(); ++i)
        runtime.WriteWord(allocated + old_size * width + i * 4u,
                          words[i]);
    RangeNoOpCallback::Invoke(begin, end);
    runtime.Free(begin);
    runtime.WriteWord(object + begin_offset + 8u,
                      allocated + new_capacity * width);
    runtime.WriteWord(object + begin_offset, allocated);
    runtime.WriteWord(object + begin_offset + 4u,
                      allocated + (old_size + 1u) * width);
}

void CompactList(std::uint32_t object, std::uint32_t begin_offset,
                 std::uint32_t width,
                 IScriptSkillCollectionRuntime& runtime) {
    auto entry = runtime.ReadWord(object + begin_offset);
    while (entry != runtime.ReadWord(object + begin_offset + 4u)) {
        if (runtime.ReadWord(entry) != 0u) {
            entry += width;
            continue;
        }
        const auto end = runtime.ReadWord(object + begin_offset + 4u);
        for (auto source = entry + width; source != end; source += width)
            for (std::uint32_t offset = 0u; offset < width; offset += 4u)
                runtime.WriteWord(source - width + offset,
                                  runtime.ReadWord(source + offset));
        runtime.WriteWord(object + begin_offset + 4u, end - width);
    }
}

}  // namespace

std::uint8_t ScriptSkillCollection::HandleFade(
    std::uint32_t object, std::int32_t skill_id,
    std::uint32_t context, IScriptSkillCollectionRuntime& runtime) {
    const auto skill = runtime.FindSkillRecord(skill_id);
    if (skill == 0u) return 0u;
    return ProcessComplexObjectList(object, skill, context, runtime);
}

std::uint8_t ScriptSkillCollection::ProcessComplexObjectList(
    std::uint32_t object, std::uint32_t skill,
    std::uint32_t context, IScriptSkillCollectionRuntime& runtime) {
    static_cast<void>(ProcessObjectFadeAndListUpdate(object, skill, runtime));
    if (SkillDataLookup::IsUnsetRecord(skill, runtime)) {
        const ScriptSkillContextState item{skill, context};
        const std::array words{item.skill, item.context};
        static_cast<void>(ScriptSkillVector8::Insert(
            object + 692u, runtime.ReadWord(object + 700u), 1u,
            words, runtime));
        static_cast<void>(ScriptSkillContext::Handle(
            item, 1u, object, object, runtime));
    } else {
        ScriptSkillCounterState item{};
        static_cast<void>(ScriptSkillCounter::Construct(item, skill, context));
        const std::array words{item.context.skill,
                               item.context.context, item.count};
        AppendRecord(object, 680u, words, runtime);
        static_cast<void>(ScriptSkillContext::Handle(
            item.context, 1u, object, object, runtime));
    }
    const std::array arguments{context, skill};
    static_cast<void>(runtime.InvokeVirtual(object, 300u, arguments));
    runtime.UpdateHealthStatus(object);
    return 1u;
}

std::uint8_t ScriptSkillCollection::FilterAndCleanData(
    std::uint32_t object, std::uint32_t key,
    IScriptSkillCollectionRuntime& runtime) {
    for (const auto [begin_offset, width] :
         std::array{std::pair{680u, 12u}, std::pair{696u, 8u}}) {
        for (auto entry = runtime.ReadWord(object + begin_offset);
             entry != runtime.ReadWord(object + begin_offset + 4u);
             entry += width) {
            const ScriptSkillContextState item{runtime.ReadWord(entry), 0u};
            if (ScriptSkillContext::FirstWord(item, runtime) != key)
                continue;
            const auto record = runtime.ResolveSkillRecord(item.skill);
            runtime.WriteWord(entry, 0u);
            const std::array arguments{record};
            static_cast<void>(runtime.InvokeVirtual(object, 304u, arguments));
        }
    }
    static_cast<void>(runtime.InvokeVirtual(object, 276u, {}));
    return 1u;
}

std::uint8_t ScriptSkillCollection::ProcessDynamicList(
    std::uint32_t object, std::uint32_t elapsed,
    IScriptSkillCollectionRuntime& runtime) {
    CompactList(object, 696u, 8u, runtime);
    CompactList(object, 680u, 12u, runtime);

    const auto begin = runtime.ReadWord(object + 680u);
    const auto initial_end = runtime.ReadWord(object + 684u);
    auto result = static_cast<std::uint8_t>(initial_end);
    for (auto entry = begin; entry != runtime.ReadWord(object + 684u);
         entry += 12u) {
        ScriptSkillCounterState item{{runtime.ReadWord(entry),
                                      runtime.ReadWord(entry + 4u)},
                                     runtime.ReadWord(entry + 8u)};
        static_cast<void>(ScriptSkillCounter::Add(item, elapsed));
        runtime.WriteWord(entry + 8u, item.count);
        const auto expired = ScriptSkillCounter::ReachedLimit(item, runtime);
        result = static_cast<std::uint8_t>(expired);
        if (!expired) continue;
        const auto record = runtime.ResolveSkillRecord(item.context.skill);
        runtime.WriteWord(entry, 0u);
        const std::array arguments{record};
        static_cast<void>(runtime.InvokeVirtual(object, 304u, arguments));
        result = static_cast<std::uint8_t>(
            runtime.InvokeVirtual(object, 276u, {}));
    }
    return result;
}

bool ScriptSkillCollection::HasSkillKey(
    std::uint32_t object, std::uint32_t key,
    IScriptSkillCollectionRuntime& runtime) {
    for (const auto [begin_offset, width] :
         std::array{std::pair{696u, 8u}, std::pair{680u, 12u}}) {
        for (auto entry = runtime.ReadWord(object + begin_offset);
             entry != runtime.ReadWord(object + begin_offset + 4u);
             entry += width) {
            const ScriptSkillContextState item{runtime.ReadWord(entry), 0u};
            if (ScriptSkillContext::FirstWord(item, runtime) == key)
                return true;
        }
    }
    return false;
}

bool ScriptSkillCollection::HasRecordField60(
    std::uint32_t object, std::uint32_t value,
    IScriptSkillCollectionRuntime& runtime) {
    for (const auto [begin_offset, width] :
         std::array{std::pair{696u, 8u}, std::pair{680u, 12u}}) {
        for (auto entry = runtime.ReadWord(object + begin_offset);
             entry != runtime.ReadWord(object + begin_offset + 4u);
             entry += width) {
            const auto record = runtime.ResolveSkillRecord(
                runtime.ReadWord(entry));
            if (runtime.ReadWord(record + 60u) == value)
                return true;
        }
    }
    return false;
}

bool ScriptSkillCollection::TriggerSkillForTarget(
    std::uint32_t object, std::int32_t skill_id,
    std::uint32_t target, IScriptSkillCollectionRuntime& runtime) {
    const auto skill = runtime.FindSkillRecord(skill_id);
    if (skill == 0u) return false;
    const std::array actor_arguments{skill, 2u, object, target};
    static_cast<void>(runtime.InvokeVirtual(object, 280u, actor_arguments));
    const std::array target_arguments{object, skill, 2u};
    static_cast<void>(runtime.InvokeVirtual(target, 308u, target_arguments));
    runtime.ProcessSkillChain(runtime.SkillSystemInstance(), skill,
                              object, object, target);
    static_cast<void>(runtime.InvokeVirtual(object, 284u, actor_arguments));
    static_cast<void>(runtime.InvokeVirtual(target, 312u, target_arguments));
    runtime.UpdateHealthStatus(target);
    return true;
}

namespace {
template <class Runtime>
std::uint32_t ResetProperties(std::uint32_t object, Runtime& runtime) {
    std::uint32_t result = 0u;
    const std::array argument{0u};
    for (const auto offset :
         std::array{140u, 148u, 156u, 164u, 180u, 188u, 196u, 204u})
        result = runtime.InvokeVirtual(object, offset, argument);
    return result;
}
} // namespace

std::uint32_t ScriptSkillCollection::ResetObjectProperties(
    std::uint32_t object, IScriptSkillCollectionRuntime& runtime) {
    return ResetProperties(object, runtime);
}
std::uint32_t ScriptSkillCollection::ResetObjectProperties(
    std::uint32_t object, IScriptSkillListCleanupRuntime& runtime) {
    return ResetProperties(object, runtime);
}

std::uint8_t ScriptSkillCollection::ProcessObjectFadeAndListUpdate(
    std::uint32_t object, std::uint32_t skill,
    IScriptSkillCollectionRuntime& runtime) {
    const auto field60 = runtime.ReadWord(skill + 60u);
    const auto unset = SkillDataLookup::IsUnsetRecord(skill, runtime);
    std::uint8_t result;
    if (field60 != 0u) {
        result = ConditionalListCleanup(object, field60, unset, runtime);
    } else if (unset) {
        const auto end = runtime.ReadWord(object + 700u);
        result = static_cast<std::uint8_t>(end);
        for (auto entry = runtime.ReadWord(object + 696u);
             entry != runtime.ReadWord(object + 700u); entry += 8u) {
            const auto key = runtime.ReadWord(skill);
            const ScriptSkillContextState item{runtime.ReadWord(entry), 0u};
            if (ScriptSkillContext::FirstWord(item, runtime) == key)
                runtime.WriteWord(entry, 0u);
            result = static_cast<std::uint8_t>(
                runtime.ReadWord(object + 700u));
        }
    } else {
        const auto end = runtime.ReadWord(object + 684u);
        result = static_cast<std::uint8_t>(end);
        for (auto entry = runtime.ReadWord(object + 680u);
             entry != runtime.ReadWord(object + 684u); entry += 12u) {
            const auto key = runtime.ReadWord(skill);
            const ScriptSkillContextState item{runtime.ReadWord(entry), 0u};
            const auto current = ScriptSkillContext::FirstWord(item, runtime);
            result = static_cast<std::uint8_t>(current);
            if (current != key) continue;
            runtime.WriteWord(entry, 0u);
            const std::array arguments{skill};
            result = static_cast<std::uint8_t>(
                runtime.InvokeVirtual(object, 296u, arguments));
        }
    }
    if (runtime.ReadWord(skill + 8u) == 1u)
        result = static_cast<std::uint8_t>(
            runtime.InvokeVirtual(object, 276u, {}));
    return result;
}

std::uint8_t ScriptSkillCollection::ConditionalListCleanup(
    std::uint32_t object, std::uint32_t field60,
    bool unset_record, IScriptSkillCollectionRuntime& runtime) {
    const auto begin_offset = unset_record ? 696u : 680u;
    const auto width = unset_record ? 8u : 12u;
    const auto end = runtime.ReadWord(object + begin_offset + 4u);
    const auto begin = runtime.ReadWord(object + begin_offset);
    if (begin == end) return static_cast<std::uint8_t>(end);

    bool update_required = false;
    for (auto entry = begin;
         entry != runtime.ReadWord(object + begin_offset + 4u);
         entry += width) {
        const auto pointer = runtime.ReadWord(entry);
        const auto record = runtime.ResolveSkillRecord(pointer);
        if (runtime.ReadWord(record + 60u) != field60) continue;
        const auto matched_record = runtime.ResolveSkillRecord(pointer);
        runtime.WriteWord(entry, 0u);
        const std::array arguments{matched_record};
        static_cast<void>(runtime.InvokeVirtual(object, 296u, arguments));
        if (runtime.ReadWord(matched_record + 8u) == 1u)
            update_required = true;
    }
    if (!update_required) return 0u;
    return static_cast<std::uint8_t>(
        runtime.InvokeVirtual(object, 276u, {}));
}

}  // namespace pal4
