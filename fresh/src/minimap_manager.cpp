#include "pal4/minimap_manager.h"

#include <bit>
#include <new>

namespace pal4 {
namespace {

std::uint32_t ReplaceTexture(
    MinimapManagerObjectState& state, const std::uint32_t grid,
    const std::array<std::uint32_t, 4>& image_region,
    IMinimapManagerObjectRuntime& runtime) {
    const std::uint32_t old_texture = state.ReadDword(0x28u);
    if (old_texture != 0u) {
        runtime.ResetTexture(old_texture);
        runtime.Free(old_texture);
    }
    const bool mode = state.ReadDword(0x50u) == 2u;
    const std::uint32_t texture = runtime.Allocate(0x2cu);
    if (texture == 0u) {
        return 0u;
    }
    try {
        return runtime.InitializeTexture(texture, image_region, grid, mode);
    } catch (...) {
        runtime.Free(texture);
        throw;
    }
}

}  // namespace

std::uint32_t MinimapManagerAccess::Get(
    MinimapManagerAccessState& state,
    IMinimapManagerAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palColorMiniMapManager", 0x40bu,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t MinimapManagerObjectState::ReadDword(
    const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void MinimapManagerObjectState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t MinimapManagerObject::Create(
    const std::uint8_t incoming_stack_byte,
    IMinimapManagerObjectRuntime& runtime) {
    const std::uint32_t address =
        runtime.Allocate(MinimapManagerObjectState::kBytes);
    if (address == 0u) {
        return 0u;
    }
    try {
        return Construct(address, incoming_stack_byte, runtime);
    } catch (...) {
        runtime.Free(address);
        throw;
    }
}

std::uint32_t MinimapManagerObject::Construct(
    const std::uint32_t address,
    const std::uint8_t incoming_stack_byte,
    IMinimapManagerObjectRuntime& runtime) {
    runtime.BaseConstruct(address);
    auto& state = runtime.Access(address);
    state.Bytes().at(0x14u) = incoming_stack_byte;
    for (std::uint32_t offset = 0x18u; offset <= 0x48u; offset += 4u) {
        state.WriteDword(offset, 0u);
    }
    state.WriteDword(0x50u, 2u);
    state.WriteDword(0x4cu, 1u);
    state.Bytes().at(0x54u) = 1u;
    state.Bytes().at(0x55u) = 0xffu;
    return address;
}

void MinimapManagerObject::Destruct(
    const std::uint32_t address,
    IMinimapManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    runtime.Free(state.ReadDword(0x18u));
    state.WriteDword(0x18u, 0u);
    state.WriteDword(0x1cu, 0u);
    state.WriteDword(0x20u, 0u);
    runtime.CleanupCollection(address);
}

void MinimapManagerObject::Destroy(
    const std::uint32_t address,
    IMinimapManagerObjectRuntime& runtime) {
    if (address == 0u) {
        return;
    }
    Destruct(address, runtime);
    runtime.Free(address);
}

std::uint32_t MinimapManagerObject::Reset(
    const std::uint32_t address,
    IMinimapManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    const std::uint32_t texture = state.ReadDword(0x28u);
    if (texture != 0u) {
        runtime.ResetTexture(texture);
        const std::uint32_t current = state.ReadDword(0x28u);
        if (current != 0u) {
            runtime.ResetTexture(current);
            runtime.Free(current);
        }
        state.WriteDword(0x28u, 0u);
    }
    state.WriteDword(0x24u, 0u);
    for (std::uint32_t index = 0u;; ++index) {
        const std::uint32_t first = state.ReadDword(0x18u);
        if (first == 0u) {
            break;
        }
        const std::uint32_t count = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(state.ReadDword(0x1cu) - first) >> 2u);
        if (index >= count) {
            break;
        }
        const std::uint32_t grid = runtime.ReadDword(first + index * 4u);
        if (grid != 0u) {
            runtime.CleanupGrid(grid);
            runtime.Free(grid);
        }
    }
    state.WriteDword(0x1cu, state.ReadDword(0x18u));
    return runtime.CleanupObjects(address);
}

void MinimapManagerObject::SetupTexture(
    const std::uint32_t address,
    const std::array<std::uint32_t, 4>& region,
    IMinimapManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    const std::uint32_t grid = state.ReadDword(0x24u);
    if (grid == 0u || runtime.GridPixelBuffer(grid) == 0u) {
        return;
    }
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.WriteDword(0x2cu + index * 4u, region[index]);
    }
    const std::array<std::uint32_t, 4> image_region{
        0u, 0u, region[2], region[3]};
    state.WriteDword(0x28u, ReplaceTexture(
        state, grid, image_region, runtime));
}

void MinimapManagerObject::SetTextureByIndex(
    const std::uint32_t address, const std::uint32_t index,
    IMinimapManagerObjectRuntime& runtime) {
    if (index == 0xffffffffu) {
        return;
    }
    auto& state = runtime.Access(address);
    const std::uint32_t first = state.ReadDword(0x18u);
    if (first == 0u) {
        return;
    }
    const std::uint32_t count = static_cast<std::uint32_t>(
        static_cast<std::int32_t>(state.ReadDword(0x1cu) - first) >> 2u);
    if (count == 0u) {
        return;
    }
    if (index >= count) {
        runtime.LogInvalidGridIndex(index);
        return;
    }
    const std::uint32_t grid = runtime.ReadDword(first + index * 4u);
    state.WriteDword(0x24u, grid);
    const std::array<std::uint32_t, 4> image_region{
        0u, 0u, state.ReadDword(0x34u), state.ReadDword(0x38u)};
    state.WriteDword(0x28u, ReplaceTexture(
        state, grid, image_region, runtime));
    runtime.InitializeDialogSystem(address);
}

std::uint32_t MinimapManagerObject::CurrentGridIndex(
    const std::uint32_t address,
    IMinimapManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    for (std::uint32_t index = 0u;; ++index) {
        const std::uint32_t first = state.ReadDword(0x18u);
        if (first == 0u) {
            break;
        }
        const std::uint32_t count = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(state.ReadDword(0x1cu) - first) >> 2u);
        if (index >= count) {
            break;
        }
        const std::uint32_t selected = state.ReadDword(0x24u);
        if (runtime.ReadDword(first + index * 4u) == selected) {
            return index;
        }
    }
    return 0xffffffffu;
}

MinimapManagerObjectState& MinimapManagerDrawStub::Access(std::uint32_t) {
    return state_;
}
std::uint32_t MinimapManagerDrawStub::TextureField(
    std::uint32_t, std::uint32_t) {
    return 0u;
}
void MinimapManagerDrawStub::CommitTextureRaster(std::uint32_t) {}
void MinimapManagerDrawStub::DrawFilledRectangle(
    std::uint32_t, const std::array<std::uint32_t, 4>&,
    const std::array<std::uint8_t, 3>&, std::uint8_t) {}
void MinimapManagerDrawStub::DrawTexturedRectangle(
    std::uint32_t, const std::array<std::uint32_t, 4>&,
    std::uint32_t, const std::array<std::uint8_t, 3>&,
    std::uint8_t, bool) {}
std::uint32_t MinimapManagerDrawStub::DrawMinimapTextures(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t) {
    return 0u;
}
std::uint32_t MinimapManagerDrawStub::FindObjectAt(
    std::uint32_t, std::uint32_t, std::uint32_t) {
    return 0u;
}

std::uint32_t MinimapManagerObject::Draw(
    const std::uint32_t address, const std::uint32_t context,
    IMinimapManagerDrawRuntime& runtime) {
    auto& state = runtime.Access(address);
    if (state.Bytes().at(0x54u) == 0u) {
        return 0u;
    }
    const std::uint32_t texture_state = state.ReadDword(0x28u);
    if (texture_state == 0u) {
        return 0u;
    }
    if (runtime.TextureField(texture_state, 0x28u) == 0u) {
        return texture_state;
    }
    runtime.CommitTextureRaster(texture_state);
    const std::array<std::uint32_t, 4> region{
        state.ReadDword(0x2cu), state.ReadDword(0x30u),
        state.ReadDword(0x34u), state.ReadDword(0x38u)};
    const auto alpha = state.Bytes().at(0x55u);
    const bool alpha_mode = state.ReadDword(0x50u) == 2u;
    runtime.DrawFilledRectangle(context, region, {0u, 0u, 0u}, alpha);
    runtime.DrawTexturedRectangle(
        context, region, runtime.TextureField(texture_state, 0x28u),
        {0xffu, 0xffu, 0xffu}, alpha, alpha_mode);
    return runtime.DrawMinimapTextures(
        address, context, region[0], region[1], texture_state + 4u);
}

std::uint32_t MinimapManagerObject::FindObjectWithOffset(
    const std::uint32_t address, const std::uint32_t x,
    const std::uint32_t y, const std::uint32_t context,
    IMinimapManagerDrawRuntime& runtime) {
    auto& state = runtime.Access(address);
    const std::uint32_t texture = state.ReadDword(0x28u);
    if (texture == 0u) {
        return 0u;
    }
    const std::uint32_t offset_x = runtime.TextureField(texture, 4u);
    const std::uint32_t offset_y = runtime.TextureField(texture, 8u);
    return runtime.FindObjectAt(x + offset_x, y + offset_y, context);
}

void MinimapManagerObject::UpdateGridDisplay(
    const std::uint32_t address, const std::array<float, 3>& world,
    const std::array<std::uint32_t, 3>& data,
    IMinimapManagerUpdateRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto coordinate_object = state.ReadDword(0x10u);
    if (coordinate_object == 0u) {
        return;
    }
    const auto grid = state.ReadDword(0x24u);
    if (grid == 0u || runtime.GridPixelBuffer(grid) == 0u) {
        return;
    }

    const auto coordinates = runtime.WorldToGrid(grid, world);
    const float x = coordinates[0];
    const float y = coordinates[1];
    if (!(x >= 0.0f) || !(y >= 0.0f) ||
        !(static_cast<double>(runtime.GridWidth(grid)) > x) ||
        !(static_cast<double>(runtime.GridHeight(grid)) > y)) {
        return;
    }

    const auto first_y = runtime.FloatToInt(y);
    const auto first_x = runtime.FloatToInt(x);
    runtime.SetCoordinates(coordinate_object, first_x, first_y);
    runtime.CopyThreeDwords(coordinate_object, data);

    const auto grid_x = runtime.FloatToInt(x);
    const auto grid_y = runtime.FloatToInt(y);
    const auto region_width = std::bit_cast<std::int32_t>(
        state.ReadDword(0x34u));
    const auto region_height = std::bit_cast<std::int32_t>(
        state.ReadDword(0x38u));
    const auto left = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(grid_x) -
        static_cast<std::uint32_t>(region_width / 2));
    const auto top = std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(grid_y) -
        static_cast<std::uint32_t>(region_height / 2));

    bool radius_changed = false;
    if (state.ReadDword(0x50u) == 2u) {
        radius_changed = runtime.ApplyRadius(grid, grid_x, grid_y);
    }
    const auto texture = state.ReadDword(0x28u);
    if (texture != 0u) {
        const bool position_changed =
            runtime.UpdateTexturePosition(texture, left, top);
        if (position_changed || radius_changed) {
            runtime.RefreshTexture(texture);
        }
    }
}

bool MinimapManagerObject::LoadScriptData(
    const std::uint32_t address, const std::string& folder,
    const std::string& scene, const std::uint8_t incoming_header_byte,
    IMinimapManagerScriptRuntime& runtime) {
    runtime.Reset(address);
    const auto layers = runtime.LoadLayerRecords(
        folder, scene, incoming_header_byte);
    auto& state = runtime.Access(address);
    auto begin = state.ReadDword(0x18u);
    const auto end = state.ReadDword(0x1cu);
    const auto capacity_end = state.ReadDword(0x20u);
    const auto count = static_cast<std::uint32_t>(layers.size());
    const auto size = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            std::bit_cast<std::int32_t>(end - begin) >> 2);
    const auto capacity = begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            std::bit_cast<std::int32_t>(capacity_end - begin) >> 2);

    if (count > size && count > capacity) {
        runtime.FreeList(begin);
        begin = runtime.AllocateList(count * 4u);
        state.WriteDword(0x18u, begin);
        state.WriteDword(0x20u, begin + count * 4u);
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        runtime.WriteListWord(begin + index * 4u, layers[index]);
    }
    state.WriteDword(0x1cu, begin + count * 4u);
    if (begin == 0u || count == 0u) {
        return false;
    }
    runtime.SelectTexture(address, 0u);
    return true;
}

const std::array<float, 3>* MinimapManagerObject::ConvertGridCoordinates(
    const std::uint32_t address, const std::array<float, 3>& world,
    std::array<float, 2>& grid_coordinates,
    IMinimapManagerUpdateRuntime& runtime) {
    const auto grid = runtime.Access(address).ReadDword(0x24u);
    if (grid == 0u) {
        return nullptr;
    }
    grid_coordinates = runtime.WorldToGrid(grid, world);
    return &world;
}

std::uint32_t MinimapManagerObject::CaptureGridData(
    const std::uint32_t address, const std::uint8_t collection_stack_byte,
    const std::uint8_t mask_stack_byte,
    IMinimapManagerSnapshotRuntime& runtime) {
    const auto snapshot = MinimapCollection::AllocateAndConstruct(
        collection_stack_byte, runtime);
    if (snapshot == 0u) {
        throw std::bad_alloc();
    }
    auto& manager = runtime.AccessManager(address);

    std::uint32_t current_index = 0xffffffffu;
    for (std::uint32_t index = 0u;; ++index) {
        const auto first = manager.ReadDword(0x18u);
        if (first == 0u) break;
        const auto count = static_cast<std::uint32_t>(
            std::bit_cast<std::int32_t>(
                manager.ReadDword(0x1cu) - first) >> 2);
        if (index >= count) break;
        if (runtime.ReadDword(first + index * 4u) ==
            manager.ReadDword(0x24u)) {
            current_index = index;
            break;
        }
    }
    runtime.WriteDword(snapshot + 20u, current_index);
    if (manager.ReadDword(0x50u) != 2u) {
        return snapshot;
    }

    for (std::uint32_t index = 0u;; ++index) {
        const auto first = manager.ReadDword(0x18u);
        if (first == 0u) break;
        const auto count = static_cast<std::uint32_t>(
            std::bit_cast<std::int32_t>(
                manager.ReadDword(0x1cu) - first) >> 2);
        if (index >= count) break;

        const auto mask = runtime.Allocate(16u);
        if (mask == 0u) {
            throw std::bad_alloc();
        }
        runtime.WriteByte(mask, mask_stack_byte);
        runtime.WriteDword(mask + 4u, 0u);
        runtime.WriteDword(mask + 8u, 0u);
        runtime.WriteDword(mask + 12u, 0u);
        runtime.EncodeGridMask(runtime.ReadDword(first + index * 4u), mask);

        const auto begin = runtime.ReadDword(snapshot + 8u);
        const auto end = runtime.ReadDword(snapshot + 12u);
        const auto capacity_end = runtime.ReadDword(snapshot + 16u);
        if (capacity_end - end >= 4u) {
            runtime.WriteDword(end, mask);
            runtime.WriteDword(snapshot + 12u, end + 4u);
            continue;
        }

        const auto old_size = begin == 0u ? 0u : (end - begin) / 4u;
        const auto new_capacity = old_size + (old_size > 1u ? old_size : 1u);
        const auto new_begin = runtime.Allocate(new_capacity * 4u);
        if (new_begin == 0u) {
            throw std::bad_alloc();
        }
        for (std::uint32_t position = 0u; position < old_size; ++position) {
            runtime.WriteDword(new_begin + position * 4u,
                               runtime.ReadDword(begin + position * 4u));
        }
        runtime.WriteDword(new_begin + old_size * 4u, mask);
        runtime.Free(begin);
        runtime.WriteDword(snapshot + 16u,
                           new_begin + new_capacity * 4u);
        runtime.WriteDword(snapshot + 8u, new_begin);
        runtime.WriteDword(snapshot + 12u,
                           new_begin + (old_size + 1u) * 4u);
    }
    return snapshot;
}

void MinimapManagerObject::RestoreGridData(
    const std::uint32_t address, const std::uint32_t snapshot,
    IMinimapManagerRestoreRuntime& runtime) {
    auto& manager = runtime.AccessManager(address);
    if (manager.ReadDword(0x50u) == 2u) {
        const auto mask_begin = runtime.ReadDword(snapshot + 8u);
        const auto mask_count = mask_begin == 0u ? 0u :
            static_cast<std::uint32_t>(
                std::bit_cast<std::int32_t>(
                    runtime.ReadDword(snapshot + 12u) - mask_begin) >> 2);
        const auto grid_begin = manager.ReadDword(0x18u);
        const auto grid_count = grid_begin == 0u ? 0u :
            static_cast<std::uint32_t>(
                std::bit_cast<std::int32_t>(
                    manager.ReadDword(0x1cu) - grid_begin) >> 2);
        if (mask_count != grid_count) {
            return;
        }
        for (std::uint32_t index = 0u;; ++index) {
            const auto first = manager.ReadDword(0x18u);
            if (first == 0u) break;
            const auto count = static_cast<std::uint32_t>(
                std::bit_cast<std::int32_t>(
                    manager.ReadDword(0x1cu) - first) >> 2);
            if (index >= count) break;
            const auto grid = runtime.ReadDword(first + index * 4u);
            const auto mask = runtime.ReadDword(mask_begin + index * 4u);
            runtime.ApplyGridMask(grid, mask);
        }
    }
    runtime.SelectTexture(address, runtime.ReadDword(snapshot + 20u));
}

}  // namespace pal4
