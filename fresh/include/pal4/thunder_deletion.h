#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IThunderDeletionRuntime {
public:
    virtual ~IThunderDeletionRuntime() = default;
    virtual void Destroy(ThunderObjectState& state) = 0;
    virtual void Free(std::uint32_t object_address) = 0;
};

class ThunderDeletion final {
public:
    // IDA 0x61ad70.
    [[nodiscard]] static std::uint32_t Delete(
        ThunderObjectState& state, std::uint32_t object_address,
        std::uint32_t flags, IThunderDeletionRuntime& runtime);

    // IDA 0x61cc90..0x61cd10: nine +20..+52 facet adjustment thunks.
    [[nodiscard]] static std::uint32_t DeleteFromFacet(
        ThunderObjectState& state, std::uint32_t facet_address,
        std::uint32_t facet_offset, std::uint32_t flags,
        IThunderDeletionRuntime& runtime);
    static constexpr std::array<std::uint32_t, 9u> kFacetOffsets{
        20u, 24u, 28u, 32u, 36u, 40u, 44u, 48u, 52u};
};

}  // namespace pal4
