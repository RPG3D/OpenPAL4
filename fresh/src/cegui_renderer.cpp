#include "pal4/cegui_renderer.h"
#include "pal4/cegui_vertex_collection.h"

#include <bit>

namespace pal4 {

void CeguiRendererStub::ConstructBase(CeguiRendererState& /*state*/) {}

std::uint32_t CeguiRendererStub::Allocate(const std::uint32_t bytes) {
    const auto address = next_address_;
    next_address_ += bytes + 0x10u;
    return address;
}

void CeguiRendererStub::Free(std::uint32_t /*address*/) {}
void CeguiRendererStub::WriteDword(const std::uint32_t address,
                                   const std::uint32_t value) {
    words_[address] = value;
}
void CeguiRendererStub::WriteWord(std::uint32_t /*address*/,
                                  std::uint16_t /*value*/) {}
void CeguiRendererStub::CopyElement60(std::uint32_t /*destination*/,
                                      std::uint32_t /*source*/) {}
void CeguiRendererStub::RangeNoOp(std::uint32_t /*begin*/,
                                  std::uint32_t /*end*/) {}

std::uint32_t CeguiRendererStub::EffectCount(
    const CeguiRendererState& state) {
    const auto begin = state.ReadDword(188u);
    return begin == 0u ? 0u : (state.ReadDword(192u) - begin) / 60u;
}

namespace {

class StubVertexCollectionAdapter final : public ICeguiVertexCollectionRuntime {
public:
    StubVertexCollectionAdapter(
        CeguiRendererStub& owner,
        std::map<std::uint32_t, CeguiVertexRecord>& records)
        : owner_(owner), records_(records) {}

    std::uint32_t Allocate(const std::uint32_t bytes) override {
        return owner_.Allocate(bytes);
    }
    void Free(const std::uint32_t address) override { owner_.Free(address); }
    void CopyRecord(const std::uint32_t destination,
                    const std::uint32_t source) override {
        records_[destination] = records_.at(source);
    }
    void WriteRecord(const std::uint32_t destination,
                     const CeguiVertexRecord& value) override {
        records_[destination] = value;
    }

private:
    CeguiRendererStub& owner_;
    std::map<std::uint32_t, CeguiVertexRecord>& records_;
};

}  // namespace

void CeguiRendererStub::InsertElements28(
    CeguiRendererState& state, const std::uint32_t position,
    const std::uint32_t count,
    const std::array<std::uint8_t, 28>& value) {
    StubVertexCollectionAdapter runtime(*this, vertex_records_);
    static_cast<void>(CeguiVertexCollection::Insert(
        state, position, count, value, runtime));
}

std::uint32_t CeguiRendererStub::ElementCount28(
    const CeguiRendererState& state) {
    const auto begin = state.ReadDword(204u);
    return begin == 0u ? 0u : (state.ReadDword(208u) - begin) / 28u;
}

void CeguiRendererStub::MoveElements28(
    CeguiRendererState& state, const std::uint32_t destination,
    const std::uint32_t source) {
    StubVertexCollectionAdapter runtime(*this, vertex_records_);
    static_cast<void>(CeguiVertexCollection::MoveRange(
        state, destination, source, runtime));
}

std::uint32_t CeguiRendererStub::WideSize(
    const CeguiRendererState& state) {
    const auto begin = state.ReadDword(220u);
    return begin == 0u ? 0u : (state.ReadDword(224u) - begin) / 2u;
}

void CeguiRendererStub::InsertWide(
    CeguiRendererState& state, std::uint32_t /*position*/,
    const std::uint32_t count, std::uint16_t /*fill*/) {
    const auto begin = Allocate(2u * count);
    state.WriteDword(220u, begin);
    state.WriteDword(224u, begin + 2u * count);
    state.WriteDword(228u, begin + 2u * count);
}

void CeguiRendererStub::CopyWide(
    CeguiRendererState& state, const std::uint32_t destination,
    std::uint32_t /*source_end*/) {
    state.WriteDword(224u, destination);
}

void CeguiRendererStub::AssignRect(
    CeguiRendererState& state, const std::uint32_t destination_offset,
    const std::uint32_t source_offset) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.WriteDword(destination_offset + 4u * index,
                         state.ReadDword(source_offset + 4u * index));
    }
}

void CeguiRendererStub::ProcessCollectionUntilEmpty(
    CeguiRendererState& /*state*/) {}

std::uint32_t CeguiRendererStub::ReadDword(const std::uint32_t address) {
    return words_.at(address);
}

void CeguiRendererStub::RemoveListElement(
    CeguiRendererState& /*state*/, std::uint32_t /*node*/) {}

void CeguiRendererStub::DestroyBase(CeguiRendererState& /*state*/) {}

CeguiRendererState& CeguiRenderer::Construct(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height,
    const std::uint8_t incoming_stack_byte,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    runtime.ConstructBase(state);
    for (const auto offset : {184u, 200u, 216u, 232u}) {
        state.WriteByte(offset, incoming_stack_byte);
    }
    for (const auto offset : {188u, 192u, 196u, 204u, 208u, 212u,
                              220u, 224u, 228u}) {
        state.WriteDword(offset, 0u);
    }
    const auto sentinel = runtime.Allocate(0x0cu);
    runtime.WriteDword(sentinel, sentinel);
    runtime.WriteDword(sentinel + 4u, sentinel);
    state.WriteDword(236u, sentinel);
    state.WriteDword(240u, 0u);
    state.WriteDword(264u, 0u);
    state.WriteByte(260u, 1u);
    state.WriteByte(268u, 1u);
    state.WriteDword(0u, CeguiRendererState::kVtable);
    state.WriteDword(252u, 0u);
    state.WriteDword(244u, 0u);
    state.WriteFloat(256u, static_cast<float>(config_width));
    state.WriteFloat(248u, static_cast<float>(config_height));
    InitializeBuffers(state, stack_element, runtime);
    return state;
}

void CeguiRenderer::InitializeBuffers(
    CeguiRendererState& state,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    auto begin60 = state.ReadDword(188u);
    auto end60 = state.ReadDword(192u);
    const auto capacity60 = state.ReadDword(196u);
    if (begin60 == 0u || (capacity60 - begin60) / 60u < 0x1000u) {
        const auto new_begin = runtime.Allocate(0x3c000u);
        auto target = new_begin;
        for (auto source = begin60; source != end60;
             source += 60u, target += 60u) {
            runtime.CopyElement60(target, source);
        }
        runtime.RangeNoOp(begin60, end60);
        runtime.Free(begin60);
        state.WriteDword(196u, new_begin + 0x3c000u);
        const auto count = runtime.EffectCount(state);
        state.WriteDword(188u, new_begin);
        state.WriteDword(192u, new_begin + 60u * count);
    }

    const auto begin28 = state.ReadDword(204u);
    const auto end28 = state.ReadDword(208u);
    if (begin28 == 0u || (end28 - begin28) / 28u < 0x4000u) {
        const auto count = begin28 == 0u ? 0u : (end28 - begin28) / 28u;
        runtime.InsertElements28(state, end28, 0x4000u - count,
                                 stack_element);
    } else if (runtime.ElementCount28(state) > 0x4000u) {
        runtime.MoveElements28(state, begin28 + 0x70000u, end28);
    }

    const auto begin_wide = state.ReadDword(220u);
    const auto end_wide = state.ReadDword(224u);
    if (begin_wide != 0u && (end_wide - begin_wide) / 2u >= 0x6000u) {
        if (runtime.WideSize(state) > 0x6000u) {
            runtime.CopyWide(state, begin_wide + 0xc000u, end_wide);
        }
    } else {
        const auto count = 0x6000u - runtime.WideSize(state);
        runtime.InsertWide(state, end_wide, count, 0u);
    }

    const auto indices = state.ReadDword(220u);
    for (std::uint32_t quad = 0u; quad < 0x1000u; ++quad) {
        const auto first_vertex = static_cast<std::uint16_t>(4u * quad);
        const auto offset = indices + 12u * quad;
        runtime.WriteWord(offset, first_vertex);
        runtime.WriteWord(offset + 2u,
                          static_cast<std::uint16_t>(first_vertex + 1u));
        runtime.WriteWord(offset + 4u,
                          static_cast<std::uint16_t>(first_vertex + 2u));
        runtime.WriteWord(offset + 6u,
                          static_cast<std::uint16_t>(first_vertex + 3u));
        runtime.WriteWord(offset + 8u, first_vertex);
        runtime.WriteWord(offset + 10u,
                          static_cast<std::uint16_t>(first_vertex + 2u));
    }
}

namespace {

float ScaledWidth(const std::int32_t width) {
    constexpr auto factor = std::bit_cast<float>(0x3aa3d70au);
    return static_cast<float>(static_cast<double>(width) *
                              static_cast<double>(factor));
}

float ScaledHeight(const std::int32_t height) {
    constexpr auto factor = std::bit_cast<float>(0x3ada740eu);
    return static_cast<float>(static_cast<double>(height) *
                              static_cast<double>(factor));
}

}  // namespace

CeguiRendererState& CeguiRenderer::ConstructScaled(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height,
    const std::uint8_t incoming_stack_byte,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    static_cast<void>(Construct(state, config_width, config_height,
                                incoming_stack_byte, stack_element, runtime));
    state.WriteDword(0u, CeguiRendererState::kScaledVtable);
    state.WriteDword(256u, 0x44480000u);
    state.WriteDword(248u, 0x44160000u);
    state.WriteFloat(272u, ScaledWidth(config_width));
    state.WriteFloat(276u, ScaledHeight(config_height));
    return state;
}

CeguiRendererState& CeguiRenderer::ConstructVariant3(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height,
    const std::uint8_t incoming_stack_byte,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    static_cast<void>(ConstructScaled(state, config_width, config_height,
                                      incoming_stack_byte, stack_element,
                                      runtime));
    state.WriteDword(0u, CeguiRendererState::kVariant3Vtable);
    state.WriteDword(256u, 0x44480000u);
    state.WriteDword(248u, 0x44160000u);
    state.WriteFloat(280u, ScaledWidth(config_width));
    state.WriteFloat(284u, ScaledHeight(config_height));
    return state;
}

CeguiRendererState& CeguiRenderer::ConstructVariant4(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height,
    const std::uint8_t incoming_stack_byte,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    static_cast<void>(ConstructScaled(state, config_width, config_height,
                                      incoming_stack_byte, stack_element,
                                      runtime));
    state.WriteDword(0u, CeguiRendererState::kVariant4Vtable);
    state.WriteDword(256u, 0x44480000u);
    state.WriteDword(248u, 0x44160000u);
    state.WriteFloat(280u, ScaledWidth(config_width));
    state.WriteFloat(284u, ScaledHeight(config_height));
    return state;
}

CeguiRendererState& CeguiRenderer::ConstructWide(
    CeguiRendererState& state, const std::int32_t config_width,
    const std::int32_t config_height,
    const std::uint8_t incoming_stack_byte,
    const std::array<std::uint8_t, 28>& stack_element,
    ICeguiRendererRuntime& runtime) {
    static_cast<void>(ConstructScaled(state, config_width, config_height,
                                      incoming_stack_byte, stack_element,
                                      runtime));
    state.WriteDword(272u, 0x3faaaaabu);
    state.WriteDword(276u, 0x3faaaaabu);
    state.WriteDword(0u, CeguiRendererState::kWideVtable);
    state.WriteDword(280u, 0x42d55558u);
    runtime.AssignRect(state, 284u, 244u);
    const auto offset = static_cast<double>(state.ReadFloat(280u)) /
                        static_cast<double>(state.ReadFloat(272u));
    state.WriteFloat(292u, static_cast<float>(
        static_cast<double>(state.ReadFloat(292u)) - offset));
    state.WriteFloat(296u, static_cast<float>(
        offset + static_cast<double>(state.ReadFloat(296u))));
    return state;
}

void CeguiRenderer::Destroy(CeguiRendererState& state,
                            ICeguiRendererRuntime& runtime) {
    state.WriteDword(0u, CeguiRendererState::kVtable);
    runtime.ProcessCollectionUntilEmpty(state);
    const auto sentinel = state.ReadDword(236u);
    auto node = runtime.ReadDword(sentinel);
    while (node != sentinel) {
        const auto next = runtime.ReadDword(node);
        runtime.RemoveListElement(state, node);
        node = next;
    }
    runtime.Free(sentinel);
    state.WriteDword(236u, 0u);
    state.WriteDword(240u, 0u);
    for (const auto begin_offset : {220u, 204u, 188u}) {
        runtime.Free(state.ReadDword(begin_offset));
        state.WriteDword(begin_offset, 0u);
        state.WriteDword(begin_offset + 4u, 0u);
        state.WriteDword(begin_offset + 8u, 0u);
    }
    runtime.DestroyBase(state);
}

}  // namespace pal4
