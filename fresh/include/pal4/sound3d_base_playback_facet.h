#pragma once

#include <cstdint>

namespace pal4 {

struct Sound3DBasePlaybackFacetStorage final {
    std::uint32_t vtable = 0u;
};

class ISound3DBasePlaybackFacetRuntime {
public:
    virtual ~ISound3DBasePlaybackFacetRuntime() = default;
    virtual void Free(Sound3DBasePlaybackFacetStorage& facet) = 0;
};

class Sound3DBasePlaybackFacetRuntimeStub final
    : public ISound3DBasePlaybackFacetRuntime {
public:
    void Free(Sound3DBasePlaybackFacetStorage&) override {}
};

class Sound3DBasePlaybackFacet final {
public:
    static constexpr std::uint32_t kVtable = 0x84a9d8u;
    // IDA 0x617e00 / 0x617e10.
    static void Initialize(Sound3DBasePlaybackFacetStorage& facet);
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        Sound3DBasePlaybackFacetStorage& facet, std::uint32_t flags,
        ISound3DBasePlaybackFacetRuntime& runtime);
};

}  // namespace pal4
