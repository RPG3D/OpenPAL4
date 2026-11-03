#pragma once

#include <cstdint>

#include "pal4/base_object_named_weapon.h"

namespace pal4 {

class BaseObjectHandEffect final {
public:
    // IDA 0x4436e0 and 0x4437a0.
    [[nodiscard]] static std::uint32_t SetupRight(
        BaseObjectState& state, std::uint32_t effect_name,
        IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetupLeft(
        BaseObjectState& state, std::uint32_t effect_name,
        IBaseObjectNamedWeaponRuntime& runtime);

    // IDA 0x443860 and 0x443880.
    [[nodiscard]] static std::uint32_t UpdateRight(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateLeft(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);

    // IDA 0x4438a0 through 0x443900.
    [[nodiscard]] static std::uint32_t CleanupAll(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupWeapons(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupWeaponEffects(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
    [[nodiscard]] static std::uint32_t UpdateWeaponEffects(
        BaseObjectState& state, IBaseObjectNamedWeaponRuntime& runtime);
};

}  // namespace pal4
