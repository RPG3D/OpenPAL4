#include "pal4/record_array_copier.h"

#include <bit>

namespace pal4 {

std::uint32_t RecordArrayCopyRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t RecordArrayCopyRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void RecordArrayCopyRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}

namespace {

RecordArrayState& Copy(RecordArrayState& destination,
                       const RecordArrayState& source,
                       std::uint32_t stride,
                       IRecordArrayCopyRuntime& runtime) {
    destination.marker = (destination.marker & 0xffffff00u) |
                         (source.marker & 0xffu);
    const auto signed_bytes = std::bit_cast<std::int32_t>(
        source.end - source.begin);
    auto count = source.begin == 0u ? 0 :
        (stride == 12u ? signed_bytes / 12 : signed_bytes >> 3);
    if (count < 0)
        count = 0;
    const auto allocation = runtime.Allocate(
        static_cast<std::uint32_t>(count) * stride);
    destination.begin = allocation;
    auto output = allocation;
    for (auto input = source.begin; input != source.end;
         input += stride, output += stride) {
        if (output == 0u)
            continue;
        for (std::uint32_t offset = 0u; offset < stride; offset += 4u)
            runtime.WriteDword(output + offset,
                               runtime.ReadDword(input + offset));
    }
    destination.end = output;
    destination.capacity_end = output;
    return destination;
}

}  // namespace

RecordArrayState& RecordArrayCopier::CopyTriples(
    RecordArrayState& destination, const RecordArrayState& source,
    IRecordArrayCopyRuntime& runtime) {
    return Copy(destination, source, 12u, runtime);
}

RecordArrayState& RecordArrayCopier::CopyPairs(
    RecordArrayState& destination, const RecordArrayState& source,
    IRecordArrayCopyRuntime& runtime) {
    return Copy(destination, source, 8u, runtime);
}

}  // namespace pal4
