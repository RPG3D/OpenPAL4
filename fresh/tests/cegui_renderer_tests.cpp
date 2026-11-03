#include "pal4/cegui_renderer.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::ICeguiRendererRuntime {
public:
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> freed;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> copied60;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> range_calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> external_words;
    std::map<std::uint32_t, std::uint32_t> external_memory;
    std::vector<std::uint32_t> removed_nodes;
    std::vector<std::string> destroy_events;
    std::map<std::uint32_t, std::uint16_t> indices;
    std::array<std::uint8_t, 28> forwarded_template{};
    std::uint32_t insert28_position = 0xffffffffu;
    std::uint32_t insert28_count = 0u;
    std::uint32_t move28_destination = 0u;
    std::uint32_t move28_end = 0u;
    std::uint32_t insert_wide_position = 0xffffffffu;
    std::uint32_t insert_wide_count = 0u;
    std::uint16_t insert_wide_fill = 0xffffu;
    std::uint32_t copy_wide_destination = 0u;
    std::uint32_t copy_wide_end = 0u;
    std::uint32_t effect_count = 0u;
    int base_calls = 0;
    int rect_calls = 0;
    std::uint32_t rect_destination = 0u;
    std::uint32_t rect_source = 0u;

    void ConstructBase(pal4::CeguiRendererState& state) override {
        ++base_calls;
        state.WriteDword(0u, 0xdeadbeefu);
        state.WriteByte(180u, 0x77u);
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        return allocations.size() == 1u ? 0x5000u : 0x6000u;
    }
    void Free(const std::uint32_t address) override {
        destroy_events.push_back("free");
        freed.push_back(address);
    }
    void WriteDword(const std::uint32_t address,
                    const std::uint32_t value) override {
        external_words.emplace_back(address, value);
        external_memory[address] = value;
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint16_t value) override {
        indices[address] = value;
    }
    void CopyElement60(const std::uint32_t destination,
                       const std::uint32_t source) override {
        copied60.emplace_back(destination, source);
    }
    void RangeNoOp(const std::uint32_t begin,
                   const std::uint32_t end) override {
        range_calls.emplace_back(begin, end);
    }
    std::uint32_t EffectCount(
        const pal4::CeguiRendererState& /*state*/) override {
        return effect_count;
    }
    void InsertElements28(
        pal4::CeguiRendererState& state, const std::uint32_t position,
        const std::uint32_t count,
        const std::array<std::uint8_t, 28>& value) override {
        insert28_position = position;
        insert28_count = count;
        forwarded_template = value;
        state.WriteDword(204u, 0x800000u);
        state.WriteDword(208u, 0x800000u + 28u * (0x4000u + 3u));
        state.WriteDword(212u, state.ReadDword(208u));
    }
    std::uint32_t ElementCount28(
        const pal4::CeguiRendererState& state) override {
        return (state.ReadDword(208u) - state.ReadDword(204u)) / 28u;
    }
    void MoveElements28(pal4::CeguiRendererState& state,
                        const std::uint32_t destination,
                        const std::uint32_t source_end) override {
        move28_destination = destination;
        move28_end = source_end;
        state.WriteDword(208u, destination);
    }
    std::uint32_t WideSize(
        const pal4::CeguiRendererState& state) override {
        const auto begin = state.ReadDword(220u);
        return begin == 0u ? 0u :
               (state.ReadDword(224u) - begin) / 2u;
    }
    void InsertWide(pal4::CeguiRendererState& state,
                    const std::uint32_t position,
                    const std::uint32_t count,
                    const std::uint16_t fill) override {
        insert_wide_position = position;
        insert_wide_count = count;
        insert_wide_fill = fill;
        state.WriteDword(220u, 0x500000u);
        state.WriteDword(224u, 0x500000u + 2u * count);
        state.WriteDword(228u, 0x500000u + 2u * count);
    }
    void CopyWide(pal4::CeguiRendererState& state,
                  const std::uint32_t destination,
                  const std::uint32_t source_end) override {
        copy_wide_destination = destination;
        copy_wide_end = source_end;
        state.WriteDword(224u, destination);
    }
    void AssignRect(pal4::CeguiRendererState& state,
                    const std::uint32_t destination_offset,
                    const std::uint32_t source_offset) override {
        Check(state.ReadDword(0u) ==
                  pal4::CeguiRendererState::kWideVtable,
              "wide vtable is installed before Rect assignment");
        ++rect_calls;
        rect_destination = destination_offset;
        rect_source = source_offset;
        for (std::uint32_t index = 0u; index < 4u; ++index) {
            state.WriteDword(destination_offset + 4u * index,
                             state.ReadDword(source_offset + 4u * index));
        }
    }
    void ProcessCollectionUntilEmpty(
        pal4::CeguiRendererState& state) override {
        Check(state.ReadDword(0u) == pal4::CeguiRendererState::kVtable,
              "base vtable is restored before collection cleanup");
        destroy_events.push_back("process");
    }
    std::uint32_t ReadDword(const std::uint32_t address) override {
        return external_memory.at(address);
    }
    void RemoveListElement(pal4::CeguiRendererState& /*state*/,
                           const std::uint32_t node) override {
        destroy_events.push_back("remove");
        removed_nodes.push_back(node);
    }
    void DestroyBase(pal4::CeguiRendererState& /*state*/) override {
        destroy_events.push_back("base");
    }
};

void CheckIndexPattern(const Runtime& runtime,
                       const std::uint32_t begin) {
    Check(runtime.indices.size() == 0x6000u,
          "renderer fills exactly 24576 index words");
    for (std::uint32_t quad = 0u; quad < 0x1000u; ++quad) {
        const auto offset = begin + 12u * quad;
        const auto first = static_cast<std::uint16_t>(4u * quad);
        Check(runtime.indices.at(offset) == first &&
                  runtime.indices.at(offset + 2u) == first + 1u &&
                  runtime.indices.at(offset + 4u) == first + 2u &&
                  runtime.indices.at(offset + 6u) == first + 3u &&
                  runtime.indices.at(offset + 8u) == first &&
                  runtime.indices.at(offset + 10u) == first + 2u,
              "each quad receives the observed six-index pattern");
    }
}

void FreshConstructorInitializesAllObservedFields() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    std::array<std::uint8_t, 28> stack_element{};
    stack_element.fill(0xa5u);
    Check(&pal4::CeguiRenderer::Construct(
              state, 1280, 720, 0x5au, stack_element,
              runtime) == &state && runtime.base_calls == 1 &&
              state.ReadDword(0u) == pal4::CeguiRendererState::kVtable &&
              state.ReadByte(180u) == 0x77u,
          "constructor calls base then installs derived vtable");
    for (const auto offset : {184u, 200u, 216u, 232u}) {
        Check(state.ReadByte(offset) == 0x5au,
              "undefined stack byte is explicit at four headers");
    }
    Check(state.ReadDword(236u) == 0x5000u &&
              state.ReadDword(240u) == 0u &&
              state.ReadDword(244u) == 0u &&
              state.ReadDword(252u) == 0u &&
              state.ReadDword(264u) == 0u &&
              state.ReadByte(260u) == 1u &&
              state.ReadByte(268u) == 1u &&
              state.ReadFloat(256u) == 1280.0f &&
              state.ReadFloat(248u) == 720.0f,
          "renderer state fields and config dimensions match IDA offsets");
    Check(runtime.external_words ==
              std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x5000u, 0x5000u}, {0x5004u, 0x5000u}} &&
              runtime.allocations == std::vector<std::uint32_t>{
                  0x0cu, 0x3c000u} &&
              runtime.freed == std::vector<std::uint32_t>{0u} &&
              runtime.range_calls ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0u, 0u}},
          "sentinel and empty 60-byte collection use observed calls");
    Check(state.ReadDword(188u) == 0x6000u &&
              state.ReadDword(192u) == 0x6000u &&
              state.ReadDword(196u) == 0x6000u + 0x3c000u &&
              runtime.copied60.empty(),
          "first 60-byte allocation reserves 4096 empty elements");
    Check(runtime.insert28_position == 0u &&
              runtime.insert28_count == 0x4000u &&
              runtime.forwarded_template == stack_element &&
              runtime.insert_wide_position == 0u &&
              runtime.insert_wide_count == 0x6000u &&
              runtime.insert_wide_fill == 0u,
          "constructor requests 16384 collection items and 24576 wide indices");
    CheckIndexPattern(runtime, 0x500000u);
}

void ExistingSmallBuffersGrowAndCopy() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    runtime.effect_count = 2u;
    state.WriteDword(188u, 0x10000u);
    state.WriteDword(192u, 0x10000u + 120u);
    state.WriteDword(196u, 0x10000u + 180u);
    state.WriteDword(204u, 0x90000u);
    state.WriteDword(208u, 0x90000u + 84u);
    state.WriteDword(212u, 0x90000u + 84u);
    state.WriteDword(220u, 0x300000u);
    state.WriteDword(224u, 0x300000u + 0xc000u);
    state.WriteDword(228u, 0x300000u + 0xc000u);
    const std::array<std::uint8_t, 28> element{};
    pal4::CeguiRenderer::InitializeBuffers(state, element, runtime);
    Check(runtime.allocations == std::vector<std::uint32_t>{0x3c000u} &&
              runtime.copied60 ==
                  std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x5000u, 0x10000u}, {0x503cu, 0x1003cu}} &&
              runtime.freed == std::vector<std::uint32_t>{0x10000u} &&
              state.ReadDword(188u) == 0x5000u &&
              state.ReadDword(192u) == 0x5000u + 120u,
          "small 60-byte buffer copies two records and preserves count");
    Check(runtime.insert28_position == 0x90000u + 84u &&
              runtime.insert28_count == 0x4000u - 3u &&
              runtime.insert_wide_position == 0xffffffffu &&
              runtime.copy_wide_destination == 0u,
          "small 28-byte collection grows while exact-size wide buffer stays");
    CheckIndexPattern(runtime, 0x300000u);
}

void OversizedBuffersAreCompacted() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(188u, 0x100000u);
    state.WriteDword(192u, 0x100000u);
    state.WriteDword(196u, 0x100000u + 0x3c000u);
    state.WriteDword(204u, 0x200000u);
    state.WriteDword(208u, 0x200000u + 28u * 17000u);
    state.WriteDword(212u, state.ReadDword(208u));
    state.WriteDword(220u, 0x300000u);
    state.WriteDword(224u, 0x300000u + 2u * 24580u);
    state.WriteDword(228u, state.ReadDword(224u));
    const auto old_end28 = state.ReadDword(208u);
    const auto old_end_wide = state.ReadDword(224u);
    const std::array<std::uint8_t, 28> element{};
    pal4::CeguiRenderer::InitializeBuffers(state, element, runtime);
    Check(runtime.allocations.empty() &&
              runtime.move28_destination == 0x200000u + 0x70000u &&
              runtime.move28_end == old_end28 &&
              runtime.insert28_position == 0xffffffffu,
          "oversized 28-byte collection uses move helper without allocation");
    Check(runtime.copy_wide_destination == 0x300000u + 0xc000u &&
              runtime.copy_wide_end == old_end_wide &&
              runtime.insert_wide_position == 0xffffffffu,
          "oversized wide buffer uses copy helper");
    CheckIndexPattern(runtime, 0x300000u);
}

void StubProvidesLinkableRendererBoundary() {
    pal4::CeguiRendererState state;
    pal4::CeguiRendererStub runtime;
    const std::array<std::uint8_t, 28> element{};
    Check(&pal4::CeguiRenderer::Construct(
              state, 800, 600, 0u, element, runtime) == &state &&
              state.ReadDword(236u) != 0u &&
              state.ReadDword(220u) != 0u &&
              state.ReadFloat(256u) == 800.0f,
          "renderer constructor links and returns with no CEGUI or RenderWare DLL");
    pal4::CeguiRenderer::Destroy(state, runtime);
    Check(state.ReadDword(236u) == 0u && state.ReadDword(220u) == 0u,
          "linkable stub supports renderer destruction");
}

void ScaledAndDerivedConstructorsRetainTheirLayers() {
    const std::array<std::uint8_t, 28> element{};
    constexpr auto width_factor = std::bit_cast<float>(0x3aa3d70au);
    constexpr auto height_factor = std::bit_cast<float>(0x3ada740eu);
    const auto expected_width = static_cast<float>(
        1280.0 * static_cast<double>(width_factor));
    const auto expected_height = static_cast<float>(
        720.0 * static_cast<double>(height_factor));

    pal4::CeguiRendererState scaled;
    Runtime scaled_runtime;
    Check(&pal4::CeguiRenderer::ConstructScaled(
              scaled, 1280, 720, 0x11u, element,
              scaled_runtime) == &scaled &&
              scaled.ReadDword(0u) ==
                  pal4::CeguiRendererState::kScaledVtable &&
              scaled.ReadFloat(256u) == 800.0f &&
              scaled.ReadFloat(248u) == 600.0f &&
              scaled.ReadFloat(272u) == expected_width &&
              scaled.ReadFloat(276u) == expected_height,
          "scaled constructor applies logical dimensions and IDA scale factors");

    pal4::CeguiRendererState variant3;
    Runtime runtime3;
    Check(&pal4::CeguiRenderer::ConstructVariant3(
              variant3, 1280, 720, 0u, element,
              runtime3) == &variant3 &&
              variant3.ReadDword(0u) ==
                  pal4::CeguiRendererState::kVariant3Vtable &&
              variant3.ReadFloat(272u) == expected_width &&
              variant3.ReadFloat(276u) == expected_height &&
              variant3.ReadFloat(280u) == expected_width &&
              variant3.ReadFloat(284u) == expected_height,
          "third constructor extends scaled renderer with another scale pair");

    pal4::CeguiRendererState variant4;
    Runtime runtime4;
    Check(&pal4::CeguiRenderer::ConstructVariant4(
              variant4, 1280, 720, 0u, element,
              runtime4) == &variant4 &&
              variant4.ReadDword(0u) ==
                  pal4::CeguiRendererState::kVariant4Vtable &&
              variant4.ReadFloat(280u) == expected_width &&
              variant4.ReadFloat(284u) == expected_height,
          "fourth constructor retains data writes with its distinct vtable");
}

void WideConstructorCopiesRectThenAdjustsHorizontalBounds() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    const std::array<std::uint8_t, 28> element{};
    Check(&pal4::CeguiRenderer::ConstructWide(
              state, 1280, 800, 0u, element,
              runtime) == &state &&
              state.ReadDword(0u) ==
                  pal4::CeguiRendererState::kWideVtable &&
              state.ReadDword(272u) == 0x3faaaaabu &&
              state.ReadDword(276u) == 0x3faaaaabu &&
              state.ReadDword(280u) == 0x42d55558u &&
              runtime.rect_calls == 1 &&
              runtime.rect_destination == 284u &&
              runtime.rect_source == 244u,
          "wide constructor writes constants and invokes Rect assignment once");
    const auto shift = static_cast<double>(state.ReadFloat(280u)) /
                       static_cast<double>(state.ReadFloat(272u));
    Check(state.ReadFloat(284u) == 0.0f &&
              state.ReadFloat(288u) == 600.0f &&
              state.ReadFloat(292u) == static_cast<float>(-shift) &&
              state.ReadFloat(296u) ==
                  static_cast<float>(shift + 800.0),
          "wide constructor expands copied rectangle symmetrically");
}

void DestructorProcessesListAndReleasesBuffersInOrder() {
    pal4::CeguiRendererState state;
    Runtime runtime;
    state.WriteDword(0u, pal4::CeguiRendererState::kWideVtable);
    state.WriteDword(236u, 0x5000u);
    state.WriteDword(240u, 2u);
    state.WriteDword(220u, 0x6000u);
    state.WriteDword(224u, 0x6010u);
    state.WriteDword(228u, 0x6020u);
    state.WriteDword(204u, 0x7000u);
    state.WriteDword(208u, 0x7010u);
    state.WriteDword(212u, 0x7020u);
    state.WriteDword(188u, 0x8000u);
    state.WriteDword(192u, 0x8010u);
    state.WriteDword(196u, 0x8020u);
    state.WriteByte(260u, 1u);
    runtime.external_memory[0x5000u] = 0x5100u;
    runtime.external_memory[0x5100u] = 0x5200u;
    runtime.external_memory[0x5200u] = 0x5000u;
    pal4::CeguiRenderer::Destroy(state, runtime);
    Check(runtime.removed_nodes == std::vector<std::uint32_t>{
              0x5100u, 0x5200u} &&
              runtime.freed == std::vector<std::uint32_t>{
                  0x5000u, 0x6000u, 0x7000u, 0x8000u} &&
              runtime.destroy_events == std::vector<std::string>{
                  "process", "remove", "remove", "free", "free",
                  "free", "free", "base"},
          "destructor drains list then frees four stores in IDA order");
    for (const auto offset : {188u, 192u, 196u, 204u, 208u, 212u,
                              220u, 224u, 228u, 236u, 240u}) {
        Check(state.ReadDword(offset) == 0u,
              "destructor clears each released pointer and count");
    }
    Check(state.ReadDword(0u) == pal4::CeguiRendererState::kVtable &&
              state.ReadByte(260u) == 1u,
          "destructor restores base vtable and preserves unrelated flag");
}

}  // namespace

int main() {
    FreshConstructorInitializesAllObservedFields();
    ExistingSmallBuffersGrowAndCopy();
    OversizedBuffersAreCompacted();
    StubProvidesLinkableRendererBoundary();
    ScaledAndDerivedConstructorsRetainTheirLayers();
    WideConstructorCopiesRectThenAdjustsHorizontalBounds();
    DestructorProcessesListAndReleasesBuffersInOrder();
    std::cout << "7 CEGUI renderer test cases passed\n";
}
