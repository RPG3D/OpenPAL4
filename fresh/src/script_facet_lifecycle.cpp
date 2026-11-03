#include "pal4/script_facet_lifecycle.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kVtableBase = 0x848430u;
constexpr std::uint32_t kFacetCount = 26u;
constexpr std::array<std::uint32_t, 6> kEarlyDestructorOrder{
    2u, 3u, 4u, 5u, 0u, 1u};

std::uint32_t FacetIndex(std::uint32_t entry, std::uint32_t first,
                         std::uint32_t stride) {
    if (entry < first || entry >= first + kFacetCount * stride ||
        (entry - first) % stride != 0u)
        throw std::invalid_argument("unknown script facet lifecycle entry");
    return (entry - first) / stride;
}

void Restore(std::uint32_t index, std::uint32_t object,
             IScriptFacetLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtableBase + index * 8u);
}

}  // namespace

void ScriptFacetLifecycle::RestoreVtable(
    std::uint32_t native_entry, std::uint32_t object,
    IScriptFacetLifecycleRuntime& runtime) {
    Restore(FacetIndex(native_entry, 0x5886f0u, 0x30u), object, runtime);
}

std::uint32_t ScriptFacetLifecycle::Destroy(
    std::uint32_t native_entry, std::uint32_t object,
    std::uint32_t flags, IScriptFacetLifecycleRuntime& runtime) {
    std::uint32_t index;
    if (native_entry >= 0x5883b0u && native_entry <= 0x5886d0u) {
        const auto ordinal = FacetIndex(native_entry, 0x5883b0u, 0x20u);
        index = ordinal < kEarlyDestructorOrder.size()
                    ? kEarlyDestructorOrder[ordinal]
                    : ordinal;
    } else {
        index = FacetIndex(native_entry, 0x588700u, 0x30u);
    }
    Restore(index, object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace pal4
