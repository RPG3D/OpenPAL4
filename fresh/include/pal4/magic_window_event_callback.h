#pragma once

#include <cstdint>

namespace pal4 {

class IMagicWindowEventCallbackRuntime {
public:
    virtual ~IMagicWindowEventCallbackRuntime() = default;
    virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class MagicWindowEventCallbackRuntimeStub :
    public IMagicWindowEventCallbackRuntime {
public:
    std::uint32_t Allocate(std::uint32_t) override { return 0; }
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
};

class MagicWindowEventCallback final {
public:
    static constexpr std::uint32_t kProvisionalVtable = 0x843500u;
    static constexpr std::uint32_t kCloneVtable = 0x843914u;
    static constexpr std::uint32_t kPanelVtable = 0x843994u;

    static std::uint32_t Clone(std::uint32_t source,
                               IMagicWindowEventCallbackRuntime& runtime);
    static std::uint32_t ConstructPanel(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t flags, std::uint32_t owner,
        IMagicWindowEventCallbackRuntime& runtime);
};

}  // namespace pal4
