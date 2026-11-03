#include "pal4/rw_geometry_plugin.h"

#include <cstdint>
#include <cstring>

namespace pal4 {
namespace {

void ClearData(const RwGeometryPluginState& state,
               const std::uint32_t object_address,
               IRwGeometryPluginLifecycle& runtime) {
    if (state.geometry_offset <= 0)
        return;
    const auto base = object_address +
                      static_cast<std::uint32_t>(state.geometry_offset);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.WriteWord(base + 4u * index, 0u);
}

}  // namespace

bool RwGeometryPlugin::Register(
    RwGeometryPluginState& state, IRwGeometryPluginLifecycle& runtime) {
    state.engine_offset = runtime.RegisterEnginePlugin(1u, kPluginId);
    if (state.engine_offset < 0)
        return false;
    state.geometry_offset = runtime.RegisterGeometryPlugin(12u, kPluginId);
    return state.geometry_offset >= 0 &&
           runtime.RegisterGeometryStream(kPluginId) ==
               state.geometry_offset;
}

std::uint32_t RwGeometryPlugin::ConstructData(
    const RwGeometryPluginState& state,
    const std::uint32_t object_address,
    IRwGeometryPluginLifecycle& runtime) {
    ClearData(state, object_address, runtime);
    return object_address;
}

std::uint32_t RwGeometryPlugin::DestructData(
    const RwGeometryPluginState& state,
    const std::uint32_t object_address,
    IRwGeometryPluginLifecycle& runtime) {
    ClearData(state, object_address, runtime);
    return object_address;
}

std::uint32_t RwGeometryPlugin::CopyData(
    const RwGeometryPluginState& state, const std::uint32_t destination,
    const std::uint32_t source, IRwGeometryPluginLifecycle& runtime) {
    const auto offset = static_cast<std::uint32_t>(state.geometry_offset);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.WriteWord(destination + offset + 4u * index,
                          runtime.ReadWord(source + offset + 4u * index));
    return destination;
}

std::uint32_t RwGeometryPlugin::ProcessClumpAtomics(
    const RwGeometryPluginState& state, const std::uint32_t clump_address,
    const std::uint32_t payload, IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0 || state.engine_offset <= 0)
        return 0;
    RwAtomicProcessContext context{
        0u, runtime.GeometryRenderCallbackAddress(), payload};
    runtime.ForAllAtomics(clump_address, 0x65fd70u, &context);
    return context.material_match;
}

std::uint32_t RwGeometryPlugin::ProcessAtomicGeometry(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    IRwGeometryPluginRuntime& runtime) {
    if (GetAtomicGeometryData(state, atomic_address, runtime) == 0u)
        return atomic_address;
    const float distance = CalculateDistanceToCamera(atomic_address, runtime);
    const auto array = GetAtomicGeometryDataArray(state, atomic_address, runtime);
    if (array == 0u)
        return atomic_address;

    auto begin = runtime.ReadWord(array + 4u);
    auto end = runtime.ReadWord(array + 8u);
    const auto capacity_end = runtime.ReadWord(array + 12u);
    std::uint32_t distance_bits = 0u;
    static_assert(sizeof(distance_bits) == sizeof(distance));
    std::memcpy(&distance_bits, &distance, sizeof(distance_bits));

    if (capacity_end - end < 8u) {
        const auto size = begin == 0u ? 0u : (end - begin) / 8u;
        const auto growth = size > 1u ? size : 1u;
        const auto new_begin = runtime.AllocateBytes((size + growth) * 8u);
        for (std::uint32_t index = 0u; index < size; ++index) {
            runtime.WriteWord(new_begin + 8u * index,
                              runtime.ReadWord(begin + 8u * index));
            runtime.WriteWord(new_begin + 8u * index + 4u,
                              runtime.ReadWord(begin + 8u * index + 4u));
        }
        runtime.FreeBytes(begin);
        begin = new_begin;
        end = begin + size * 8u;
        runtime.WriteWord(array + 4u, begin);
        runtime.WriteWord(array + 12u, begin + (size + growth) * 8u);
    }
    runtime.WriteWord(end, atomic_address);
    runtime.WriteWord(end + 4u, distance_bits);
    runtime.WriteWord(array + 8u, end + 8u);
    return atomic_address;
}

std::uint32_t RwGeometryPlugin::GetAtomicGeometryData(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0 || state.engine_offset <= 0)
        return 0u;
    return runtime.ReadWord(atomic_address +
                            static_cast<std::uint32_t>(state.geometry_offset) + 4u);
}

std::uint32_t RwGeometryPlugin::GetAtomicGeometryDataArray(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0 || state.engine_offset <= 0)
        return 0u;
    return runtime.ReadWord(atomic_address +
                            static_cast<std::uint32_t>(state.geometry_offset) + 8u);
}

float RwGeometryPlugin::CalculateDistanceToCamera(
    const std::uint32_t atomic_address, IRwGeometryPluginRuntime& runtime) {
    if ((runtime.ReadWord(atomic_address + 76u) & 2u) != 0u)
        runtime.ResyncInterpolatedSphere(atomic_address);
    const auto frame = runtime.ReadWord(atomic_address + 4u);
    const auto transform = runtime.GetObjectTransform(frame);
    const auto position = runtime.CalculatePosition(atomic_address + 28u,
                                                     transform);
    const auto camera = runtime.ActiveCameraAddress();
    const auto camera_frame = runtime.ReadWord(camera + 4u);
    const auto camera_transform = runtime.GetObjectTransform(camera_frame);
    const auto read_float = [&runtime](const std::uint32_t address) {
        const auto bits = runtime.ReadWord(address);
        float value = 0.0f;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    };
    const auto dx = position.x - read_float(camera_transform + 48u);
    const auto dy = position.y - read_float(camera_transform + 52u);
    const auto dz = position.z - read_float(camera_transform + 56u);
    return dx * dx + dy * dy + dz * dz;
}

std::uint32_t RwGeometryPlugin::ProcessAtomicMaterial(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    RwAtomicProcessContext& context, IRwGeometryPluginRuntime& runtime) {
    static_cast<void>(GetAtomicGeometryMaterial(atomic_address, context, runtime));
    if (context.material_match == 0u)
        return atomic_address;
    const auto previous_callback = runtime.ReadWord(atomic_address + 72u);
    if (previous_callback == context.geometry_callback ||
        GetAtomicGeometryData(state, atomic_address, runtime) != 0u)
        return atomic_address;
    static_cast<void>(SetAtomicGeometryData(state, atomic_address,
                                            previous_callback, runtime));
    static_cast<void>(SetAtomicGeometryFlag(state, atomic_address, 1u, runtime));
    static_cast<void>(SetAtomicGeometryPayload(state, atomic_address,
                                               context.payload, runtime));
    runtime.WriteWord(atomic_address + 72u,
                      context.geometry_callback == 0u
                          ? runtime.DefaultAtomicRenderCallback()
                          : context.geometry_callback);
    return atomic_address;
}

namespace {

std::int32_t SetAtomicField(const RwGeometryPluginState& state,
                            const std::uint32_t atomic_address,
                            const std::uint32_t field_offset,
                            const std::uint32_t value,
                            IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset > 0 && state.engine_offset > 0)
        runtime.WriteWord(atomic_address +
                              static_cast<std::uint32_t>(state.geometry_offset) +
                              field_offset,
                          value);
    return state.geometry_offset;
}

}  // namespace

std::int32_t RwGeometryPlugin::SetAtomicGeometryData(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    const std::uint32_t value, IRwGeometryPluginRuntime& runtime) {
    return SetAtomicField(state, atomic_address, 0u, value, runtime);
}

std::int32_t RwGeometryPlugin::SetAtomicGeometryFlag(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    const std::uint32_t value, IRwGeometryPluginRuntime& runtime) {
    return SetAtomicField(state, atomic_address, 4u, value, runtime);
}

std::int32_t RwGeometryPlugin::SetAtomicGeometryPayload(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    const std::uint32_t value, IRwGeometryPluginRuntime& runtime) {
    return SetAtomicField(state, atomic_address, 8u, value, runtime);
}

std::uint32_t RwGeometryPlugin::GetAtomicGeometryMaterial(
    const std::uint32_t atomic_address, RwAtomicProcessContext& context,
    IRwGeometryPluginRuntime& runtime) {
    const auto geometry = runtime.ReadWord(atomic_address + 24u);
    context.material_match = CheckMaterialAlpha(geometry, runtime) ? 1u : 0u;
    if (context.material_match == 0u)
        runtime.ForAllMaterials(geometry, 0x65fea0u, context);
    return atomic_address;
}

bool RwGeometryPlugin::CheckMaterialAlpha(
    const std::uint32_t geometry_address, IRwGeometryPluginRuntime& runtime) {
    if ((runtime.ReadWord(geometry_address + 8u) & 8u) == 0u)
        return false;
    auto remaining = runtime.ReadWord(geometry_address + 20u);
    if (remaining == 0u)
        return false;
    const auto colors = runtime.ReadWord(geometry_address + 48u);
    do {
        --remaining;
        if ((runtime.ReadWord(colors + 4u * remaining) >> 24u) != 0xffu)
            return true;
    } while (remaining != 0u);
    return false;
}

std::uint32_t RwGeometryPlugin::ProcessMaterial(
    const std::uint32_t material_address, RwAtomicProcessContext& context,
    IRwGeometryPluginRuntime& runtime) {
    if ((runtime.ReadWord(material_address + 4u) >> 24u) != 0xffu) {
        context.material_match = 1u;
    } else if (context.material_match == 0u) {
        const auto texture = runtime.ReadWord(material_address);
        if (texture != 0u)
            context.material_match = ProcessImageAlpha(texture, runtime) ? 1u : 0u;
    }
    return material_address;
}

bool RwGeometryPlugin::ProcessImageAlpha(
    const std::uint32_t texture_address, IRwGeometryPluginRuntime& runtime) {
    const auto raster = runtime.ReadWord(texture_address);
    if (raster == 0u)
        return false;
    if (const auto compressed =
            runtime.CompressedTextureHasAlpha(texture_address))
        return *compressed;
    const auto format = runtime.ReadByte(raster + 35u);
    const auto format_kind = format & 0x0fu;
    if (format_kind != 1u && format_kind != 3u && format_kind != 5u)
        return false;

    const auto width = runtime.ReadWord(raster + 12u);
    const auto height = runtime.ReadWord(raster + 16u);
    const bool indexed = (format & 0x60u) != 0u;
    const auto depth = indexed ? runtime.ReadWord(raster + 20u) : 32u;
    const auto image = runtime.CreateImage(width, height, depth);
    if (image == 0u)
        return false;
    runtime.AllocateImagePixels(image);
    runtime.SetImageFromRaster(image, raster);

    bool has_alpha = false;
    if (indexed) {
        const auto count = std::uint32_t{1} << (depth & 31u);
        if (static_cast<std::int32_t>(count) > 0) {
            const auto pixels = runtime.ReadWord(image + 24u);
            for (std::uint32_t index = 0u; index < count; ++index) {
                if (runtime.ReadByte(pixels + 4u * index + 3u) != 0xffu) {
                    has_alpha = true;
                    break;
                }
            }
        }
    } else if (static_cast<std::int32_t>(height) > 0) {
        auto row = runtime.ReadWord(image + 20u);
        for (std::uint32_t y = 0u; y < height && !has_alpha; ++y) {
            if (static_cast<std::int32_t>(width) > 0) {
                for (std::uint32_t x = 0u; x < width; ++x) {
                    if (runtime.ReadByte(row + 4u * x + 3u) != 0xffu) {
                        has_alpha = true;
                        break;
                    }
                }
            }
            row += runtime.ReadWord(image + 16u);
        }
    }
    runtime.DestroyImage(image);
    return has_alpha;
}

bool RwGeometryPlugin::ProcessClumpAtomicsWrapper(
    const RwGeometryPluginState& state, const std::uint32_t clump_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0 || state.engine_offset <= 0)
        return false;
    runtime.ForAllAtomics(clump_address, 0x660060u, nullptr);
    return true;
}

std::uint32_t RwGeometryPlugin::ProcessAtomic(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    IRwGeometryPluginRuntime& runtime) {
    if (GetAtomicGeometryData(state, atomic_address, runtime) == 0u)
        return atomic_address;
    static_cast<void>(SetAtomicGeometryFlag(state, atomic_address, 0u, runtime));
    const auto previous_callback = GetAtomicGeometryFlag(state, atomic_address,
                                                          runtime);
    runtime.WriteWord(atomic_address + 72u,
                      previous_callback == 0u
                          ? runtime.DefaultAtomicRenderCallback()
                          : previous_callback);
    const auto array = GetAtomicGeometryDataArray(state, atomic_address, runtime);
    if (array == 0u)
        return atomic_address;
    const auto begin = runtime.ReadWord(array + 4u);
    const auto end = runtime.ReadWord(array + 8u);
    if (begin == end)
        return atomic_address;
    auto entry = begin;
    while (entry != end && runtime.ReadWord(entry) != atomic_address)
        entry += 8u;
    if (entry == end)
        return atomic_address;
    for (auto source = entry + 8u; source != end; source += 8u) {
        runtime.WriteWord(source - 8u, runtime.ReadWord(source));
        runtime.WriteWord(source - 4u, runtime.ReadWord(source + 4u));
    }
    runtime.WriteWord(array + 8u, end - 8u);
    return atomic_address;
}

std::uint32_t RwGeometryPlugin::GetAtomicGeometryFlag(
    const RwGeometryPluginState& state, const std::uint32_t atomic_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0 || state.engine_offset <= 0)
        return 0u;
    return runtime.ReadWord(atomic_address +
                            static_cast<std::uint32_t>(state.geometry_offset));
}

bool RwGeometryPlugin::CompareAtomicGeometryFlags(
    const std::uint32_t first_record, const std::uint32_t second_record,
    IRwGeometryPluginRuntime& runtime) {
    const auto first_bits = runtime.ReadWord(first_record + 4u);
    const auto second_bits = runtime.ReadWord(second_record + 4u);
    float first = 0.0f;
    float second = 0.0f;
    std::memcpy(&first, &first_bits, sizeof(first));
    std::memcpy(&second, &second_bits, sizeof(second));
    return first > second;
}

namespace {

struct AtomicDistanceRecord {
    std::uint32_t atomic;
    std::uint32_t distance_bits;
};

AtomicDistanceRecord ReadRecord(const std::uint32_t address,
                                IRwGeometryPluginRuntime& runtime) {
    return {runtime.ReadWord(address), runtime.ReadWord(address + 4u)};
}

void WriteRecord(const std::uint32_t address, const AtomicDistanceRecord record,
                 IRwGeometryPluginRuntime& runtime) {
    runtime.WriteWord(address, record.atomic);
    runtime.WriteWord(address + 4u, record.distance_bits);
}

bool GreaterRecord(const AtomicDistanceRecord first,
                   const AtomicDistanceRecord second) {
    float a = 0.0f;
    float b = 0.0f;
    std::memcpy(&a, &first.distance_bits, sizeof(a));
    std::memcpy(&b, &second.distance_bits, sizeof(b));
    return a > b;
}

AtomicDistanceRecord MedianRecord(const AtomicDistanceRecord first,
                                  const AtomicDistanceRecord middle,
                                  const AtomicDistanceRecord last) {
    if (GreaterRecord(first, middle)) {
        if (GreaterRecord(middle, last))
            return middle;
        return GreaterRecord(first, last) ? last : first;
    }
    if (GreaterRecord(first, last))
        return first;
    return GreaterRecord(middle, last) ? last : middle;
}

}  // namespace

std::uint32_t RwGeometryPlugin::QuickSortAtomicGeometries(
    std::uint32_t begin, std::uint32_t end,
    IRwGeometryPluginRuntime& runtime) {
    while ((end - begin) / 8u > 16u) {
        const auto count = (end - begin) / 8u;
        const auto pivot = MedianRecord(ReadRecord(begin, runtime),
                                        ReadRecord(begin + (count / 2u) * 8u,
                                                   runtime),
                                        ReadRecord(end - 8u, runtime));
        auto left = begin;
        auto right = end;
        for (;;) {
            while (GreaterRecord(ReadRecord(left, runtime), pivot))
                left += 8u;
            do {
                right -= 8u;
            } while (GreaterRecord(pivot, ReadRecord(right, runtime)));
            if (right <= left)
                break;
            const auto left_record = ReadRecord(left, runtime);
            WriteRecord(left, ReadRecord(right, runtime), runtime);
            WriteRecord(right, left_record, runtime);
            left += 8u;
        }
        if (end - left > left - begin) {
            static_cast<void>(QuickSortAtomicGeometries(begin, left, runtime));
            begin = left;
        } else {
            static_cast<void>(QuickSortAtomicGeometries(left, end, runtime));
            end = left;
        }
    }
    return (end - begin) / 8u;
}

std::uint32_t RwGeometryPlugin::InsertionSortAtomicGeometries(
    const std::uint32_t begin, const std::uint32_t end,
    IRwGeometryPluginRuntime& runtime) {
    for (auto current = begin + 8u; current < end; current += 8u) {
        const auto item = ReadRecord(current, runtime);
        auto destination = current;
        while (destination > begin &&
               GreaterRecord(item, ReadRecord(destination - 8u, runtime))) {
            WriteRecord(destination,
                        ReadRecord(destination - 8u, runtime), runtime);
            destination -= 8u;
        }
        WriteRecord(destination, item, runtime);
    }
    return end;
}

std::int32_t RwGeometryPlugin::SortAtomicGeometries(
    const RwGeometryPluginState& state, const std::uint32_t array_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0)
        return state.geometry_offset;
    if (state.engine_offset <= 0)
        return state.engine_offset;
    const auto begin = runtime.ReadWord(array_address + 4u);
    if (begin == 0u)
        return state.engine_offset;
    const auto end = runtime.ReadWord(array_address + 8u);
    const auto count = (end - begin) / 8u;
    if (count == 0u)
        return 0;

    if (count > 16u) {
        static_cast<void>(QuickSortAtomicGeometries(begin, end, runtime));
        static_cast<void>(InsertionSortAtomicGeometries(begin, begin + 128u,
                                                        runtime));
        for (auto current = begin + 128u; current < end; current += 8u) {
            const auto item = ReadRecord(current, runtime);
            auto destination = current;
            while (destination > begin &&
                   GreaterRecord(item, ReadRecord(destination - 8u, runtime))) {
                WriteRecord(destination,
                            ReadRecord(destination - 8u, runtime), runtime);
                destination -= 8u;
            }
            WriteRecord(destination, item, runtime);
        }
    } else {
        static_cast<void>(InsertionSortAtomicGeometries(begin, end, runtime));
    }
    for (auto current = runtime.ReadWord(array_address + 4u);
         current != runtime.ReadWord(array_address + 8u); current += 8u) {
        const auto atomic = runtime.ReadWord(current);
        const auto callback = GetAtomicGeometryFlag(state, atomic, runtime);
        if (callback != 0u)
            runtime.InvokeAtomicRenderCallback(callback, atomic);
    }
    return ClearAtomicGeometryArray(state, array_address, runtime);
}

std::int32_t RwGeometryPlugin::ClearAtomicGeometryArray(
    const RwGeometryPluginState& state, const std::uint32_t array_address,
    IRwGeometryPluginRuntime& runtime) {
    if (state.geometry_offset <= 0)
        return state.geometry_offset;
    if (state.engine_offset <= 0)
        return state.engine_offset;
    const auto old_end = runtime.ReadWord(array_address + 8u);
    runtime.WriteWord(array_address + 8u, runtime.ReadWord(array_address + 4u));
    return static_cast<std::int32_t>(old_end);
}

}  // namespace pal4
