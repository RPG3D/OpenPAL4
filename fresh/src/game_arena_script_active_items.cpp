#include "pal4/game_arena_script_active_items.h"

#include <algorithm>
#include <limits>
#include <new>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kRecordBytes = 12u;
using Runtime = IGameArenaScriptActiveItemRuntime;

bool ContainsName(const std::uint32_t script, const std::uint32_t beginOffset,
                  const std::uint32_t object, Runtime& runtime) {
    for (auto cursor = runtime.ReadWord(script + beginOffset);
         cursor != runtime.ReadWord(script + beginOffset + 4u);
         cursor += kRecordBytes) {
        const auto existing = runtime.ReadWord(cursor);
        if (runtime.NamesEqualIgnoreCase(runtime.ReadWord(existing + 4u),
                                         runtime.ReadWord(object + 4u)))
            return true;
    }
    return false;
}

void CopyRecord(const std::uint32_t destination,
                const std::uint32_t source, Runtime& runtime) {
    runtime.WriteWord(destination, runtime.ReadWord(source));
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    runtime.WriteWord(destination + 8u, runtime.ReadWord(source + 8u));
}

void Append(const std::uint32_t script, const std::uint32_t offset,
            const std::uint32_t object, const std::uint32_t context,
            Runtime& runtime) {
    const auto begin = runtime.ReadWord(script + offset);
    const auto end = runtime.ReadWord(script + offset + 4u);
    const auto limit = runtime.ReadWord(script + offset + 8u);
    if (begin != 0u && (end < begin || limit < end ||
                        (end - begin) % kRecordBytes != 0u ||
                        (limit - begin) % kRecordBytes != 0u))
        throw std::invalid_argument("invalid arena active-item array");

    auto slot = end;
    if (limit == end) {
        const auto size = begin == 0u ? 0u : (end - begin) / kRecordBytes;
        const auto growth = std::max(size, 1u);
        const auto maxRecords =
            std::numeric_limits<std::uint32_t>::max() / kRecordBytes;
        if (size > maxRecords || growth > maxRecords - size)
            throw std::length_error("arena active-item array overflow");
        const auto capacity = size + growth;
        const auto replacement = runtime.Allocate(capacity * kRecordBytes);
        if (replacement == 0u) throw std::bad_alloc();
        (void)GameArenaScriptActiveItems::CopyRange(
            begin, end, replacement, runtime);
        slot = replacement + size * kRecordBytes;
        runtime.Free(begin);
        runtime.WriteWord(script + offset, replacement);
        runtime.WriteWord(script + offset + 8u,
                          replacement + capacity * kRecordBytes);
    }
    runtime.WriteWord(slot, object);
    runtime.WriteWord(slot + 4u, context);
    runtime.WriteWord(slot + 8u, 0u);
    runtime.WriteWord(script + offset + 4u, slot + kRecordBytes);
}

}  // namespace

std::uint32_t GameArenaScriptActiveItems::AddSceneObject(
    const std::uint32_t script, const std::uint32_t object,
    const std::uint32_t context, Runtime& runtime) {
    if (object == 0u || ContainsName(script, 0x26cu, object, runtime) ||
        runtime.ReadWord(object + 0x184u) == 0u)
        return 0u;
    Append(script, 0x26cu, object, context, runtime);
    return 1u;
}

std::uint32_t GameArenaScriptActiveItems::AddAnimatedObject(
    const std::uint32_t script, const std::uint32_t object,
    const std::uint32_t context, Runtime& runtime) {
    if (object == 0u || ContainsName(script, 0x27cu, object, runtime) ||
        runtime.ReadWord(object + 0x184u) != 0u)
        return 0u;
    runtime.Call(GameArenaScriptActiveItemCall::SetVirtual156, {object, 1u});
    runtime.Call(GameArenaScriptActiveItemCall::SetMaterialFlags,
                 {runtime.ReadWord(object + 20u), 0u});
    runtime.Call(GameArenaScriptActiveItemCall::AddAnimatedObject,
                 {script, object});
    Append(script, 0x27cu, object, context, runtime);
    return 1u;
}

std::uint32_t GameArenaScriptActiveItems::AddCharacter(
    const std::uint32_t script, const std::uint32_t object,
    const std::uint32_t context, Runtime& runtime) {
    if (object == 0u || ContainsName(script, 0x28cu, object, runtime) ||
        runtime.ReadWord(object + 0x1f0u) == 0u)
        return 0u;
    runtime.Call(GameArenaScriptActiveItemCall::SetStateFlag, {object, 0u});
    Append(script, 0x28cu, object, context, runtime);
    return 1u;
}

std::uint32_t GameArenaScriptActiveItems::CopyRange(
    const std::uint32_t first, const std::uint32_t last,
    std::uint32_t destination, Runtime& runtime) {
    for (auto source = first; source != last; source += kRecordBytes) {
        if (destination != 0u) CopyRecord(destination, source, runtime);
        destination += kRecordBytes;
    }
    return destination;
}

std::uint32_t GameArenaScriptActiveItems::Fill(
    std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t source, Runtime& runtime) {
    if (count == 0u) return 0u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        if (destination != 0u) CopyRecord(destination, source, runtime);
        destination += kRecordBytes;
    }
    return destination;
}

}  // namespace pal4
