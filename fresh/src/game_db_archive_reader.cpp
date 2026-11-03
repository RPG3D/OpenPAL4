#include "pal4/game_db_archive_reader.h"
#include "pal4/crt_atexit_stub.h"

#include <cstring>
#include <string>

namespace pal4 {

std::uint32_t GameDbSchemaArray::Count(
    const GameDbSchemaArrayState& state) {
    const auto begin = state.words[1u];
    return begin == 0u ? 0u : (state.words[2u] - begin) / 60u;
}

std::uint32_t GameDbSchemaArray::Capacity(
    const GameDbSchemaArrayState& state) {
    const auto begin = state.words[1u];
    return begin == 0u ? 0u : (state.words[3u] - begin) / 60u;
}

void GameDbSchemaArray::FreeElements(
    const std::uint32_t begin, const std::uint32_t end,
    IGameDbSchemaArrayRuntime& runtime) {
    for (auto element = begin; element != end; element += 60u)
        runtime.Release(element);
}

std::uint32_t GameDbSchemaArray::CopyElements(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination,
    IGameDbSchemaArrayRuntime& runtime) {
    auto output = destination;
    for (auto source = begin; source != end; source += 60u) {
        runtime.ConstructCopy(output, source);
        output += 60u;
    }
    return output;
}

void GameDbSchemaArray::ConstructRepeated(
    const std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t value, IGameDbSchemaArrayRuntime& runtime) {
    for (auto index = 0u; index < count; ++index)
        runtime.ConstructCopy(destination + index * 60u, value);
}

std::uint32_t GameDbSchemaArray::AssignElements(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t value, IGameDbSchemaArrayRuntime& runtime) {
    for (auto element = begin; element != end; element += 60u)
        runtime.AssignCopy(element, value);
    return end;
}

std::uint32_t GameDbSchemaArray::MoveElementsBackward(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination_end,
    IGameDbSchemaArrayRuntime& runtime) {
    auto source = end;
    auto destination = destination_end;
    while (source != begin) {
        source -= 60u;
        destination -= 60u;
        runtime.AssignCopy(destination, source);
    }
    return destination;
}

std::uint32_t GameDbSchemaArray::AssignPairwise(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination,
    IGameDbSchemaArrayRuntime& runtime) {
    auto output = destination;
    for (auto source = begin; source != end; source += 60u) {
        runtime.AssignCopy(output, source);
        output += 60u;
    }
    return output;
}

bool GameDbSchemaOwner::AssignFrom(
    GameDbSchemaOwnerState& destination,
    const GameDbSchemaOwnerState& source,
    IGameDbSchemaOwnerRuntime& runtime) {
    if (&destination != &source) {
        const auto source_count = GameDbSchemaArray::Count(source.schemas);
        const auto old_count = GameDbSchemaArray::Count(destination.schemas);
        const auto source_begin = source.schemas.words[1u];
        const auto target_begin = destination.schemas.words[1u];
        const auto old_end = destination.schemas.words[2u];
        if (source_count <= old_count) {
            const auto new_end = GameDbSchemaArray::AssignPairwise(
                source_begin, source.schemas.words[2u],
                target_begin, runtime);
            GameDbSchemaArray::FreeElements(new_end, old_end, runtime);
            destination.schemas.words[2u] = new_end;
        } else if (source_count <=
                   GameDbSchemaArray::Capacity(destination.schemas)) {
            (void)GameDbSchemaArray::AssignPairwise(
                source_begin, source_begin + old_count * 60u,
                target_begin, runtime);
            destination.schemas.words[2u] = GameDbSchemaArray::CopyElements(
                source_begin + old_count * 60u, source.schemas.words[2u],
                old_end, runtime);
        } else {
            GameDbSchemaArray::FreeElements(target_begin, old_end, runtime);
            runtime.Free(target_begin);
            const auto replacement = runtime.Allocate(source_count * 60u);
            destination.schemas.words[1u] = replacement;
            destination.schemas.words[2u] = GameDbSchemaArray::CopyElements(
                source_begin, source.schemas.words[2u], replacement, runtime);
            destination.schemas.words[3u] = replacement + source_count * 60u;
        }
    }

    const auto count = GameDbSchemaArray::Count(destination.schemas);
    const auto pointer_begin = destination.pointers[1u];
    const auto pointer_count = pointer_begin == 0u
                                   ? 0u
                                   : (destination.pointers[2u] - pointer_begin) / 4u;
    const auto pointer_capacity = pointer_begin == 0u
                                      ? 0u
                                      : (destination.pointers[3u] - pointer_begin) / 4u;
    if (pointer_capacity < count) {
        const auto replacement = runtime.Allocate(count * 4u);
        runtime.CopyPointers(pointer_begin, destination.pointers[2u],
                             replacement);
        runtime.Free(pointer_begin);
        destination.pointers[1u] = replacement;
        destination.pointers[2u] = replacement + pointer_count * 4u;
        destination.pointers[3u] = replacement + count * 4u;
    }
    for (auto index = 0u; index < count; ++index)
        runtime.AppendNullPointer(destination);
    return true;
}

void GameDbSchemaOwner::BindMissing(
    GameDbSchemaOwnerState& owner, std::uint32_t& temporary_offset,
    const std::uint32_t callback_slot,
    IGameDbSchemaBindingRuntime& runtime) {
    const auto begin = owner.schemas.words[1u];
    if (begin == 0u)
        return;
    const auto count = GameDbSchemaArray::Count(owner.schemas);
    for (auto index = 0u; index < count; ++index) {
        const auto slot = owner.pointers[1u] + index * 4u;
        if (runtime.ReadPointer(slot) != 0u)
            continue;
        const auto object = runtime.AllocatePlaceholder(20u);
        if (object != 0u) {
            runtime.WriteVtable(object, 0x849898u);
            CrtTemporaryOffset::Increment(temporary_offset);
            runtime.InitializePlaceholderText(object + 4u);
            runtime.WriteVtable(object, 0x84b4b0u);
        }
        runtime.WritePointer(slot, object);
        const auto name = runtime.SchemaName(begin + index * 60u);
        runtime.SetExceptionMessage(
            std::string("find unbinded field:") + std::string(name));
        (void)EffectSystemGlobalCallback::Call(callback_slot, runtime);
    }
}

bool GameDbSchemaOwner::ProcessBindings(
    const GameDbSchemaOwnerState& owner,
    IGameDbSchemaProcessRuntime& runtime) {
    const auto begin = owner.pointers[1u];
    if (begin == 0u)
        return true;
    for (auto slot = begin; slot != owner.pointers[2u]; slot += 4u) {
        const auto object = runtime.ReadPointer(slot);
        if (object != 0u)
            runtime.InvokeFirstVirtual(object, 1u);
    }
    return true;
}

bool GameDbSchemaOwner::BindFromObject(
    GameDbSchemaOwnerState& owner, const std::uint32_t object,
    IGameDbSchemaBindRuntime& runtime) {
    const auto kind = runtime.VirtualKind(object);
    const auto source = runtime.VirtualSource(object);
    return BindEffect(owner, source, object, kind, runtime);
}

bool GameDbSchemaOwner::BindEffect(
    GameDbSchemaOwnerState& owner, const std::uint32_t source,
    const std::uint32_t object, const std::uint32_t kind,
    IGameDbSchemaBindRuntime& runtime) {
    const auto name = runtime.SourceName(source);
    const auto begin = owner.schemas.words[1u];
    const auto count = GameDbSchemaArray::Count(owner.schemas);
    for (auto index = 0u; index < count; ++index) {
        const auto element = begin + index * 60u;
        if (runtime.SchemaName(element) != name)
            continue;
        if (runtime.SchemaKind(element) != kind) {
            if (object != 0u)
                runtime.DeleteObject(object, 1u);
            GameDbBindingDiagnostics::ReportTypeMismatch(source, runtime);
            return false;
        }
        const auto slot = owner.pointers[1u] + index * 4u;
        const auto old = runtime.ReadPointer(slot);
        if (old != 0u) {
            runtime.DeleteObject(old, 1u);
            GameDbBindingDiagnostics::ReportRebind(source, runtime);
        }
        runtime.WritePointer(slot, object);
        return true;
    }
    if (object != 0u)
        runtime.DeleteObject(object, 1u);
    GameDbBindingDiagnostics::ReportMissing(source, runtime);
    return false;
}

void GameDbBindingDiagnostics::ReportMissing(
    const std::uint32_t source,
    IGameDbSchemaBindRuntime& runtime) {
    Report("binding field requested not find!! field:", source, runtime);
}

void GameDbBindingDiagnostics::ReportRebind(
    const std::uint32_t source,
    IGameDbSchemaBindRuntime& runtime) {
    Report("find bind a field which is already binded!! field:",
           source, runtime);
}

void GameDbBindingDiagnostics::ReportTypeMismatch(
    const std::uint32_t source,
    IGameDbSchemaBindRuntime& runtime) {
    Report("binding field requested not match the dbField's type !! field:",
           source, runtime);
}

void GameDbBindingDiagnostics::Report(
    const std::string_view prefix, const std::uint32_t source,
    IGameDbSchemaBindRuntime& runtime) {
    runtime.SetExceptionMessage(std::string(prefix) +
                                std::string(runtime.SourceName(source)));
    (void)EffectSystemGlobalCallback::Call(runtime.GlobalCallbackSlot(),
                                           runtime);
}

void GameDbPlaceholderEffect::Cleanup(
    GameDbPlaceholderEffectState& state,
    std::uint32_t& temporary_offset,
    IGameDbPlaceholderEffectRuntime& runtime) {
    if (state.data != 0u)
        runtime.ReleaseText(state.data);
    state.data = state.length = state.capacity = 0u;
    state.vtable = 0x849898u;
    CrtTemporaryOffset::Decrement(temporary_offset);
}

std::uint32_t GameDbPlaceholderEffect::Delete(
    const std::uint32_t address,
    GameDbPlaceholderEffectState& state,
    const std::uint8_t flags,
    std::uint32_t& temporary_offset,
    IGameDbPlaceholderEffectRuntime& runtime) {
    Cleanup(state, temporary_offset, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

bool GameDbBoundEffectFactory::CreateKindOne(
    const std::uint32_t owner, const std::uint32_t source,
    const std::uint32_t value, std::uint32_t& temporary_offset,
    IGameDbBoundEffectRuntime& runtime) {
    return Create(owner, source, value, 1u, 0x84b4c0u,
                  temporary_offset, runtime);
}

bool GameDbBoundEffectFactory::CreateKindTwo(
    const std::uint32_t owner, const std::uint32_t source,
    const std::uint32_t value, std::uint32_t& temporary_offset,
    IGameDbBoundEffectRuntime& runtime) {
    return Create(owner, source, value, 2u, 0x84b4d0u,
                  temporary_offset, runtime);
}

bool GameDbBoundEffectFactory::Create(
    const std::uint32_t owner, const std::uint32_t source,
    const std::uint32_t value, const std::uint32_t kind,
    const std::uint32_t vtable,
    std::uint32_t& temporary_offset,
    IGameDbBoundEffectRuntime& runtime) {
    const auto address = runtime.Allocate(24u);
    if (address != 0u) {
        auto& object = runtime.Object(address);
        object.vtable = 0x849898u;
        CrtTemporaryOffset::Increment(temporary_offset);
        object.text_flag = (object.text_flag & 0xffffff00u) |
                           static_cast<std::uint8_t>(source);
        object.text_data = object.text_length = object.text_capacity = 0u;
        object.vtable = vtable;
        object.value = value;
        runtime.AssignText(object, source);
    }
    return runtime.Bind(owner, source, address, kind);
}

std::int32_t GameDbBoundEffect::ApplyInteger(
    const GameDbBoundEffectState& effect,
    const std::uint32_t record,
    const GameDbTypedValueState& value,
    IGameDbBoundEffectPropertyRuntime& runtime) {
    const auto number = GameDbTypedValue::GetInteger(value);
    runtime.WriteWord(record + effect.value,
                      static_cast<std::uint32_t>(number));
    return number;
}

std::uint8_t GameDbBoundEffect::ApplyString(
    const GameDbBoundEffectState& effect,
    const std::uint32_t record,
    const GameDbTypedValueState& value,
    GameDbTypedValueErrorState& fallback,
    const std::uint8_t source_high_byte,
    IGameDbBoundEffectPropertyRuntime& runtime) {
    const auto source = GameDbTypedValue::GetStringObject(
        value, fallback, source_high_byte, runtime);
    return runtime.AssignText(record + effect.value, source);
}

void GameDbBoundEffect::CleanupKindOne(
    GameDbBoundEffectState& effect,
    std::uint32_t& temporary_offset,
    IGameDbBoundEffectPropertyRuntime& runtime) {
    Cleanup(effect, 0x84b4c0u, temporary_offset, runtime);
}

void GameDbBoundEffect::CleanupKindTwo(
    GameDbBoundEffectState& effect,
    std::uint32_t& temporary_offset,
    IGameDbBoundEffectPropertyRuntime& runtime) {
    Cleanup(effect, 0x84b4d0u, temporary_offset, runtime);
}

void GameDbBoundEffect::Cleanup(
    GameDbBoundEffectState& effect, const std::uint32_t vtable,
    std::uint32_t& temporary_offset,
    IGameDbBoundEffectPropertyRuntime& runtime) {
    effect.vtable = vtable;
    if (effect.text_data != 0u)
        runtime.ReleaseText(effect.text_data);
    effect.text_data = effect.text_length = effect.text_capacity = 0u;
    effect.vtable = 0x849898u;
    CrtTemporaryOffset::Decrement(temporary_offset);
}

void GameDbSchemaElement::ConstructCopy(
    const std::uint32_t destination, const std::uint32_t source,
    IGameDbSchemaElementRuntime& runtime) {
    if (destination == 0u)
        return;
    for (const auto offset : {0u, 20u, 36u}) {
        runtime.WriteByte(destination + offset,
                          runtime.ReadByte(source + offset));
        for (auto field = offset + 4u; field <= offset + 12u; field += 4u)
            runtime.WriteWord(destination + field, 0u);
        runtime.AssignText(destination + offset, source + offset);
        if (offset == 0u)
            runtime.WriteWord(destination + 16u,
                              runtime.ReadWord(source + 16u));
    }
    runtime.WriteWord(destination + 52u, runtime.ReadWord(source + 52u));
    runtime.WriteWord(destination + 56u, runtime.ReadWord(source + 56u));
}

std::uint32_t GameDbSchemaElement::AssignCopy(
    const std::uint32_t destination, const std::uint32_t source,
    IGameDbSchemaElementRuntime& runtime) {
    runtime.AssignText(destination, source);
    runtime.WriteWord(destination + 16u, runtime.ReadWord(source + 16u));
    runtime.AssignText(destination + 20u, source + 20u);
    runtime.AssignText(destination + 36u, source + 36u);
    runtime.WriteWord(destination + 52u, runtime.ReadWord(source + 52u));
    runtime.WriteWord(destination + 56u, runtime.ReadWord(source + 56u));
    return destination;
}

std::uint32_t GameDbSchemaArray::InsertCopy(
    GameDbSchemaArrayState& state, const std::uint32_t position,
    const std::uint32_t value, IGameDbSchemaArrayRuntime& runtime) {
    const auto begin = state.words[1u];
    const auto end = state.words[2u];
    const auto offset = begin == 0u ? 0u : position - begin;
    if (Capacity(state) > Count(state)) {
        if (position != end) {
            (void)CopyElements(end - 60u, end, end, runtime);
            (void)MoveElementsBackward(position, end - 60u, end, runtime);
            (void)AssignElements(position, position + 60u, value, runtime);
        } else {
            ConstructRepeated(end, 1u, value, runtime);
        }
        state.words[2u] = end + 60u;
        return state.words[1u] + offset;
    }

    const auto old_count = Count(state);
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * 60u);
    auto output = CopyElements(begin, position, replacement, runtime);
    runtime.ConstructCopy(output, value);
    output += 60u;
    output = CopyElements(position, end, output, runtime);
    FreeElements(begin, end, runtime);
    runtime.Free(begin);
    state.words[1u] = replacement;
    state.words[2u] = output;
    state.words[3u] = replacement + new_capacity * 60u;
    return replacement + offset;
}

std::uint32_t GameDbObjectFactory::Create(
    const char* name, IGameDbObjectFactoryRuntime& runtime) {
    const auto object = runtime.Allocate(92u);
    return object == 0u ? 0u : runtime.Construct(object, name);
}

GameDbPropertyState& GameDbProperty::Construct(
    GameDbPropertyState& state, const char* name,
    IGameDbPropertyRuntime& runtime) {
    const auto low_byte = static_cast<std::uint8_t>(
        reinterpret_cast<std::uintptr_t>(name));
    state.words[0u] = (state.words[0u] & 0xffffff00u) | low_byte;
    state.words[1u] = state.words[2u] = state.words[3u] = 0u;
    state.words[4u] = (state.words[4u] & 0xffffff00u) | low_byte;
    state.words[5u] = state.words[6u] = state.words[7u] = 0u;
    const auto length = static_cast<std::uint32_t>(std::strlen(name));
    const auto text = runtime.AllocateText(length, 1u);
    if (text != 0u) {
        runtime.CopyText(text, name, length);
        state.words[5u] = text;
        state.words[6u] = length;
        state.words[7u] = length;
    }
    state.words[9u] = state.words[10u] = state.words[11u] = 0u;
    state.words[8u] = (state.words[8u] & 0xffffff00u) | low_byte;
    const auto begin = runtime.Allocate(0xc0u);
    (void)runtime.Free(0u);
    state.words[1u] = state.words[2u] = begin;
    state.words[3u] = begin + 0xc0u;
    return state;
}

void GameDbProperty::ClearRecords(GameDbPropertyState& state,
                                  IGameDbPropertyRuntime& runtime) {
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u)
        runtime.CleanupRecord(record);
    state.words[2u] = state.words[1u];
}

int GameDbProperty::Destroy(GameDbPropertyState& state,
                            IGameDbPropertyRuntime& runtime) {
    ClearRecords(state, runtime);
    if (state.words[9u] != 0u)
        runtime.ReleaseText(state.words[9u]);
    state.words[9u] = state.words[10u] = state.words[11u] = 0u;
    if (state.words[5u] != 0u)
        runtime.ReleaseText(state.words[5u]);
    state.words[5u] = state.words[6u] = state.words[7u] = 0u;
    const auto result = runtime.Free(state.words[1u]);
    state.words[1u] = state.words[2u] = state.words[3u] = 0u;
    return result;
}

bool GameDbProperty::AddEntry(GameDbPropertyState& state,
                              const std::int32_t id, const char* name,
                              IGameDbPropertyRuntime& runtime) {
    if (id < 0)
        return false;
    const auto position = LowerBound(state, id, runtime);
    if (position != state.words[2u] &&
        id >= runtime.ReadRecordId(position))
        return false;
    if (FindEntryByName(state, name, runtime) >= 0)
        return false;
    const auto length = static_cast<std::uint32_t>(std::strlen(name));
    const auto text = runtime.AllocateText(length, 1u);
    if (text != 0u)
        runtime.CopyText(text, name, length);
    runtime.InsertRecord(state, position, id, text, length);
    if (text != 0u)
        runtime.ReleaseText(text);
    return true;
}

std::uint32_t GameDbProperty::LowerBound(
    const GameDbPropertyState& state, const std::int32_t id,
    IGameDbPropertyRuntime& runtime) {
    auto position = state.words[1u];
    auto count = (state.words[2u] - position) / 24u;
    while (count != 0u) {
        const auto half = count / 2u;
        const auto candidate = position + half * 24u;
        if (runtime.ReadRecordId(candidate) >= id) {
            count = half;
        } else {
            position = candidate + 24u;
            count -= half + 1u;
        }
    }
    return position;
}

std::int32_t GameDbProperty::FindEntryByName(
    const GameDbPropertyState& state, const char* name,
    IGameDbPropertyRuntime& runtime) {
    const std::string_view query{name};
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u) {
        if (runtime.ReadRecordText(record) == query)
            return runtime.ReadRecordId(record);
    }
    return -1;
}

std::uint32_t GameDbProperty::FindEntryById(
    const GameDbPropertyState& state, const std::int32_t id,
    IGameDbPropertyRuntime& runtime) {
    const auto record = LowerBound(state, id, runtime);
    return record != state.words[2u] && runtime.ReadRecordId(record) == id
               ? record : 0u;
}

std::uint32_t GameDbProperty::FindEntryTextById(
    const GameDbPropertyState& state, const std::int32_t id,
    IGameDbPropertyRuntime& runtime) {
    const auto record = FindEntryById(state, id, runtime);
    return record == 0u ? 0u : record + 4u;
}

std::uint32_t GameDbProperty::EntryAt(
    const GameDbPropertyState& state, const std::uint32_t index) {
    const auto begin = state.words[1u];
    return begin != 0u && index < (state.words[2u] - begin) / 24u
               ? begin + index * 24u : 0u;
}

std::uint32_t GameDbProperty::InsertCopies(
    GameDbPropertyState& state, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t source,
    IGameDbPropertyRuntime& runtime) {
    const auto begin = state.words[1u];
    const auto end = state.words[2u];
    const auto bytes = count * 24u;
    if ((state.words[3u] - end) / 24u >= count) {
        if (count == 0u)
            return position;
        const auto tail_count = (end - position) / 24u;
        if (tail_count >= count) {
            for (auto input = end - bytes, output = end;
                 input != end; input += 24u, output += 24u)
                runtime.ConstructCopy(output, input);
            for (auto input = end - bytes; input != position;) {
                input -= 24u;
                runtime.AssignCopy(input + bytes, input);
            }
            for (auto output = position; output != position + bytes;
                 output += 24u)
                runtime.AssignCopy(output, source);
        } else {
            for (auto input = position; input != end; input += 24u)
                runtime.ConstructCopy(input + bytes, input);
            for (auto output = end; output != position + bytes;
                 output += 24u)
                runtime.ConstructCopy(output, source);
            for (auto output = position; output != end; output += 24u)
                runtime.AssignCopy(output, source);
        }
        state.words[2u] = end + bytes;
        return position;
    }

    const auto old_count = begin == 0u ? 0u : (end - begin) / 24u;
    const auto growth = old_count > count ? old_count : count;
    const auto replacement = runtime.Allocate((old_count + growth) * 24u);
    auto output = replacement;
    for (auto input = begin; input != position; input += 24u) {
        runtime.ConstructCopy(output, input);
        output += 24u;
    }
    const auto inserted = output;
    for (auto index = 0u; index < count; ++index) {
        runtime.ConstructCopy(output, source);
        output += 24u;
    }
    for (auto input = position; input != end; input += 24u) {
        runtime.ConstructCopy(output, input);
        output += 24u;
    }
    for (auto input = begin; input != end; input += 24u)
        runtime.CleanupRecord(input);
    (void)runtime.Free(begin);
    state.words[1u] = replacement;
    state.words[2u] = output;
    state.words[3u] = replacement + (old_count + growth) * 24u;
    return inserted;
}

std::uint32_t GameDbProperty::DeleteRecord(
    const std::uint32_t record, const std::uint8_t flags,
    IGameDbPropertyRuntime& runtime) {
    runtime.CleanupRecord(record);
    if ((flags & 1u) != 0u)
        (void)runtime.Free(record);
    return record;
}

}  // namespace pal4
