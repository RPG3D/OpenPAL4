#include "pal4/cegui_text_converter.h"

namespace pal4 {
namespace {

std::uint32_t WideCapacity(const StaticCharacterBufferState& state) {
    if (state.wide_begin == 0u) return 0u;
    const auto delta = static_cast<std::int32_t>(
        state.wide_end - state.wide_begin);
    return static_cast<std::uint32_t>(delta >> 1u);
}

std::uint32_t ByteCapacity(const StaticCharacterBufferState& state) {
    if (state.multibyte_begin == 0u) return 0u;
    return state.multibyte_end - state.multibyte_begin;
}

}  // namespace

std::int32_t CeguiTextConversionStub::MultiByteToWideChar(
    std::uint32_t /*code_page*/, std::uint32_t /*flags*/,
    std::string_view /*source*/, std::int32_t /*source_length*/,
    std::uint32_t /*destination*/, std::int32_t /*capacity*/) {
    return 0;
}
std::int32_t CeguiTextConversionStub::WideCharToMultiByte(
    std::uint32_t /*code_page*/, std::uint32_t /*flags*/,
    std::uint32_t /*source*/, std::int32_t /*source_length*/,
    std::uint32_t /*destination*/, std::int32_t /*capacity*/) {
    return 0;
}
void CeguiTextConversionStub::InsertWide(
    StaticCharacterBufferState& /*state*/, std::uint32_t /*at*/,
    std::uint32_t /*count*/, std::uint16_t /*fill*/) {}
void CeguiTextConversionStub::CopyWide(
    StaticCharacterBufferState& /*state*/,
    std::uint32_t /*destination*/, std::uint32_t /*source_end*/) {}
void CeguiTextConversionStub::InsertByte(
    StaticCharacterBufferState& /*state*/, std::uint32_t /*at*/,
    std::uint32_t /*count*/, std::uint8_t /*fill*/) {}
void CeguiTextConversionStub::WriteWide(
    std::uint32_t /*address*/, std::uint16_t /*value*/) {}
void CeguiTextConversionStub::WriteByte(
    std::uint32_t /*address*/, std::uint8_t /*value*/) {}

std::uint32_t CeguiTextConverter::ConvertToUtf8(
    const std::string_view source, StaticCharacterBufferState& state,
    ICeguiTextConversionRuntime& runtime) {
    const auto length = static_cast<std::int32_t>(
        source.find('\0') == std::string_view::npos
            ? source.size()
            : source.find('\0'));
    const auto wide_count = runtime.MultiByteToWideChar(
        0u, 0u, source, length, 0u, 0);
    auto wide_capacity = WideCapacity(state);
    const auto doubled_wide =
        static_cast<std::uint32_t>(wide_count) * 2u;
    if (wide_capacity < static_cast<std::uint32_t>(wide_count) + 1u) {
        if (wide_capacity < doubled_wide) {
            runtime.InsertWide(state, state.wide_end,
                               doubled_wide - wide_capacity, 0u);
        } else if (state.wide_begin != 0u &&
                   doubled_wide < wide_capacity) {
            runtime.CopyWide(state,
                             state.wide_begin + doubled_wide * 2u,
                             state.wide_end);
        }
    }
    const auto wide = state.wide_begin;
    static_cast<void>(runtime.MultiByteToWideChar(
        0u, 0u, source, length, wide, wide_count));
    runtime.WriteWide(wide + static_cast<std::uint32_t>(wide_count) * 2u,
                      0u);

    const auto byte_count = runtime.WideCharToMultiByte(
        0xfde9u, 0u, wide, wide_count, 0u, 0);
    auto byte_capacity = ByteCapacity(state);
    const auto doubled_bytes =
        static_cast<std::uint32_t>(byte_count) * 2u;
    if (byte_capacity < static_cast<std::uint32_t>(byte_count) + 1u) {
        if (byte_capacity < doubled_bytes) {
            runtime.InsertByte(state, state.multibyte_end,
                               doubled_bytes - byte_capacity, 0u);
        } else if (state.multibyte_begin != 0u &&
                   doubled_bytes < byte_capacity) {
            state.multibyte_end =
                state.multibyte_begin + doubled_bytes;
        }
    }
    const auto bytes = state.multibyte_begin;
    static_cast<void>(runtime.WideCharToMultiByte(
        0xfde9u, 0u, wide, wide_count, bytes, byte_count));
    runtime.WriteByte(bytes + static_cast<std::uint32_t>(byte_count), 0u);
    return bytes;
}

}  // namespace pal4
