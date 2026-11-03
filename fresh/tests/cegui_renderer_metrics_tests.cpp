#include "pal4/cegui_renderer_metrics.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void QueueFlagStoresOnlyObservedByte() {
    pal4::CeguiRendererState state;
    state.WriteByte(259u, 0x55u);
    state.WriteByte(261u, 0xaau);
    Check(pal4::CeguiRendererMetrics::SetQueueEnabled(state, 0x7fu) ==
              0x7fu &&
              pal4::CeguiRendererMetrics::QueueEnabled(state) == 0x7fu &&
              state.ReadByte(259u) == 0x55u &&
              state.ReadByte(261u) == 0xaau,
          "setter returns byte and preserves adjacent flags");
}

void WidthHeightAndSizeUseFourObservedFloats() {
    pal4::CeguiRendererState state;
    state.WriteFloat(244u, 2.0f);
    state.WriteFloat(248u, 22.0f);
    state.WriteFloat(252u, 3.0f);
    state.WriteFloat(256u, 33.0f);
    std::array<float, 2> size{};
    Check(pal4::CeguiRendererMetrics::MainRectWidth(state) == 30.0 &&
              pal4::CeguiRendererMetrics::MainRectHeight(state) == 20.0 &&
              &pal4::CeguiRendererMetrics::MainRectSize(state, size) == &size &&
              size == std::array<float, 2>{30.0f, 20.0f},
          "width, height and output pair use binary offsets and order");
}

void BothRectCopiesPreserveRawWords() {
    pal4::CeguiRendererState state;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.WriteDword(244u + 4u * index, 0x80000000u + index);
        state.WriteDword(284u + 4u * index, 0x7fc00000u + index);
    }
    std::array<std::uint32_t, 4> first{};
    std::array<std::uint32_t, 4> second{};
    Check(&pal4::CeguiRendererMetrics::CopyMainRect(state, first) == &first &&
              &pal4::CeguiRendererMetrics::CopyWideRect(state, second) ==
                  &second &&
              first == std::array<std::uint32_t, 4>{
                  0x80000000u, 0x80000001u, 0x80000002u, 0x80000003u} &&
              second == std::array<std::uint32_t, 4>{
                  0x7fc00000u, 0x7fc00001u, 0x7fc00002u, 0x7fc00003u},
          "rect helpers copy all four raw DWORDs and return output identity");
    Check(pal4::CeguiRendererMetrics::Constant96() == 96u,
          "constant helper returns observed value");
}

}  // namespace

int main() {
    QueueFlagStoresOnlyObservedByte();
    WidthHeightAndSizeUseFourObservedFloats();
    BothRectCopiesPreserveRawWords();
    std::cout << "3 CEGUI renderer metrics test cases passed\n";
}
