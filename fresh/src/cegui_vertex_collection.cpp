#include "pal4/cegui_vertex_collection.h"

#include <limits>
#include <new>
#include <stdexcept>

namespace pal4 {

std::uint32_t CeguiVertexCollectionStub::Allocate(
    const std::uint32_t bytes) {
    const auto result = next_address_;
    next_address_ += bytes + 0x10u;
    return result;
}
void CeguiVertexCollectionStub::Free(std::uint32_t /*address*/) {}
void CeguiVertexCollectionStub::CopyRecord(
    const std::uint32_t destination, const std::uint32_t source) {
    records_[destination] = records_.at(source);
}
void CeguiVertexCollectionStub::WriteRecord(
    const std::uint32_t destination, const CeguiVertexRecord& value) {
    records_[destination] = value;
}
const CeguiVertexRecord& CeguiVertexCollectionStub::Record(
    const std::uint32_t address) const {
    return records_.at(address);
}

std::uint32_t CeguiVertexCollection::Insert(
    CeguiRendererState& state, const std::uint32_t position,
    const std::uint32_t count, const CeguiVertexRecord& value,
    ICeguiVertexCollectionRuntime& runtime) {
    constexpr std::uint32_t stride = 28u;
    const auto begin = state.ReadDword(204u);
    const auto end = state.ReadDword(208u);
    const auto capacity = state.ReadDword(212u);
    if (position < begin || position > end ||
        (position - begin) % stride != 0u ||
        (end - begin) % stride != 0u ||
        capacity < end || (capacity - end) % stride != 0u) {
        throw std::invalid_argument("invalid 28-byte collection range");
    }
    if (count > static_cast<std::uint32_t>(
                    std::numeric_limits<std::int32_t>::max() / stride)) {
        throw std::length_error("28-byte collection count overflows");
    }
    const auto used = begin == 0u ? 0u : (end - begin) / stride;
    const auto available = (capacity - end) / stride;
    const auto tail = (end - position) / stride;
    const auto inserted_bytes = stride * count;

    if (available < count) {
        const auto growth = used > count ? used : count;
        if (used + static_cast<std::uint64_t>(growth) >
            static_cast<std::uint32_t>(
                std::numeric_limits<std::int32_t>::max() / stride)) {
            throw std::length_error("28-byte collection growth overflows");
        }
        const auto capacity_count = used + growth;
        const auto new_begin = runtime.Allocate(stride * capacity_count);
        if (new_begin == 0u) {
            throw std::bad_alloc();
        }
        const auto prefix = (position - begin) / stride;
        for (std::uint32_t index = 0u; index < prefix; ++index) {
            runtime.CopyRecord(new_begin + stride * index,
                               begin + stride * index);
        }
        for (std::uint32_t index = 0u; index < count; ++index) {
            runtime.WriteRecord(new_begin + stride * (prefix + index), value);
        }
        for (std::uint32_t index = 0u; index < tail; ++index) {
            runtime.CopyRecord(
                new_begin + stride * (prefix + count + index),
                position + stride * index);
        }
        runtime.Free(begin);
        state.WriteDword(212u, new_begin + stride * capacity_count);
        state.WriteDword(204u, new_begin);
        state.WriteDword(208u, new_begin + stride * (used + count));
        return 0u;
    }

    if (count == 0u) {
        return 0u;
    }
    if (tail < count) {
        for (std::uint32_t index = 0u; index < tail; ++index) {
            runtime.CopyRecord(position + inserted_bytes + stride * index,
                               position + stride * index);
        }
        for (std::uint32_t index = 0u; index < count - tail; ++index) {
            runtime.WriteRecord(end + stride * index, value);
        }
        for (std::uint32_t index = 0u; index < tail; ++index) {
            runtime.WriteRecord(position + stride * index, value);
        }
        const auto new_end = end + inserted_bytes;
        state.WriteDword(208u, new_end);
        return new_end;
    }

    for (std::uint32_t index = 0u; index < count; ++index) {
        runtime.CopyRecord(end + stride * index,
                           end - inserted_bytes + stride * index);
    }
    for (auto source = end - inserted_bytes; source != position;) {
        source -= stride;
        runtime.CopyRecord(source + inserted_bytes, source);
    }
    for (std::uint32_t index = 0u; index < count; ++index) {
        runtime.WriteRecord(position + stride * index, value);
    }
    state.WriteDword(208u, end + inserted_bytes);
    return position + inserted_bytes;
}

std::uint32_t CeguiVertexCollection::MoveRange(
    CeguiRendererState& state, const std::uint32_t destination,
    const std::uint32_t source,
    ICeguiVertexCollectionRuntime& runtime) {
    const auto old_end = state.ReadDword(208u);
    if (source > old_end || (old_end - source) % 28u != 0u) {
        throw std::invalid_argument("invalid 28-byte move range");
    }
    auto target = destination;
    for (auto current = source; current != old_end;
         current += 28u, target += 28u) {
        runtime.CopyRecord(target, current);
    }
    state.WriteDword(208u, target);
    return destination;
}

}  // namespace pal4
