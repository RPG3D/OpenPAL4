#pragma once

#include <cstdint>

namespace pal4 {

class IPalTestWindowEventCallbackRuntime {
public:
    virtual ~IPalTestWindowEventCallbackRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class PalTestWindowEventCallbackRuntimeStub :
    public IPalTestWindowEventCallbackRuntime {
public:
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class PalTestWindowEventCallback final {
public:
    static constexpr std::uint32_t kVtable = 0x84381cu;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t context, std::uint32_t owner,
        IPalTestWindowEventCallbackRuntime& runtime);
};

}  // namespace pal4
