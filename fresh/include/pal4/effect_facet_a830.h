#pragma once

#include <cstdint>

namespace pal4 {

struct EffectFacetA830Storage final {
    std::uint32_t vtable = 0u;
};

class IEffectFacetA830Runtime {
public:
    virtual ~IEffectFacetA830Runtime() = default;
    virtual void Free(EffectFacetA830Storage& facet) = 0;
};

class EffectFacetA830RuntimeStub final : public IEffectFacetA830Runtime {
public:
    void Free(EffectFacetA830Storage&) override {}
};

class EffectFacetA830 final {
public:
    static constexpr std::uint32_t kVtable = 0x84a830u;
    // IDA 0x615080 / 0x615090.
    static void Initialize(EffectFacetA830Storage& facet);
    [[nodiscard]] static std::uintptr_t DeletingDestroy(
        EffectFacetA830Storage& facet, std::uint32_t flags,
        IEffectFacetA830Runtime& runtime);
};

}  // namespace pal4
