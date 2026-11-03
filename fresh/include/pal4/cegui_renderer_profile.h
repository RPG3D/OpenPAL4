#pragma once
#include <cstdint>
namespace pal4 {
enum class CeguiRendererVariant : std::uint8_t {
    Base800,
    Renderer2,
    Renderer3,
    Renderer4,
    Renderer5,
};

struct CeguiRendererPlan {
    CeguiRendererVariant variant;
    std::uint32_t bytes;
    bool align_uv;
};
// IDA4BC360 renderer construction branch; shared by initialization/adapters.
[[nodiscard]] constexpr CeguiRendererPlan SelectCeguiRendererPlan(std::uint32_t width, std::uint32_t height) {
    if (width == 1280u && height == 800u) return {CeguiRendererVariant::Renderer5, 0x12cu, true};
    if (width == 1280u && height == 960u) return {CeguiRendererVariant::Renderer4, 0x120u, true};
    if (width == 1152u) return {CeguiRendererVariant::Renderer3, 0x120u, true};
    if (width == 800u) return {CeguiRendererVariant::Base800, 0x110u, false};
    return {CeguiRendererVariant::Renderer2, 0x118u, true};
}
}
