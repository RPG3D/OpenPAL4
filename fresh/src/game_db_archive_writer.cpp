#include "pal4/game_db_archive_writer.h"

namespace pal4 {

bool GameDbArchiveWriter::WriteToFile(
    const char* filename, const GameDbRecordIndexState& index,
    const GameDbFlagStaticState& flag,
    IGameDbArchiveRuntime& runtime) {
    if (filename == nullptr) return false;
    const auto file = runtime.Open(filename, "wb+");
    if (file == 0u) return false;

    const auto file_stream = runtime.CreateFileStream(file);
    ArchiveStreamAdapter adapter(runtime);
    ArchiveStreamAdapterState stream{};
    adapter.Construct(stream, file_stream, 0u);
    ArchiveTextWriter text(adapter, runtime);
    const ArchiveTextRecordState flag_text{
        flag.initialized_byte, flag.data, flag.length};
    (void)text.Write(stream, flag_text);
    constexpr auto version = kFileVersion;
    (void)adapter.Write(stream, &version, 4u);
    const auto property_count = index.words[7u];
    (void)adapter.Write(stream, &property_count, 4u);

    std::uint32_t cursor = 0u;
    for (auto property = runtime.FirstProperty(index, cursor);
         property != 0u;
         property = runtime.NextProperty(index, cursor)) {
        (void)text.Write(stream, runtime.TextRecord(property + 16u));
        if (!runtime.WriteProperty(property, stream)) {
            runtime.FinishAdapter(stream);
            return false;
        }
    }

    const auto begin = index.words[1u];
    const auto record_count = begin == 0u ? 0u :
        (index.words[2u] - begin) / 24u;
    (void)adapter.Write(stream, &record_count, 4u);
    for (auto record = GameDbRecordIndex::FirstRecord(index, cursor);
         record != 0u;
         record = GameDbRecordIndex::NextRecord(index, cursor)) {
        const auto object = runtime.ReadWord(record + 20u);
        if (object == 0u) continue;
        (void)text.Write(stream, runtime.TextRecord(record));
        if (!runtime.WriteObject(object, stream)) {
            runtime.FinishAdapter(stream);
            return false;
        }
    }

    runtime.Flush(file);
    runtime.Close(file);
    runtime.FinishAdapter(stream);
    return true;
}

bool GameDbArchiveWriter::WritePropertyRecord(
    const std::uint32_t property,
    const ArchiveStreamAdapterState& stream,
    IGameDbArchiveRuntime& runtime) {
    ArchiveStreamAdapter adapter(runtime);
    ArchiveTextWriter text(adapter, runtime);
    constexpr auto start = kPropertyStart;
    (void)adapter.Write(stream, &start, 4u);
    (void)text.Write(stream, runtime.TextRecord(property + 32u));
    const auto begin = runtime.ReadWord(property + 4u);
    const auto count = begin == 0u ? 0u :
        (runtime.ReadWord(property + 8u) - begin) / 24u;
    (void)adapter.Write(stream, &count, 4u);
    for (auto index = 0u; index < count; ++index) {
        const auto entry = runtime.PropertyEntry(property, index);
        const auto tag = runtime.ReadWord(entry);
        (void)adapter.Write(stream, &tag, 4u);
        (void)text.Write(stream, runtime.TextRecord(entry + 4u));
    }
    constexpr auto end = kPropertyEnd;
    (void)adapter.Write(stream, &end, 4u);
    return true;
}

bool GameDbArchiveWriter::WriteObjectRecord(
    const std::uint32_t object,
    const ArchiveStreamAdapterState& stream,
    IGameDbArchiveRuntime& runtime) {
    ArchiveStreamAdapter adapter(runtime);
    ArchiveTextWriter text(adapter, runtime);
    const auto property_begin = runtime.ReadWord(object + 40u);
    const auto property_count = property_begin == 0u ? 0u :
        (runtime.ReadWord(object + 44u) - property_begin) / 16u;
    const auto schema_begin = runtime.ReadWord(object + 24u);
    const auto schema_count = schema_begin == 0u ? 0u :
        (runtime.ReadWord(object + 28u) - schema_begin) / 60u;

    constexpr auto start = kObjectStart;
    (void)adapter.Write(stream, &start, 4u);
    (void)text.Write(stream, runtime.TextRecord(object + 52u));
    (void)text.Write(stream, runtime.TextRecord(object + 68u));
    const auto attribute = runtime.ReadWord(object + 84u);
    (void)adapter.Write(stream, &attribute, 4u);
    (void)adapter.Write(stream, &schema_count, 4u);
    (void)adapter.Write(stream, &property_count, 4u);

    for (auto index = 0u; index < schema_count; ++index) {
        const auto schema = schema_begin + index * 60u;
        (void)text.Write(stream, runtime.TextRecord(schema));
        const auto type = runtime.ReadWord(schema + 16u);
        (void)adapter.Write(stream, &type, 4u);
        (void)text.Write(stream, runtime.TextRecord(schema + 20u));
        (void)text.Write(stream, runtime.TextRecord(schema + 36u));
    }

    for (auto row = 0u; row < property_count; ++row) {
        const auto property = runtime.ObjectProperty(object, row);
        constexpr auto row_start = kObjectRowStart;
        (void)adapter.Write(stream, &row_start, 4u);
        for (auto field = 0u; field < schema_count; ++field) {
            const auto type = runtime.ReadWord(
                schema_begin + field * 60u + 16u);
            if (type == 1u) {
                const auto value = runtime.IntegerProperty(property, field);
                (void)adapter.Write(stream, &value, 4u);
            } else if (type == 2u) {
                (void)text.Write(stream, runtime.TextRecord(
                    runtime.TextProperty(property, field)));
            } else if (type == 3u) {
                const auto value = runtime.FloatProperty(property, field);
                (void)adapter.Write(stream, &value, 4u);
            } else {
                return false;
            }
        }
        constexpr auto row_end = kObjectRowEnd;
        (void)adapter.Write(stream, &row_end, 4u);
    }
    constexpr auto end = kObjectEnd;
    (void)adapter.Write(stream, &end, 4u);
    return true;
}

}  // namespace pal4
