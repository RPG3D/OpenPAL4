#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct XmlNodeArrayState {
    std::array<std::uint32_t, 4> words{};
};
static_assert(sizeof(XmlNodeArrayState) == 16u);

class IXmlNodeArrayRuntime {
public:
    virtual ~IXmlNodeArrayRuntime() = default;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual std::uint32_t CopyConstruct(std::uint32_t destination,
                                        std::uint32_t source) = 0;
    virtual std::uint32_t CopyAssign(std::uint32_t destination,
                                     std::uint32_t source) = 0;
    virtual void DestroyNode(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class XmlNodeArrayRuntimeStub final : public IXmlNodeArrayRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t CopyConstruct(std::uint32_t destination,
                                std::uint32_t source) override;
    std::uint32_t CopyAssign(std::uint32_t destination,
                             std::uint32_t source) override;
    void DestroyNode(std::uint32_t address) override;
    void Free(std::uint32_t address) override;
};

class XmlNodeArray final {
public:
    [[nodiscard]] static std::uint32_t CopyConstructNode(
        std::uint32_t destination, std::uint32_t source,
        IXmlNodeArrayRuntime& runtime, std::uint32_t undefinedEax = 0u);
    [[nodiscard]] static std::uint32_t InsertCopies(
        XmlNodeArrayState& state, std::uint32_t position,
        std::uint32_t count, std::uint32_t source,
        IXmlNodeArrayRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        XmlNodeArrayState& state, IXmlNodeArrayRuntime& runtime);
};

}  // namespace pal4
