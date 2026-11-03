#include "pal4/property_string_assignment.h"

namespace pal4 {

std::uint8_t PropertyStringAssignment::Assign(
    const std::uint32_t record_offset, const std::uint32_t target,
    const std::uint32_t source_record, IPropertyStringRuntime& runtime) {
    const auto destination = target + record_offset;
    const auto length = runtime.ReadDword(source_record + 8u);
    if (destination == source_record) {
        if (runtime.ReadDword(destination + 8u) < length)
            runtime.ThrowRange();
        (void)runtime.DetachAndReset(destination);
        const auto remaining = runtime.ReadDword(destination + 8u) - length;
        std::uint32_t erased = 0xffffffffu;
        if (remaining != 0xffffffffu) erased = remaining;
        if (remaining == 0xffffffffu || erased != 0u) {
            const auto data = runtime.ReadDword(destination + 4u);
            runtime.Copy(data + length, data + length + erased,
                         remaining - erased);
            const auto new_length = runtime.ReadDword(destination + 8u) -
                erased;
            if (runtime.AllocateText(destination, new_length, 0u) != 0u)
                runtime.SetTextSize(destination, new_length);
        }
        return runtime.DetachAndReset(destination);
    }

    if (length != 0u) {
        const auto raw_source = runtime.ReadDword(source_record + 4u);
        const auto data = raw_source == 0u ? kEmptyLogMessage : raw_source;
        if (runtime.ReadByte(data - 1u) < 0xfeu) {
            const auto old_data = runtime.ReadDword(destination + 4u);
            if (old_data != 0u) {
                const auto header = old_data - 1u;
                const auto references = runtime.ReadByte(header);
                if (references == 0u || references == 0xffu)
                    runtime.Free(header);
                else
                    runtime.WriteByte(header,
                        static_cast<std::uint8_t>(references - 1u));
            }
            runtime.WriteDword(destination + 4u, 0u);
            runtime.WriteDword(destination + 8u, 0u);
            runtime.WriteDword(destination + 12u, 0u);
            runtime.WriteDword(destination + 4u, data);
            runtime.WriteDword(destination + 8u, length);
            runtime.WriteDword(destination + 12u,
                               runtime.ReadDword(source_record + 12u));
            runtime.WriteByte(data - 1u,
                static_cast<std::uint8_t>(runtime.ReadByte(data - 1u) + 1u));
            return static_cast<std::uint8_t>(data);
        }
    }

    const auto allocated = runtime.AllocateText(destination, length, 1u);
    if (allocated == 0u) return 0u;
    const auto raw_source = runtime.ReadDword(source_record + 4u);
    const auto data = raw_source == 0u ? kEmptyLogMessage : raw_source;
    const auto destination_data = runtime.ReadDword(destination + 4u);
    runtime.Copy(destination_data, data, length);
    runtime.WriteDword(destination + 8u, length);
    runtime.WriteByte(destination_data + length, 0u);
    return static_cast<std::uint8_t>(length);
}

std::uint32_t PropertyStringAssignment::ElementAddress(
    const std::uint32_t record_offset, const std::uint32_t target) noexcept {
    return target + record_offset;
}

}  // namespace pal4
