#include "pal4/game_db_record_index.h"
#include "pal4/utilities_text_record_copy.h"

namespace pal4 {

GameDbRecordIndexState& GameDbRecordIndex::Initialize(
    GameDbRecordIndexState& state, GameDbRecordIndexSharedTree& shared,
    IGameDbRecordIndexMemory& memory) {
    state = {};
    const auto candidate = memory.Allocate(80u);
    memory.WriteWord(candidate, 4u, 0u);
    memory.WriteWord(candidate, 76u, 1u);
    if (shared.nil == 0u) {
        shared.nil = candidate;
        memory.WriteWord(candidate, 0u, 0u);
        memory.WriteWord(candidate, 8u, 0u);
    } else {
        memory.Free(candidate);
    }
    ++shared.references;

    const auto head = memory.Allocate(80u);
    memory.WriteWord(head, 4u, shared.nil);
    memory.WriteWord(head, 76u, 0u);
    memory.WriteWord(head, 0u, head);
    memory.WriteWord(head, 8u, head);
    state.words[5u] = head;
    return state;
}

int GameDbRecordIndex::Cleanup(
    GameDbRecordIndexState& state,
    GameDbRecordIndexSharedTree& shared,
    IGameDbRecordIndexCleanupRuntime& runtime) {
    runtime.ClearRecords(state);
    const auto head = state.words[5u];
    const auto first = runtime.ReadWord(head);
    if (state.words[7u] != 0u &&
        runtime.FirstElement(head) == first) {
        runtime.DestroySubtree(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, shared.nil);
        state.words[7u] = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        (void)runtime.FirstElement(head);
    } else if (first != head) {
        auto node = first;
        do {
            const auto current = node;
            node = runtime.NextNode(node);
            runtime.EraseNode(current);
        } while (node != head);
    }
    (void)runtime.Free(head);
    state.words[5u] = 0u;
    state.words[7u] = 0u;
    if (--shared.references == 0u) {
        const auto nil = shared.nil;
        shared.nil = 0u;
        if (nil != 0u) (void)runtime.Free(nil);
    }
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u)
        runtime.ResetRecord(record, 1u);
    const auto result = runtime.Free(state.words[1u]);
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    return result;
}

int GameDbRecordIndex::CleanupTree(
    GameDbRecordIndexState& state,
    GameDbRecordIndexSharedTree& shared,
    IGameDbRecordIndexCleanupRuntime& runtime) {
    const auto head = state.words[5u];
    const auto first = runtime.ReadWord(head);
    if (state.words[7u] != 0u) {
        runtime.DestroySubtree(runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, shared.nil);
        state.words[7u] = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        (void)runtime.FirstElement(head);
    } else if (first != head) {
        auto node = first;
        do {
            const auto current = node;
            node = runtime.NextNode(node);
            runtime.EraseNode(current);
        } while (node != head);
    }
    (void)runtime.Free(head);
    state.words[5u] = 0u;
    state.words[7u] = 0u;
    if (--shared.references != 0u)
        return static_cast<int>(shared.references);
    const auto nil = shared.nil;
    shared.nil = 0u;
    return nil != 0u ? runtime.Free(nil) : 0;
}

std::uint8_t GameDbRecordIndex::InvokeLoader(
    const std::uint32_t index_address, const std::int32_t argument,
    const std::uint32_t callback,
    IGameDbRecordIndexCallbackRuntime& runtime) {
    return callback != 0u
               ? static_cast<std::uint8_t>(
                     runtime.Invoke(callback, argument, index_address))
               : 0u;
}

std::uint32_t GameDbRecordIndex::ClearRecords(
    GameDbRecordIndexState& state,
    const GameDbRecordIndexSharedTree& shared,
    IGameDbRecordIndexClearRuntime& runtime) {
    const auto begin = state.words[1u];
    const auto end = state.words[2u];
    if (begin != 0u) {
        for (auto record = begin; record <= end && end - record >= 24u;
             record += 24u) {
            const auto object = runtime.RecordObjectAt(record);
            if (object != 0u) runtime.ResetRecordObject(object, 1u);
        }
    }
    for (auto record = begin; record < end; record += 24u)
        runtime.ReleaseRecordText(record);
    state.words[2u] = begin;

    const auto head = state.words[5u];
    auto result = runtime.ReadWord(head);
    if (state.words[7u] != 0u) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != shared.nil) {
            runtime.DestroyRightSubtree(runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            runtime.CleanupNodePayload(node + 12u, 0u);
            runtime.FreeNode(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, shared.nil);
        state.words[7u] = 0u;
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        result = head;
    } else if (result != head) {
        do {
            const auto current = result;
            result = runtime.NextNode(result);
            runtime.EraseNode(current);
        } while (result != head);
    }
    return result;
}

bool GameDbRecordIndex::RegisterNamedRecord(
    GameDbRecordIndexState& state, const char* name,
    const std::uint32_t object,
    IGameDbRecordIndexRegisterRuntime& runtime) {
    if (object == 0u) return false;
    const std::string_view query(name);
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u) {
        if (runtime.NameEquals(record, query)) return false;
    }
    runtime.InsertCopy(state, state.words[2u], query, object);
    return true;
}

std::uint32_t GameDbRecordIndex::FindNamedRecord(
    const GameDbRecordIndexState& state, const char* name,
    IGameDbRecordIndexLookupRuntime& runtime) {
    const std::string_view query(name);
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u) {
        if (runtime.RecordName(record) == query)
            return runtime.RecordObject(record);
    }
    return 0u;
}

std::uint32_t GameDbRecordIndex::FindOrLoadNamedRecord(
    const GameDbRecordIndexState& state, const char* name,
    IGameDbRecordIndexLookupRuntime& runtime) {
    const std::string_view query(name);
    for (auto record = state.words[1u]; record != state.words[2u];
         record += 24u) {
        if (runtime.RecordName(record) != query) continue;
        auto object = runtime.RecordObject(record);
        if (object == 0u) {
            object = runtime.InvokeFactory(runtime.RecordFactory(record), name);
            runtime.StoreRecordObject(record, object);
        }
        return object;
    }
    return 0u;
}

void GameDbRecordIndex::RemoveNamedRecord(
    GameDbRecordIndexState& state, const char* name,
    IGameDbRecordIndexEraseRuntime& runtime) {
    const std::string_view query(name);
    const auto end = state.words[2u];
    for (auto record = state.words[1u]; record != end; record += 24u) {
        if (runtime.RecordName(record) != query) continue;
        const auto object = runtime.RecordObject(record);
        if (object != 0u) runtime.ResetRecordObject(object, 1u);
        for (auto source = record + 24u; source != end; source += 24u)
            runtime.AssignRecord(source - 24u, source);
        runtime.ReleaseRecordText(end - 24u);
        state.words[2u] = end - 24u;
        return;
    }
}

std::uint32_t GameDbRecordIndex::FirstRecord(
    const GameDbRecordIndexState& state, std::uint32_t& cursor) {
    const auto begin = state.words[1u];
    if (begin == 0u || (state.words[2u] - begin) / 24u == 0u)
        return 0u;
    cursor = 0u;
    return begin;
}

std::uint32_t GameDbRecordIndex::NextRecord(
    const GameDbRecordIndexState& state, std::uint32_t& cursor) {
    ++cursor;
    const auto begin = state.words[1u];
    if (begin != 0u && cursor < (state.words[2u] - begin) / 24u)
        return begin + 24u * cursor;
    return 0u;
}

void GameDbRecordIndex::InsertRecordCopies(
    GameDbRecordIndexState& state, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t value,
    IGameDbRecordArrayRuntime& runtime) {
    if (count == 0u) return;
    const auto begin = state.words[1u];
    const auto end = state.words[2u];
    const auto capacity = state.words[3u];
    const auto bytes = count * 24u;
    const auto tail_count = (end - position) / 24u;
    if ((capacity - end) / 24u >= count) {
        if (tail_count >= count) {
            for (auto source = end - bytes, destination = end;
                 source != end; source += 24u, destination += 24u)
                runtime.ConstructCopy(destination, source);
            for (auto source = end - bytes; source != position;) {
                source -= 24u;
                runtime.AssignCopy(source + bytes, source);
            }
            for (auto destination = position;
                 destination != position + bytes; destination += 24u)
                runtime.AssignCopy(destination, value);
        } else {
            for (auto source = position; source != end; source += 24u)
                runtime.ConstructCopy(source + bytes, source);
            for (auto destination = end;
                 destination != position + bytes; destination += 24u)
                runtime.ConstructCopy(destination, value);
            for (auto destination = position; destination != end;
                 destination += 24u)
                runtime.AssignCopy(destination, value);
        }
        state.words[2u] = end + bytes;
        return;
    }

    const auto old_count = begin == 0u ? 0u : (end - begin) / 24u;
    const auto growth = old_count > count ? old_count : count;
    const auto replacement = runtime.Allocate(24u * (old_count + growth));
    auto output = replacement;
    for (auto source = begin; source != position; source += 24u) {
        runtime.ConstructCopy(output, source);
        output += 24u;
    }
    for (auto i = 0u; i < count; ++i) {
        runtime.ConstructCopy(output, value);
        output += 24u;
    }
    for (auto source = position; source != end; source += 24u) {
        runtime.ConstructCopy(output, source);
        output += 24u;
    }
    for (auto source = begin; source != end; source += 24u)
        runtime.ReleaseRecordText(source);
    runtime.Free(begin);
    state.words[1u] = replacement;
    state.words[2u] = output;
    state.words[3u] = replacement + 24u * (old_count + growth);
}

void GameDbRecordIndex::CopyRecord24(
    const std::uint32_t destination, const std::uint32_t source,
    IUtilitiesTextRecordCopyRuntime& runtime) {
    if (destination == 0u) return;
    runtime.WriteByte(destination, runtime.ReadByte(source));
    StringBufferState text{};
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto length = runtime.ReadWord(source + 8u);
    if (destination == source) {
        if (length != 0u) runtime.ThrowRange();
        runtime.DetachAndReset(text);
        runtime.DetachAndReset(text);
    } else {
        const auto source_data = runtime.ReadWord(source + 4u);
        const auto data = source_data != 0u ? source_data
            : StringBuffer::EmptyLogMessageAddress();
        if (length != 0u && runtime.ReadByte(data - 1u) < 0xfeu) {
            runtime.SetErrorFlag(text, 1u);
            text = {data, length, runtime.ReadWord(source + 12u)};
            runtime.WriteByte(data - 1u,
                static_cast<std::uint8_t>(runtime.ReadByte(data - 1u) + 1u));
        } else if (runtime.AllocateText(text, length, 1u)) {
            runtime.Copy(text.data, data, length);
            text.length = length;
            runtime.WriteByte(text.data + length, 0u);
        }
    }
    runtime.WriteWord(destination + 4u, text.data);
    runtime.WriteWord(destination + 8u, text.length);
    runtime.WriteWord(destination + 12u, text.capacity);
    runtime.WriteWord(destination + 16u,
                      runtime.ReadWord(source + 16u));
    runtime.WriteWord(destination + 20u,
                      runtime.ReadWord(source + 20u));
}

}  // namespace pal4
