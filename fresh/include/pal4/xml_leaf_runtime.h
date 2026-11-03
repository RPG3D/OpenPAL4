#pragma once

#include <cstdint>

namespace pal4 {

class IXmlLeafRuntime {
public:
    virtual ~IXmlLeafRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CallVisitor(
        std::uint32_t visitor, std::uint32_t node,
        std::uint32_t vtable_offset) = 0;
};

class XmlLeafRuntimeStub final : public IXmlLeafRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    std::uint32_t CallVisitor(std::uint32_t, std::uint32_t,
                              std::uint32_t) override { return 0u; }
};

}  // namespace pal4
