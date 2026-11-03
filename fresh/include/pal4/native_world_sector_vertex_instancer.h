#pragma once
#include <rwcore.h>
#include <rpworld.h>
#include <string>

namespace pal4 {
// Game53E550 using the shared game algorithm and matched SDK converters.
// The SDK resource entry owns successful declarations/buffers, including failed
// instance attempts. No tangent ABI substitution is permitted.
class NativeWorldSectorVertexInstancer final {
public:
    [[nodiscard]] static RwBool Instance(RpWorld& world, RpWorldSector& sector,
                                       RxD3D9ResEntryHeader& header);
    // Typed RW callback; errors stop instancing and are available to the caller.
    [[nodiscard]] static RwBool Callback(void* sector, RxD3D9ResEntryHeader* header,
                                       RwBool reinstance) noexcept;
    [[nodiscard]] static const std::string& LastError() noexcept;
};
}
