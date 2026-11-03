#include "pal4/host_server_data_codec.h"

namespace pal4 {
namespace {

template <class Runtime>
void ReleaseMarker(const ConfigStringState& marker, Runtime& runtime) {
    if (marker.data == 0u)
        return;
    const auto header = marker.data - 1u;
    const auto references = runtime.ReadByte(header);
    if (references == 0u || references == 0xffu)
        runtime.Free(header);
    else
        runtime.WriteByte(header, static_cast<std::uint8_t>(references - 1u));
}

void WriteSlot(std::uint32_t begin, std::uint32_t end,
               std::uint32_t element_size, std::uint32_t stream,
               IHostServerStreamRuntime& runtime) {
    const auto count = begin == 0u ? 0u :
                       static_cast<std::uint32_t>(
                           static_cast<std::int32_t>(end - begin) >>
                           (element_size == 4u ? 2 : 3));
    (void)runtime.WriteCount(stream, count);
    (void)runtime.WriteBytes(stream, begin, count * element_size);
}

template <class Slot>
void ReadSlot(Slot& slot, std::uint32_t stride, std::uint32_t stream,
              IHostServerReadRuntime& runtime) {
    CollectionSlotState working{
        slot.reserved, slot.begin, slot.end, slot.capacity_end};
    if (stride == 8u)
        (void)CollectionSlotOperations::CopyPairs(
            working, working.begin, working.end, runtime);
    else
        working.end = working.begin;

    std::uint32_t count = 0u;
    (void)runtime.ReadCount(stream, count);
    if (stride == 8u) {
        (void)CollectionSlotOperations::ReservePairs(working, count, runtime);
        if (count != 0u) {
            const auto fill = runtime.PairFillWords();
            (void)CollectionSlotOperations::ResizePairs(
                working, count, fill[0], fill[1], runtime);
        } else {
            (void)CollectionSlotOperations::ResizePairs(
                working, count, 0u, 0u, runtime);
        }
    } else {
        (void)CollectionSlotOperations::ReserveWords(working, count, runtime);
        (void)CollectionSlotOperations::ResizeWords(
            working, count, 0u, runtime);
    }
    slot.begin = working.begin;
    slot.end = working.end;
    slot.capacity_end = working.capacity_end;
    (void)runtime.ReadBytes(stream, working.begin, count * stride);
}

}  // namespace

void HostServerStreamRuntimeStub::AssignMarker(ConfigStringState& marker,
                                               std::string_view text) {
    marker.size = static_cast<std::uint32_t>(text.size());
}
std::uint32_t HostServerStreamRuntimeStub::EmptyStringAddress() { return 0u; }
std::uint8_t HostServerStreamRuntimeStub::WriteCount(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t HostServerStreamRuntimeStub::WriteBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::uint8_t HostServerStreamRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void HostServerStreamRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void HostServerStreamRuntimeStub::Free(std::uint32_t) {}

std::uint8_t HostServerReadRuntimeStub::ReadMarker(
    std::uint32_t, ConfigStringState&) { return 0u; }
std::int32_t HostServerReadRuntimeStub::CompareMarker(
    const ConfigStringState&, std::string_view) { return 1; }
std::uint8_t HostServerReadRuntimeStub::ReadCount(
    std::uint32_t, std::uint32_t& count) {
    count = 0u;
    return 0u;
}
std::uint8_t HostServerReadRuntimeStub::ReadBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
std::array<std::uint32_t, 2> HostServerReadRuntimeStub::PairFillWords() {
    return {};
}
std::uint32_t HostServerReadRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t HostServerReadRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void HostServerReadRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void HostServerReadRuntimeStub::BeforeFreePairs(
    std::uint32_t, std::uint32_t) {}
std::uint8_t HostServerReadRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}
void HostServerReadRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void HostServerReadRuntimeStub::Free(std::uint32_t) {}

bool HostServerDataCodec::Serialize(
    const IndexedCollectionResetState& state, std::uint32_t stream,
    IHostServerStreamRuntime& runtime) {
    ConfigStringState marker{};
    marker.flag = static_cast<std::uint8_t>(stream);
    runtime.AssignMarker(marker, kMarker);
    if (runtime.WriteCount(stream, marker.size) != 0u) {
        const auto data = marker.data == 0u ?
                              runtime.EmptyStringAddress() : marker.data;
        (void)runtime.WriteBytes(stream, data, marker.size);
    }

    for (std::size_t index = 0u; index < 5u; ++index) {
        const auto& values = state.values[index];
        WriteSlot(values.begin, values.end, 4u, stream, runtime);
        const auto& pairs = state.key_values[index];
        WriteSlot(pairs.begin, pairs.end, 8u, stream, runtime);
    }
    const auto& last = state.key_values[5];
    WriteSlot(last.begin, last.end, 4u, stream, runtime);
    ReleaseMarker(marker, runtime);
    return true;
}

bool HostServerDataCodec::Deserialize(
    IndexedCollectionResetState& state, std::uint32_t stream,
    IHostServerReadRuntime& runtime) {
    ConfigStringState marker{};
    marker.flag = static_cast<std::uint8_t>(stream);
    (void)runtime.ReadMarker(stream, marker);
    if (runtime.CompareMarker(marker, kMarker) != 0) {
        ReleaseMarker(marker, runtime);
        return false;
    }

    for (std::size_t index = 0u; index < 5u; ++index) {
        ReadSlot(state.values[index], 4u, stream, runtime);
        ReadSlot(state.key_values[index], 8u, stream, runtime);
    }
    ReadSlot(state.key_values[5], 4u, stream, runtime);
    ReleaseMarker(marker, runtime);
    return true;
}

}  // namespace pal4
