#pragma once

#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;
struct DialogObjectState;
class IDialogObjectPreDeleteRuntime;

// Releases facet lookup nodes/names, preserving the borrowed facet payloads and
// the tree's head/nil. The Dialog owner supplies its actual shared text-tree nil.
class NativeDialogFacetCleanup final {
public:
    static void Clear(NativeBaseObjectMemory&, DialogObjectState&,
        std::uint32_t shared_nil);
    static void PrepareForDelete(NativeBaseObjectMemory&,DialogObjectState&,
        std::uint32_t shared_nil,IDialogObjectPreDeleteRuntime&);
};
}  // namespace pal4
