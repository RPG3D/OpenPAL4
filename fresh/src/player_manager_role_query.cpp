#include "pal4/player_manager_role_query.h"

#include "pal4/player_manager_object.h"

namespace pal4 {

std::uint32_t PlayerManagerRoleQueryRuntimeStub::Manager() { return 0u; }
std::uint32_t PlayerManagerRoleQueryRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
std::uint8_t PlayerManagerRoleQueryRuntimeStub::ReadByte(std::uint32_t) {
    return 0u;
}

bool PlayerManagerRoleQuery::FirstPlayerIs102(
    IPlayerManagerRoleQueryRuntime& runtime) {
    const auto first = PlayerManagerObject::GetPlayerByIndex(runtime.Manager(), 0);
    if (first == 0u) return false;

    constexpr char expected[] = "102";
    constexpr std::uint32_t expected_length = sizeof(expected) - 1u;
    const auto length = runtime.ReadDword(first + kNameLengthOffset);
    const auto data = runtime.ReadDword(first + kNamePointerOffset);
    const auto common_length = length < expected_length ? length : expected_length;
    for (std::uint32_t i = 0u; i < common_length; ++i) {
        if (runtime.ReadByte(data + i) !=
            static_cast<std::uint8_t>(expected[i]))
            return false;
    }
    return length == expected_length;
}

}  // namespace pal4
