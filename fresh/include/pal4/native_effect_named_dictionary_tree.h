#pragma once

#include "pal4/rw_tex_dictionary_tree.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;

// Corresponds to 93CFC4/93CFC0. All users must share the same allocator and
// this state must outlive them. It is distinct from every effect registry nil.
struct NativeEffectNamedDictionarySharedState final {
    std::uint32_t nil{};
    std::uint32_t users{};
};

// Owns the named dictionary tree at effect singleton +52, borrowing its image.
// Releases SDK dictionary payloads before the nodes, then releases head/nil.
// External material users must be gone before Cleanup or destruction.
class NativeEffectNamedDictionaryTree final {
public:
    NativeEffectNamedDictionaryTree(NativeBaseObjectMemory&,
        RwTexDictionaryTreeState&, NativeEffectNamedDictionarySharedState&);
    ~NativeEffectNamedDictionaryTree();
    NativeEffectNamedDictionaryTree(const NativeEffectNamedDictionaryTree&) = delete;
    NativeEffectNamedDictionaryTree& operator=(const NativeEffectNamedDictionaryTree&) = delete;
    // Original 6641F0: keeps the head/nil so the tree can be reused.
    void Cleanup();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
    RwTexDictionaryTreeState& state_;
};
}  // namespace pal4
