#include "pal4/record_accessors.h"

namespace pal4 {

std::uint32_t NilStringDataHeaderAccessors::SetField00(
    NilStringDataHeader& record, const std::uint32_t value) noexcept {
    record.field_00 = value;
    return value;
}

std::uint32_t NilStringDataHeaderAccessors::SetField04(
    NilStringDataHeader& record, const std::uint32_t value) noexcept {
    record.field_04 = value;
    return value;
}

std::uint32_t NilStringDataHeaderAccessors::SetField08(
    NilStringDataHeader& record, const std::uint32_t value) noexcept {
    record.field_08 = value;
    return value;
}

std::uint32_t PropertyValueRecord::GetAndClearAt14(Fields& fields) noexcept {
    const auto value = fields[5];
    fields[3] = 0;
    return value;
}

std::uint32_t PropertyValueRecord::GetAndClearAt18(Fields& fields) noexcept {
    const auto value = fields[6];
    fields[3] = 0;
    return value;
}

}  // namespace pal4
