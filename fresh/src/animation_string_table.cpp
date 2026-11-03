#include "pal4/animation_string_table.h"

#include "pal4/animation_script_record.h"

#include <algorithm>
#include <limits>

namespace pal4 {

namespace {

class RecordCleanupRuntime final : public IAnimationScriptRecordRuntime {
public:
    explicit RecordCleanupRuntime(IAnimationStringTableRuntime& runtime)
        : runtime_(runtime) {}

    AnimationScriptRecordState& RecordAt(std::uint32_t) override {
        return unused_;
    }
    std::uint8_t ReadReference(std::uint32_t address) override {
        return runtime_.ReadReference(address);
    }
    void WriteReference(std::uint32_t address,
                        std::uint8_t value) override {
        runtime_.WriteReference(address, value);
    }
    std::uint32_t Free(std::uint32_t address) override {
        return runtime_.Free(address);
    }

private:
    IAnimationStringTableRuntime& runtime_;
    AnimationScriptRecordState unused_{};
};

void CleanupRecord(AnimationStringFieldState& name,
                   AnimationStringFieldState& value,
                   IAnimationStringTableRuntime& runtime) {
    AnimationScriptRecordState record{
        name.header, name.text, name.length, name.capacity,
        value.header, value.text, value.length, value.capacity};
    RecordCleanupRuntime cleanup_runtime(runtime);
    (void)AnimationScriptRecord::Cleanup(record, cleanup_runtime);
    name.text = record.first_text;
    name.length = record.first_length;
    name.capacity = record.first_capacity;
    value.text = record.second_text;
    value.length = record.second_length;
    value.capacity = record.second_capacity;
}

void CopyValueField(std::uint32_t destination,
                    const AnimationStringFieldState& source,
                    IAnimationStringTableRuntime& runtime) {
    auto& target = runtime.FieldAt(destination);
    target.header = (target.header & 0xffffff00u) | (source.header & 0xffu);
    target.text = 0;
    target.length = 0;
    target.capacity = 0;
    const auto length = source.length;
    if (&target == &source) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
        return;
    }
    if (length != 0u) {
        const auto source_text = source.text != 0u
                                     ? source.text : runtime.EmptyText();
        if (source_text != 0u &&
            runtime.ReadReference(source_text - 1u) < 0xfeu) {
            target.text = source.text != 0u
                              ? source.text : runtime.EmptyText();
            target.length = source.length;
            target.capacity = source.capacity;
            const auto reference = runtime.ReadReference(target.text - 1u);
            runtime.WriteReference(target.text - 1u,
                                   static_cast<std::uint8_t>(reference + 1u));
            return;
        }
    }
    if (runtime.AllocateText(destination, length, true)) {
        const auto source_text = source.text != 0u
                                     ? source.text : runtime.EmptyText();
        runtime.CopyText(target.text, source_text, length);
        runtime.SetTextSize(destination, length);
    }
}

}  // namespace

std::uint32_t AnimationStringTable::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IAnimationStringTableRuntime& runtime) {
    if (destination == 0 || source == 0) return destination;

    auto& target = runtime.TableAt(destination);
    auto& original = runtime.TableAt(source);
    target.header = (target.header & 0xffffff00u) |
                    (original.header & 0xffu);
    target.text = 0;
    target.length = 0;
    target.capacity = 0;
    const auto length = original.length;

    if (destination == source) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
    } else {
        if (length != 0) {
            const auto source_text = original.text != 0
                                         ? original.text : runtime.EmptyText();
            if (source_text != 0 &&
                runtime.ReadReference(source_text - 1u) < 0xfeu) {
                runtime.SetErrorFlag(destination, 1u);
                target.text = original.text != 0
                                  ? original.text : runtime.EmptyText();
                target.length = original.length;
                target.capacity = original.capacity;
                const auto reference = runtime.ReadReference(target.text - 1u);
                runtime.WriteReference(target.text - 1u,
                                       static_cast<std::uint8_t>(reference + 1u));
                goto finish;
            }
        }
        if (runtime.AllocateText(destination, length, true)) {
            const auto source_text = original.text != 0
                                         ? original.text : runtime.EmptyText();
            runtime.CopyText(target.text, source_text, length);
            runtime.SetTextSize(destination, length);
        }
    }

finish:
    target.begin = 0;
    target.end = 0;
    target.limit = 0;
    target.tag = (target.tag & 0xffffff00u) | (source & 0xffu);
    return destination;
}

std::uint32_t AnimationStringTable::Destroy(
    const std::uint32_t table, IAnimationStringTableRuntime& runtime) {
    if (table == 0) return 0;
    auto& state = runtime.TableAt(table);
    if (state.end >= state.begin &&
        (state.end - state.begin) % 32u == 0u) {
        for (auto element = state.begin; element != state.end;
             element += 32u) {
            runtime.SetErrorFlag(element + 16u, 1u);
            runtime.SetErrorFlag(element, 1u);
        }
    }
    (void)runtime.Free(state.begin);
    state.begin = 0;
    state.end = 0;
    state.limit = 0;

    std::uint32_t result = 0;
    if (state.text != 0) {
        const auto header = state.text - 1u;
        const auto reference = runtime.ReadReference(header);
        if (reference == 0u || reference == 0xffu) {
            result = runtime.Free(header);
        } else {
            runtime.WriteReference(
                header, static_cast<std::uint8_t>(reference - 1u));
            result = header;
        }
    }
    state.text = 0;
    state.length = 0;
    state.capacity = 0;
    return result;
}

std::uint32_t AnimationStringTable::FindString(
    const std::uint32_t table, const std::uint32_t name,
    IAnimationStringTableRuntime& runtime) {
    if (table == 0) return 0;
    const auto& state = runtime.TableAt(table);
    if (state.end < state.begin ||
        (state.end - state.begin) % 32u != 0u) return 0;
    for (auto element = state.begin; element != state.end;
         element += 32u) {
        const auto& field = runtime.FieldAt(element);
        const auto text = field.text != 0 ? field.text : runtime.EmptyText();
        if (runtime.CompareText(text, name) == 0) return element;
    }
    return 0;
}

std::uint32_t AnimationStringTable::CopyString(
    const std::uint32_t table, const std::uint32_t destination,
    const std::uint32_t name, const std::uint32_t fallback,
    IAnimationStringTableRuntime& runtime) {
    if (destination == 0) return 0;
    const auto entry = FindString(table, name, runtime);
    auto& target = runtime.FieldAt(destination);
    if (entry == 0) {
        target.header = (target.header & 0xffffff00u) |
                        (destination & 0xffu);
        target.text = 0;
        target.length = 0;
        target.capacity = 0;
        const auto length = runtime.CStringLength(fallback);
        if (runtime.AllocateText(destination, length, true)) {
            runtime.CopyText(target.text, fallback, length);
            target.length = length;
            runtime.WriteByte(target.text + length, 0u);
        }
        return destination;
    }

    const auto source_address = entry + 16u;
    auto& source = runtime.FieldAt(source_address);
    target.header = (target.header & 0xffffff00u) | (source.header & 0xffu);
    target.text = 0;
    target.length = 0;
    target.capacity = 0;
    const auto length = source.length;
    if (destination == source_address) {
        runtime.EraseText(destination, length, 0xffffffffu);
        runtime.EraseText(destination, 0u, 0u);
        return destination;
    }
    if (length != 0) {
        const auto source_text = source.text != 0
                                     ? source.text : runtime.EmptyText();
        if (source_text != 0 &&
            runtime.ReadReference(source_text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            target.text = source.text != 0
                              ? source.text : runtime.EmptyText();
            target.length = source.length;
            target.capacity = source.capacity;
            const auto reference = runtime.ReadReference(target.text - 1u);
            runtime.WriteReference(target.text - 1u,
                                   static_cast<std::uint8_t>(reference + 1u));
            return destination;
        }
    }
    if (runtime.AllocateText(destination, length, true)) {
        const auto source_text = source.text != 0
                                     ? source.text : runtime.EmptyText();
        runtime.CopyText(target.text, source_text, length);
        runtime.SetTextSize(destination, length);
    }
    return destination;
}

std::int32_t AnimationStringTable::ConvertStringToInt(
    const std::uint32_t table, const std::uint32_t name,
    const std::int32_t fallback, IAnimationStringTableRuntime& runtime) {
    const auto entry = FindString(table, name, runtime);
    if (entry == 0) return fallback;
    const auto& field = runtime.FieldAt(entry + 16u);
    const auto text = field.text != 0 ? field.text : runtime.EmptyText();
    return runtime.ParseInteger(text);
}

float AnimationStringTable::ConvertStringToFloat(
    const std::uint32_t table, const std::uint32_t name,
    const float fallback, IAnimationStringTableRuntime& runtime) {
    const auto entry = FindString(table, name, runtime);
    if (entry == 0) return fallback;
    const auto& field = runtime.FieldAt(entry + 16u);
    const auto text = field.text != 0 ? field.text : runtime.EmptyText();
    return runtime.ParseFloat(text);
}

bool AnimationStringTable::LoadFromFile(
    const std::uint32_t table, const std::uint32_t file,
    IAnimationStringTableRuntime& runtime) {
    if (table == 0) return false;
    auto& state = runtime.TableAt(table);
    const auto begin = state.begin;
    const auto end = state.end;
    if (end < begin || (end - begin) % 32u != 0u) return false;
    for (auto element = begin; element != end; element += 32u) {
        auto& name = runtime.FieldAt(element);
        auto& value = runtime.FieldAt(element + 16u);
        CleanupRecord(name, value, runtime);
    }
    state.end = begin;
    runtime.LoadTableText(table, file);

    std::uint32_t count = 0;
    runtime.ReadRecordCount(file, count);
    for (std::uint32_t index = 0; index < count; ++index) {
        AnimationStringRecordState record{};
        record.name.header = index & 0xffu;
        runtime.ResetTemporaryText(record.name, 0u);
        record.value.header = index & 0xffu;
        runtime.ResetTemporaryText(record.value, 0u);
        runtime.LoadTemporaryText(record.name, file);
        runtime.LoadTemporaryText(record.value, file);
        InsertRecords(table, state.end, 1u, record, runtime);

        CleanupRecord(record.name, record.value, runtime);
    }
    return true;
}

void AnimationStringTable::InsertRecords(
    const std::uint32_t table, const std::uint32_t position,
    const std::uint32_t count, const AnimationStringRecordState& record,
    IAnimationStringTableRuntime& runtime) {
    if (table == 0) return;
    auto& state = runtime.TableAt(table);
    const auto begin = state.begin;
    const auto end = state.end;
    const auto limit = state.limit;
    constexpr std::uint32_t stride = 32u;
    if ((begin == 0 && (end != 0 || limit != 0 || position != 0)) ||
        (begin != 0 &&
         (begin > position || position > end || end > limit ||
          (position - begin) % stride != 0u ||
          (end - begin) % stride != 0u ||
          (limit - begin) % stride != 0u)))
        return;
    const auto size = begin == 0 ? 0u : (end - begin) / stride;
    const auto spare = begin == 0 ? 0u : (limit - end) / stride;
    if (count == 0u ||
        count > std::numeric_limits<std::uint32_t>::max() / stride - size)
        return;
    const auto bytes = count * stride;

    if (spare >= count) {
        const auto tail = (end - position) / stride;
        if (tail >= count) {
            for (std::uint32_t index = 0; index < count; ++index)
                runtime.CopyRecord(end + index * stride,
                                   end - bytes + index * stride);
            for (auto source = end - bytes; source != position;) {
                source -= stride;
                runtime.AssignRecordField(source + bytes, source);
                runtime.AssignRecordField(source + bytes + 16u,
                                          source + 16u);
            }
            for (auto destination = position;
                 destination != position + bytes;
                 destination += stride) {
                runtime.AssignTemplateField(destination, record.name);
                runtime.AssignTemplateField(destination + 16u, record.value);
            }
        } else {
            for (std::uint32_t index = 0; index < tail; ++index)
                runtime.CopyRecord(position + bytes + index * stride,
                                   position + index * stride);
            for (std::uint32_t index = 0; index < count - tail; ++index)
                runtime.CopyTemplateRecord(end + index * stride, record);
            for (auto destination = position; destination != end;
                 destination += stride) {
                runtime.AssignTemplateField(destination, record.name);
                runtime.AssignTemplateField(destination + 16u, record.value);
            }
        }
        state.end = end + bytes;
        return;
    }

    const auto capacity_count = size + std::max(size, count);
    if (capacity_count >
        std::numeric_limits<std::uint32_t>::max() / stride) return;
    const auto allocation = runtime.AllocateRecordArray(capacity_count * stride);
    if (allocation == 0) return;
    if (allocation > std::numeric_limits<std::uint32_t>::max() -
                         capacity_count * stride) {
        (void)runtime.Free(allocation);
        return;
    }

    auto destination = allocation;
    for (auto source = begin; source != position;
         source += stride, destination += stride)
        runtime.CopyRecord(destination, source);
    for (std::uint32_t index = 0; index < count;
         ++index, destination += stride)
        runtime.CopyTemplateRecord(destination, record);
    for (auto source = position; source != end;
         source += stride, destination += stride)
        runtime.CopyRecord(destination, source);

    for (auto source = begin; source != end; source += stride) {
        auto& name = runtime.FieldAt(source);
        auto& value = runtime.FieldAt(source + 16u);
        CleanupRecord(name, value, runtime);
    }
    (void)runtime.Free(begin);
    state.begin = allocation;
    state.end = destination;
    state.limit = allocation + capacity_count * stride;
}

void AnimationStringTable::CopyRecord(
    const std::uint32_t destination, const std::uint32_t source,
    IAnimationStringTableRuntime& runtime) {
    if (destination == 0 || source == 0) return;
    auto& target_name = runtime.FieldAt(destination);
    const auto& source_name = runtime.FieldAt(source);
    target_name.header = (target_name.header & 0xffffff00u) |
                         (source_name.header & 0xffu);
    target_name.text = 0;
    target_name.length = 0;
    target_name.capacity = 0;
    runtime.AssignRecordField(destination, source);
    const auto& source_value = runtime.FieldAt(source + 16u);
    CopyValueField(destination + 16u, source_value, runtime);
}

void AnimationStringTable::CopyRecord(
    const std::uint32_t destination,
    const AnimationStringRecordState& source,
    IAnimationStringTableRuntime& runtime) {
    if (destination == 0) return;
    auto& target_name = runtime.FieldAt(destination);
    target_name.header = (target_name.header & 0xffffff00u) |
                         (source.name.header & 0xffu);
    target_name.text = 0;
    target_name.length = 0;
    target_name.capacity = 0;
    runtime.AssignTemplateField(destination, source.name);
    CopyValueField(destination + 16u, source.value, runtime);
}

void AnimationStringTable::AssignField(
    const std::uint32_t destination,
    const AnimationStringFieldState& source,
    IAnimationStringTableRuntime& runtime) {
    auto& target = runtime.FieldAt(destination);
    const auto length = source.length;
    if (&target == &source) {
        runtime.DetachText(destination);
        runtime.EraseText(destination, 0u, 0u);
        return;
    }
    if (length != 0u) {
        const auto source_text = source.text != 0u
                                     ? source.text : runtime.EmptyText();
        if (source_text != 0u &&
            runtime.ReadReference(source_text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            target.text = source.text != 0u
                              ? source.text : runtime.EmptyText();
            target.length = source.length;
            target.capacity = source.capacity;
            const auto reference = runtime.ReadReference(target.text - 1u);
            runtime.WriteReference(target.text - 1u,
                                   static_cast<std::uint8_t>(reference + 1u));
            return;
        }
    }
    if (runtime.AllocateText(destination, length, true)) {
        const auto source_text = source.text != 0u
                                     ? source.text : runtime.EmptyText();
        runtime.CopyText(target.text, source_text, length);
        runtime.SetTextSize(destination, length);
    }
}

std::uint32_t AnimationStringTable::AssignRecord(
    const std::uint32_t destination, const std::uint32_t source,
    IAnimationStringTableRuntime& runtime) {
    if (destination == 0 || source == 0) return destination;
    AssignField(destination, runtime.FieldAt(source), runtime);
    AssignField(destination + 16u, runtime.FieldAt(source + 16u), runtime);
    return destination;
}

void AnimationStringTableRuntimeStub::CopyRecord(
    const std::uint32_t destination, const std::uint32_t source) {
    AnimationStringTable::CopyRecord(destination, source, *this);
}

void AnimationStringTableRuntimeStub::CopyTemplateRecord(
    const std::uint32_t destination,
    const AnimationStringRecordState& source) {
    AnimationStringTable::CopyRecord(destination, source, *this);
}

void AnimationStringTableRuntimeStub::AssignRecordField(
    const std::uint32_t destination, const std::uint32_t source) {
    AnimationStringTable::AssignField(destination, FieldAt(source), *this);
}

void AnimationStringTableRuntimeStub::AssignTemplateField(
    const std::uint32_t destination,
    const AnimationStringFieldState& source) {
    AnimationStringTable::AssignField(destination, source, *this);
}

}  // namespace pal4
