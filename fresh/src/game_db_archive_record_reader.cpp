#include "pal4/game_db_archive_reader.h"

#include <cstring>
#include <vector>

namespace pal4 {

bool GameDbArchiveReader::ReadPropertyRecord(
    const char* name, const std::uint32_t owner,
    const ArchiveStreamAdapterState& stream,
    IGameDbArchiveReadRuntime& runtime) {
    ArchiveStreamAdapter adapter(runtime);
    std::uint32_t marker = 0u;
    (void)adapter.Read(stream, &marker, 4u);
    if (marker != GameDbArchiveWriter::kPropertyStart) return false;

    const auto property = runtime.ConstructProperty(name);
    ArchiveTextReader text(adapter, runtime);
    StringBufferState label{};
    (void)text.Read(stream, label);
    runtime.SetPropertyLabel(property, runtime.TextData(label));

    std::uint32_t count = 0u;
    (void)adapter.Read(stream, &count, 4u);
    StringBufferState entry_name{};
    for (auto index = 0u; index < count; ++index) {
        std::int32_t id = 0;
        (void)adapter.Read(stream, &id, 4u);
        (void)text.Read(stream, entry_name);
        runtime.AddEntry(property, id, runtime.TextData(entry_name));
    }

    (void)adapter.Read(stream, &marker, 4u);
    const auto valid = marker == GameDbArchiveWriter::kPropertyEnd;
    if (valid) runtime.InsertProperty(owner, property);
    runtime.ReleaseText(entry_name);
    runtime.ReleaseText(label);
    runtime.DestroyProperty(property);
    return valid;
}

bool GameDbArchiveReader::ReadTable(
    const char* expected_name, const std::uint32_t index,
    const ArchiveStreamAdapterState& stream,
    IGameDbArchiveTableRuntime& runtime) {
    runtime.LogBegin(expected_name);
    ArchiveStreamAdapter adapter(runtime);
    ArchiveTextReader text(adapter, runtime);
    std::uint32_t marker = 0u;
    (void)adapter.Read(stream, &marker, 4u);
    if (marker != GameDbArchiveWriter::kObjectStart) return false;

    StringBufferState object_name{};
    (void)text.Read(stream, object_name);
    const auto name = runtime.TextData(object_name);
    auto object = runtime.FindOrLoad(index, name);
    if (object == 0u) {
        runtime.RegisterObject(index, name);
        object = runtime.FindOrLoad(index, name);
    }
    if (object == 0u || runtime.NamesDiffer(name, expected_name)) {
        runtime.ReleaseText(object_name);
        return false;
    }

    StringBufferState description{};
    StringBufferState field_name{};
    StringBufferState field_label{};
    StringBufferState field_detail{};
    StringBufferState row_text{};
    (void)text.Read(stream, description);
    runtime.SetDescription(object, runtime.TextData(description));
    std::uint32_t attribute = 0u;
    std::uint32_t field_count = 0u;
    std::uint32_t row_count = 0u;
    (void)adapter.Read(stream, &attribute, 4u);
    (void)adapter.Read(stream, &field_count, 4u);
    (void)adapter.Read(stream, &row_count, 4u);

    runtime.ReserveSchema(field_count);
    std::vector<std::uint32_t> types;
    types.reserve(field_count);
    for (auto field = 0u; field < field_count; ++field) {
        (void)text.Read(stream, field_name);
        std::uint32_t type = 0u;
        (void)adapter.Read(stream, &type, 4u);
        (void)text.Read(stream, field_label);
        (void)text.Read(stream, field_detail);
        runtime.AddSchemaField(runtime.TextData(field_name), type,
                               runtime.TextData(field_label),
                               runtime.TextData(field_detail));
        types.push_back(type);
    }
    runtime.AssignSchema(object, attribute, row_count);
    runtime.PrepareObject(object);
    runtime.InitializeRows(field_count);
    for (auto field = 0u; field < field_count; ++field)
        runtime.AddDefaultValue();

    const auto cleanup = [&]() {
        runtime.ReleaseText(row_text);
        runtime.CleanupRows();
        runtime.CleanupSchema();
        runtime.ReleaseText(field_detail);
        runtime.ReleaseText(field_label);
        runtime.ReleaseText(field_name);
        runtime.ReleaseText(description);
        runtime.ReleaseText(object_name);
    };
    for (auto row = 0u; row < row_count; ++row) {
        (void)adapter.Read(stream, &marker, 4u);
        if (marker != GameDbArchiveWriter::kObjectRowStart) {
            cleanup();
            return false;
        }
        for (auto field = 0u; field < field_count; ++field) {
            runtime.ResetValue();
            switch (types[field]) {
            case 1u: {
                std::uint32_t value = 0u;
                (void)adapter.Read(stream, &value, 4u);
                runtime.SetInteger(value);
                break;
            }
            case 2u:
                (void)text.Read(stream, row_text);
                runtime.SetText(runtime.TextData(row_text));
                break;
            case 3u: {
                float value = 0.0f;
                (void)adapter.Read(stream, &value, 4u);
                runtime.SetFloat(value);
                break;
            }
            default:
                cleanup();
                return false;
            }
            runtime.CommitValue(field);
        }
        (void)adapter.Read(stream, &marker, 4u);
        if (marker != GameDbArchiveWriter::kObjectRowEnd) {
            cleanup();
            return false;
        }
        runtime.CommitRow(object);
    }
    (void)adapter.Read(stream, &marker, 4u);
    if (marker != GameDbArchiveWriter::kObjectEnd) {
        cleanup();
        return false;
    }
    runtime.FinalizeObject(object);
    runtime.LogEnd(expected_name);
    cleanup();
    return true;
}

}  // namespace pal4
