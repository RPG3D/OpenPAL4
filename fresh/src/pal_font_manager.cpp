#include "pal4/pal_font_manager.h"

#include "pal4/standard_stream_lifecycle.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t PalFontManagerAccess::Get(
    PalFontManagerAccessState& state,
    IPalFontManagerAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palFontManagerA", 1004u,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t PalFontManagerState::ReadDword(
    const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void PalFontManagerState::WriteDword(
    const std::uint32_t offset, const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

std::uint32_t PalFontManagerObjectRuntimeStub::Allocate(
    std::uint32_t /*bytes*/) {
    const auto address = next_address_;
    next_address_ += PalFontManagerState::kBytes;
    objects_.emplace(address, PalFontManagerState{});
    return address;
}

std::uint32_t PalFontManagerObjectRuntimeStub::Free(
    const std::uint32_t address) {
    objects_.erase(address);
    return 0u;
}

PalFontManagerState& PalFontManagerObjectRuntimeStub::Access(
    const std::uint32_t address) {
    return objects_.at(address);
}

void PalFontManagerObjectRuntimeStub::CallEmptyDestructor(
    std::uint32_t /*address*/) {}

std::uint32_t PalFontManagerObject::Create(
    const std::uint8_t incoming_stack_byte,
    IPalFontManagerObjectRuntime& runtime) {
    const auto address = runtime.Allocate(PalFontManagerState::kBytes);
    if (address == 0u) {
        return 0u;
    }
    try {
        return Construct(address, incoming_stack_byte, runtime);
    } catch (...) {
        static_cast<void>(runtime.Free(address));
        throw;
    }
}

void PalFontManagerObject::Destroy(
    const std::uint32_t address,
    IPalFontManagerObjectRuntime& runtime) {
    if (address != 0u) {
        static_cast<void>(Tidy(address, runtime));
        static_cast<void>(runtime.Free(address));
    }
}

std::uint32_t PalFontManagerObject::Construct(
    const std::uint32_t address,
    const std::uint8_t incoming_stack_byte,
    IPalFontManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    state.Bytes().at(0u) = incoming_stack_byte;
    state.WriteDword(4u, 0u);
    state.WriteDword(8u, 0u);
    state.WriteDword(12u, 0u);
    static_cast<void>(StandardStreamLifecycle::ConstructIostreamInit(
        address + 16u));
    static_cast<void>(StandardStreamLifecycle::ConstructIostreamInit(
        address + 20u));
    static_cast<void>(StandardStreamLifecycle::ConstructIostreamInit(
        address + 24u));
    return address;
}

std::uint32_t PalFontManagerObject::Tidy(
    const std::uint32_t address,
    IPalFontManagerObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    runtime.CallEmptyDestructor(address + 24u);
    runtime.CallEmptyDestructor(address + 20u);
    runtime.CallEmptyDestructor(address + 16u);
    const auto result = runtime.Free(state.ReadDword(4u));
    state.WriteDword(4u, 0u);
    state.WriteDword(8u, 0u);
    state.WriteDword(12u, 0u);
    return result;
}

std::uint32_t PalFontManagerFontRuntimeStub::Log(
    std::uint32_t /*source_line*/) {
    return 0u;
}

void PalFontManagerFontRuntimeStub::CreateFont(
    std::uint32_t /*font_slot*/,
    const PalFontDescription& /*description*/) {}

std::uint32_t PalFontManagerFontRuntimeStub::AllocateArray(
    const std::uint32_t bytes) {
    const auto address = next_address_;
    next_address_ += bytes + 0x10u;
    return address;
}

std::uint32_t PalFontManagerFontRuntimeStub::ReadArray(
    const std::uint32_t address) {
    return words_.at(address);
}

void PalFontManagerFontRuntimeStub::WriteArray(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

void PalFontManagerFontRuntimeStub::FreeArray(
    std::uint32_t /*address*/) {}

void PalFontManagerFontRuntimeStub::InsertOne(
    PalFontManagerState& state, const std::uint32_t font_slot) {
    const auto begin = state.ReadDword(4u);
    const auto end = state.ReadDword(8u);
    const auto capacity = state.ReadDword(12u);
    if (capacity - end >= 4u && begin != 0u) {
        WriteArray(end, font_slot);
        state.WriteDword(8u, end + 4u);
        return;
    }
    const auto count = begin == 0u ? 0u : (end - begin) / 4u;
    const auto new_count = count + (count > 1u ? count : 1u);
    const auto storage = AllocateArray(new_count * 4u);
    for (std::uint32_t index = 0u; index < count; ++index) {
        WriteArray(storage + index * 4u,
                   ReadArray(begin + index * 4u));
    }
    WriteArray(storage + count * 4u, font_slot);
    FreeArray(begin);
    state.WriteDword(4u, storage);
    state.WriteDword(8u, storage + (count + 1u) * 4u);
    state.WriteDword(12u, storage + new_count * 4u);
}

std::uint32_t PalFontManagerFonts::Initialize(
    const std::uint32_t object_address, PalFontManagerState& state,
    IPalFontManagerFontRuntime& runtime) {
    static_cast<void>(runtime.Log(0x15u));
    const auto first = object_address + 16u;
    const auto second = object_address + 20u;
    const auto third = object_address + 24u;
    const PalFontDescription normal{
        0, 0, 400, 0, 0x8a6068u, 0, {1u, 0u, 0u, 0u}};
    auto bold = normal;
    bold.weight = 800;
    auto variant = normal;
    variant.fourth = 1;
    runtime.CreateFont(first, normal);
    runtime.InsertOne(state, first);
    runtime.CreateFont(second, bold);
    runtime.InsertOne(state, second);
    runtime.CreateFont(third, variant);

    const auto begin = state.ReadDword(4u);
    const auto end = state.ReadDword(8u);
    const auto capacity = state.ReadDword(12u);
    if (capacity - end >= 4u) {
        runtime.WriteArray(end, third);
        state.WriteDword(8u, end + 4u);
    } else {
        const auto count = begin == 0u ? 0u : (end - begin) / 4u;
        const auto new_count = count + (count > 1u ? count : 1u);
        const auto storage = runtime.AllocateArray(new_count * 4u);
        if (storage == 0u) {
            throw std::bad_alloc();
        }
        for (std::uint32_t index = 0u; index < count; ++index) {
            runtime.WriteArray(storage + index * 4u,
                               runtime.ReadArray(begin + index * 4u));
        }
        runtime.WriteArray(storage + count * 4u, third);
        runtime.FreeArray(begin);
        state.WriteDword(4u, storage);
        state.WriteDword(8u, storage + (count + 1u) * 4u);
        state.WriteDword(12u, storage + new_count * 4u);
    }
    return runtime.Log(0x22u);
}

std::uint32_t PalFontManagerFonts::Release(
    PalFontManagerState& state, IRenderStateObjectRuntime& runtime) {
    for (const auto offset : {16u, 20u, 24u}) {
        auto object = state.ReadDword(offset);
        static_cast<void>(RenderStateObject::Release(object, runtime));
        state.WriteDword(offset, object);
    }
    const auto old_end = state.ReadDword(8u);
    state.WriteDword(8u, state.ReadDword(4u));
    return old_end;
}

std::uint32_t PalFontManagerFonts::GetTextures(
    const PalFontManagerState& state,
    IPalFontManagerFontRuntime& array_runtime,
    IRenderStateObjectRuntime& render_runtime) {
    for (auto cursor = state.ReadDword(4u);
         cursor != state.ReadDword(8u); cursor += 4u) {
        static_cast<void>(RenderStateObject::GetTextureSlot40(
            array_runtime.ReadArray(cursor), render_runtime));
    }
    return state.ReadDword(8u);
}

std::uint32_t PalFontManagerFonts::GetTextureStates(
    const PalFontManagerState& state,
    IPalFontManagerFontRuntime& array_runtime,
    IRenderStateObjectRuntime& render_runtime) {
    for (auto cursor = state.ReadDword(4u);
         cursor != state.ReadDword(8u); cursor += 4u) {
        static_cast<void>(RenderStateObject::GetTextureSlot44(
            array_runtime.ReadArray(cursor), render_runtime));
    }
    return state.ReadDword(8u);
}

std::uint32_t PalFontManagerFonts::DrawRectangle(
    const PalFontManagerState& state, const std::int32_t command,
    const std::array<std::int32_t, 4>& rectangle,
    const std::uint32_t font_index, const std::int32_t penultimate,
    const std::int32_t last, IPalFontManagerFontRuntime& array_runtime,
    IRenderStateObjectRuntime& render_runtime) {
    const auto font = array_runtime.ReadArray(
        state.ReadDword(4u) + font_index * 4u);
    return RenderStateObject::DrawRectangle(
        font, command, rectangle, penultimate, last, render_runtime);
}

}  // namespace pal4
