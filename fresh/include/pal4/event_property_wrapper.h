#pragma once

#include "pal4/property_forwarding.h"

#include <cstdint>

namespace pal4 {

class IEventPropertyWrapperRuntime : public IPropertyForwardingRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DestroyChild(std::uint32_t child, std::uint32_t flag) = 0;
    virtual void DecrementTempOff() = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class EventPropertyWrapperRuntimeStub final
    : public IEventPropertyWrapperRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t Invoke0(std::uint32_t object,
                          std::uint32_t slot) override;
    std::uint32_t Invoke1(std::uint32_t object, std::uint32_t slot,
                          std::uint32_t first) override;
    std::uint32_t Invoke2(std::uint32_t object, std::uint32_t slot,
                          std::uint32_t first,
                          std::uint32_t second) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    void DestroyChild(std::uint32_t child, std::uint32_t flag) override;
    void DecrementTempOff() override;
    void Free(std::uint32_t address) override;
};

class EventPropertyWrapper final {
public:
    // IDA 0x5ef480 / 0x5ef4b0 / 0x5ef4c0.
    [[nodiscard]] static std::uint32_t ForwardSet(
        std::uint32_t wrapper, std::uint32_t key, std::uint32_t value,
        IEventPropertyWrapperRuntime& runtime);
    [[nodiscard]] static std::uint32_t Call8(
        std::uint32_t wrapper, IEventPropertyWrapperRuntime& runtime);
    [[nodiscard]] static std::uint32_t Call12(
        std::uint32_t wrapper, IEventPropertyWrapperRuntime& runtime);
    // IDA 0x5ef4f0 / 0x5ef4d0.
    static void Cleanup(std::uint32_t wrapper,
                        IEventPropertyWrapperRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t wrapper, std::uint32_t flag,
        IEventPropertyWrapperRuntime& runtime);
};

}  // namespace pal4
