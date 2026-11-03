#pragma once

#include <cstdint>

namespace pal4 {

class IThunderBaseFacetRuntime {
public:
    virtual ~IThunderBaseFacetRuntime() = default;
    virtual void Free(std::uint32_t facet_address) = 0;
};

class ThunderBaseFacet final {
public:
    static constexpr std::uint32_t kVtable = 0x84abd0u;

    // IDA 0x61cda0/0x61cdb0; standalone form of owner +48 facet.
    static void InstallVtable(std::uint32_t& vtable_word);
    [[nodiscard]] static std::uint32_t DeletingDestructor(
        std::uint32_t& vtable_word, std::uint32_t facet_address,
        std::uint32_t flags, IThunderBaseFacetRuntime& runtime);
};

}  // namespace pal4
