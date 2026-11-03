#include "pal4/minimap_object_collection.h"

#include "pal4/minimap_icon_object.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t MinimapObjectCollectionState::ReadDword(
    const std::uint32_t offset) const {
    if (offset > kBytes - 4u) throw std::out_of_range("collection field");
    return static_cast<std::uint32_t>(bytes_[offset]) |
           (static_cast<std::uint32_t>(bytes_[offset + 1u]) << 8u) |
           (static_cast<std::uint32_t>(bytes_[offset + 2u]) << 16u) |
           (static_cast<std::uint32_t>(bytes_[offset + 3u]) << 24u);
}

void MinimapObjectCollectionState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) {
    if (offset > kBytes - 4u) throw std::out_of_range("collection field");
    for (std::uint32_t byte = 0u; byte < 4u; ++byte) {
        bytes_[offset + byte] =
            static_cast<std::uint8_t>(value >> (8u * byte));
    }
}

std::uint32_t MinimapObjectCollection::Construct(
    const std::uint32_t address,
    const std::uint8_t incoming_stack_byte,
    IMinimapObjectCollectionRuntime& runtime) {
    auto& state = runtime.Access(address);
    state.Bytes()[0] = incoming_stack_byte;
    for (std::uint32_t offset = 4u; offset <= 16u; offset += 4u) {
        state.WriteDword(offset, 0u);
    }
    return address;
}

int MinimapObjectCollection::Cleanup(
    const std::uint32_t address,
    IMinimapObjectCollectionRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto begin = state.ReadDword(4u);
    const auto end = state.ReadDword(8u);
    for (auto record = begin; record != end; record += 20u) {
        const auto element_begin = runtime.ReadDword(record + 8u);
        // The binary walks elements through nullsub_81; it has no effect.
        runtime.FreeRecordBuffer(element_begin);
        runtime.WriteDword(record + 8u, 0u);
        runtime.WriteDword(record + 12u, 0u);
        runtime.WriteDword(record + 16u, 0u);
    }
    runtime.FreeRecordArray(begin);
    state.WriteDword(4u, 0u);
    state.WriteDword(8u, 0u);
    state.WriteDword(12u, 0u);
    return 0;
}

bool MinimapObjectCollection::AddIcon(
    const std::uint32_t collection, const std::string_view name,
    const std::uint8_t name_pointer_low_byte,
    const std::uint32_t id_pointer,
    const std::string_view filename,
    const std::int32_t x, const std::int32_t y,
    IMinimapObjectAddRuntime& runtime) {
    auto group = runtime.FindGroup(collection, id_pointer);
    if (group == 0u) {
        MinimapObjectGroupRecord record;
        record.id_pointer = id_pointer;
        record.name_pointer_low_byte = name_pointer_low_byte;
        const auto signed_count = runtime.CollectionSize(record);
        const auto count = signed_count < 0 ? 0u :
            static_cast<std::uint32_t>(signed_count);
        record.begin = runtime.Allocate(count * 4u);
        record.end = runtime.CopyDwordArray(0u, 0u, record.begin);
        record.capacity = record.end;
        const auto insertion_address = runtime.ReadDword(collection + 8u);
        try {
            runtime.InsertGroup(collection, insertion_address, record);
        } catch (...) {
            runtime.Free(record.begin);
            runtime.Free(0u);
            throw;
        }
        runtime.Free(record.begin);
        runtime.Free(0u);
        group = runtime.FindGroup(collection, id_pointer);
        if (group == 0u) return false;
    }

    const auto group_collection = group + 4u;
    if (runtime.FindIconByName(group_collection, name)) return false;
    const auto allocated = runtime.Allocate(0x48u);
    std::uint32_t icon = 0u;
    if (allocated != 0u) {
        try {
            icon = runtime.ConstructIcon(allocated);
        } catch (...) {
            runtime.Free(allocated);
            throw;
        }
    }
    if (!runtime.LoadIconTexture(icon, filename, id_pointer)) {
        runtime.DestroyIcon(icon);
        return false;
    }
    runtime.SetIconName(icon + 8u, name);
    runtime.SetIconCenter(icon, x, y);
    runtime.InsertIcon(group_collection,
                       runtime.ReadDword(group + 12u), icon);
    return true;
}

std::uint32_t MinimapObjectCollection::RemoveIcon(
    const std::uint32_t collection, const std::string_view name,
    const std::uint32_t id_pointer,
    IMinimapObjectRemoveRuntime& runtime) {
    const auto group = runtime.FindGroup(collection, id_pointer);
    if (group == 0u) return 0u;
    const auto begin = runtime.ReadDword(group + 8u);
    const auto end = runtime.ReadDword(group + 12u);
    for (auto slot = begin; slot != end; slot += 4u) {
        const auto icon = runtime.ReadDword(slot);
        if (!runtime.IconNameEquals(icon, name)) continue;
        runtime.DestroyIcon(icon);
        for (auto source = slot + 4u; source != end; source += 4u) {
            runtime.WriteDword(source - 4u,
                               runtime.ReadDword(source));
        }
        runtime.WriteDword(group + 12u, end - 4u);
        return end;
    }
    return 0u;
}

int MinimapObjectCollection::ClearIcons(
    MinimapObjectCollectionState& state,
    IMinimapObjectClearRuntime& runtime) {
    const auto begin = state.ReadDword(4u);
    const auto end = state.ReadDword(8u);
    for (auto group = begin; group != end; group += 20u) {
        for (std::uint32_t index = 0u;; ++index) {
            const auto icon_begin = runtime.ReadDword(group + 8u);
            if (icon_begin == 0u) break;
            const auto icon_end = runtime.ReadDword(group + 12u);
            if (index >= (icon_end - icon_begin) / 4u) break;
            runtime.DestroyIcon(runtime.ReadDword(icon_begin + index * 4u));
        }
    }
    int result = 0;
    for (auto group = begin; group != end; group += 20u) {
        result = runtime.CleanupGroup(group);
    }
    state.WriteDword(8u, begin);
    state.WriteDword(16u, 0u);
    return result;
}

std::uint32_t MinimapObjectCollection::ClearSection(
    const std::uint32_t collection,
    const std::uint32_t id_pointer,
    IMinimapObjectSectionRuntime& runtime) {
    const auto group = runtime.FindGroup(collection, id_pointer);
    if (group == 0u) return 0u;
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = runtime.ReadDword(group + 8u);
        if (begin == 0u) break;
        const auto end = runtime.ReadDword(group + 12u);
        if (index >= (end - begin) / 4u) break;
        runtime.DestroyIcon(runtime.ReadDword(begin + index * 4u));
    }
    const auto old_end = runtime.ReadDword(group + 12u);
    runtime.WriteDword(group + 12u, runtime.ReadDword(group + 8u));
    return old_end;
}

std::uint32_t MinimapObjectCollection::DrawIcons(
    const MinimapObjectCollectionState& state,
    const std::uint32_t context,
    const std::int32_t x, const std::int32_t y,
    const std::uint32_t bounds_address,
    IMinimapObjectDrawRuntime& runtime) {
    const auto begin = state.ReadDword(4u);
    if (begin == 0u) return 0u;
    const auto group_count = (state.ReadDword(8u) - begin) / 20u;
    for (std::uint32_t group_index = 0u;
         group_index < group_count; ++group_index) {
        const auto group = state.ReadDword(4u) + group_index * 20u;
        for (std::uint32_t icon_index = 0u;; ++icon_index) {
            const auto icon_begin = runtime.ReadDword(group + 8u);
            if (icon_begin == 0u) break;
            const auto icon_end = runtime.ReadDword(group + 12u);
            if (icon_index >= (icon_end - icon_begin) / 4u) break;
            runtime.DrawIcon(
                runtime.ReadDword(icon_begin + icon_index * 4u),
                context, x, y, bounds_address, false, 0u);
        }
    }
    return 0u;
}

std::uint32_t MinimapObjectCollection::FindIconByName(
    const std::uint32_t group_collection,
    const std::string_view name,
    IMinimapObjectLookupRuntime& runtime) {
    const auto begin = runtime.ReadDword(group_collection + 4u);
    if (begin == 0u) return 0u;
    const auto end = runtime.ReadDword(group_collection + 8u);
    const auto count = (end - begin) / 4u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto icon = runtime.ReadDword(begin + index * 4u);
        const auto name_pointer = runtime.ReadDword(icon + 8u);
        const auto stored_length = runtime.ReadDword(icon + 12u);
        if (stored_length != name.size()) continue;
        bool equal = true;
        for (std::uint32_t byte = 0u; byte <= stored_length; ++byte) {
            const auto expected = byte == name.size() ? 0u :
                static_cast<std::uint8_t>(name[byte]);
            if (runtime.ReadByte(name_pointer + byte) != expected) {
                equal = false;
                break;
            }
        }
        if (equal) return icon;
    }
    return 0u;
}

bool MinimapObjectCollection::SetActiveIcon(
    MinimapObjectCollectionState& state,
    const std::string_view name,
    const std::uint32_t id_pointer,
    IMinimapObjectActiveRuntime& runtime) {
    const auto group = FindGroupById(state, id_pointer, runtime);
    if (group == 0u) return false;
    const auto prior = state.ReadDword(16u);
    if (prior != 0u) runtime.WriteByte(prior + 0x44u, 0u);
    const auto selected = FindIconByName(group + 4u, name, runtime);
    state.WriteDword(16u, selected);
    if (selected != 0u) runtime.WriteByte(selected + 0x44u, 1u);
    return selected != 0u;
}

std::uint32_t MinimapObjectCollection::FindGroupById(
    const MinimapObjectCollectionState& state,
    const std::uint32_t id_pointer,
    IMinimapObjectLookupRuntime& runtime) {
    const auto begin = state.ReadDword(4u);
    if (begin == 0u) return 0u;
    const auto count = (state.ReadDword(8u) - begin) / 20u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto group = begin + index * 20u;
        if (runtime.ReadDword(group) == id_pointer) return group;
    }
    return 0u;
}

std::uint32_t MinimapObjectCollection::FindIconAtPoint(
    const MinimapObjectCollectionState& state,
    const std::int32_t x, const std::int32_t y,
    const std::uint32_t id_pointer,
    IMinimapObjectHitRuntime& runtime) {
    const auto group = FindGroupById(state, id_pointer, runtime);
    if (group == 0u) return 0u;
    const auto begin = runtime.ReadDword(group + 8u);
    if (begin == 0u) return 0u;
    for (std::uint32_t index = 0u;; ++index) {
        const auto end = runtime.ReadDword(group + 12u);
        if (index >= (end - begin) / 4u) break;
        const auto icon = runtime.ReadDword(begin + index * 4u);
        if (MinimapIconObject::ContainsPoint(
                runtime.IconState(icon), x, y)) {
            return icon;
        }
    }
    return 0u;
}

}  // namespace pal4
