#include "pal4/cegui_rectangle_queue.h"



namespace pal4 {

std::uint32_t CeguiRectangleQueueStub::Forward(
    CeguiRendererState& /*state*/, const CeguiRectWords& /*rectangle*/,
    std::uint32_t /*unused*/, std::uint32_t /*depth*/,
    std::uint32_t /*texture*/, const CeguiRectWords& /*uv*/,
    const std::array<CeguiColourWords, 4>& /*colours*/,
    std::uint32_t /*colour_source_address*/) {
    return 0u;
}

void CeguiRectangleQueueStub::AssignRect(
    CeguiRectWords& destination, const CeguiRectWords& source) {
    destination = source;
}

void CeguiRectangleQueueStub::CopyColour(
    CeguiColourWords& destination, const CeguiColourWords& source) {
    destination = source;
}

std::int32_t CeguiRectangleQueueStub::Convert(const double value) {
    CeguiHostFloatToInt host;
    return host.Convert(value);
}

std::uint32_t CeguiRectangleQueueStub::Allocate(const std::uint32_t bytes) {
    const auto result = next_address_;
    next_address_ += bytes + 0x10u;
    return result;
}

void CeguiRectangleQueueStub::CopyRecord(
    const std::uint32_t destination, const std::uint32_t source) {
    records_[destination] = records_.at(source);
}

void CeguiRectangleQueueStub::FillRecord(
    const std::uint32_t destination, const CeguiRectangleRecord& record) {
    records_[destination] = record;
}

void CeguiRectangleQueueStub::CopyStructuredRange(
    const std::uint32_t begin, const std::uint32_t end,
    const CeguiRectangleRecord& record) {
    for (auto address = begin; address != end; address += 60u) {
        records_[address] = record;
    }
}

void CeguiRectangleQueueStub::RangeNoOp(
    std::uint32_t /*begin*/, std::uint32_t /*end*/) {}

void CeguiRectangleQueueStub::Free(std::uint32_t /*address*/) {}

std::uint32_t CeguiRectangleQueueStub::EffectCount(
    const CeguiRendererState& state) {
    const auto begin = state.ReadDword(188u);
    return begin == 0u ? 0u : (state.ReadDword(192u) - begin) / 60u;
}

const CeguiRectangleRecord& CeguiRectangleQueueStub::Record(
    const std::uint32_t address) const {
    return records_.at(address);
}

std::uint32_t CeguiRectangleQueue::Draw(
    CeguiRendererState& state, const CeguiRectWords& rectangle,
    const std::uint32_t unused, const std::uint32_t depth,
    const std::uint32_t texture, const CeguiRectWords& uv,
    const std::array<CeguiColourWords, 4>& colours,
    const std::uint32_t colour_source_address,
    ICeguiRectangleQueueRuntime& runtime) {
    if (state.ReadByte(260u) == 0u) {
        return runtime.Forward(state, rectangle, unused, depth,
                               texture, uv, colours,
                               colour_source_address);
    }

    CeguiRectangleRecord record;
    CeguiRectWords copied_rect{};
    CeguiRectWords copied_uv{};
    runtime.AssignRect(copied_rect, rectangle);
    runtime.AssignRect(copied_uv, uv);
    record.WriteDword(0u, texture);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        record.WriteDword(4u + index * 4u, copied_rect[index]);
        record.WriteDword(24u + index * 4u, copied_uv[index]);
    }
    record.WriteDword(20u, depth);
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        CeguiColourWords copied_colour{};
        runtime.CopyColour(copied_colour, colours[index]);
        CeguiPackedColor packed;
        static_cast<void>(packed.SetAllBytesFF());
        static_cast<void>(packed.SetFromFloats(
            copied_colour[0], copied_colour[1],
            copied_colour[2], copied_colour[3], runtime));
        record.WriteDword(40u + index * 4u,
                          static_cast<std::uint32_t>(packed.Bytes()[0]) |
                          (static_cast<std::uint32_t>(packed.Bytes()[1]) << 8u) |
                          (static_cast<std::uint32_t>(packed.Bytes()[2]) << 16u) |
                          (static_cast<std::uint32_t>(packed.Bytes()[3]) << 24u));
    }
    record.WriteDword(56u, colour_source_address);

    const auto begin = state.ReadDword(188u);
    const auto end = state.ReadDword(192u);
    const auto capacity = state.ReadDword(196u);
    if (begin != 0u && capacity - end >= CeguiRectangleRecord::kSize) {
        runtime.FillRecord(end, record);
        runtime.CopyStructuredRange(end, end, record);
        const auto new_end = end + CeguiRectangleRecord::kSize;
        state.WriteDword(192u, new_end);
        return new_end;
    }

    const auto old_count = begin == 0u ? 0u : (end - begin) / 60u;
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_capacity_count = old_count + growth;
    const auto new_begin = runtime.Allocate(60u * new_capacity_count);
    for (std::uint32_t index = 0u; index < old_count; ++index) {
        runtime.CopyRecord(new_begin + 60u * index, begin + 60u * index);
    }
    runtime.FillRecord(new_begin + 60u * old_count, record);
    runtime.RangeNoOp(begin, end);
    runtime.Free(begin);
    state.WriteDword(196u, new_begin + 60u * new_capacity_count);
    const auto count = runtime.EffectCount(state);
    state.WriteDword(188u, new_begin);
    const auto new_end = new_begin + 60u * (count + 1u);
    state.WriteDword(192u, new_end);
    return new_end;
}

}  // namespace pal4
