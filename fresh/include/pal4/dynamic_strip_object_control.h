#pragma once

#include "pal4/dynamic_strip_object.h"

#include <cstdint>

namespace pal4 {

class IDynamicStripObjectControlRuntime {
public:
    virtual ~IDynamicStripObjectControlRuntime() = default;
    virtual void CleanupResources(DynamicStripObjectState& state) = 0;
    virtual void ProcessEvent(std::uint32_t system,
                              DynamicStripObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteSelf(
        DynamicStripObjectState& state, std::uint32_t flags) = 0;
};

class DynamicStripObjectControlRuntimeStub final
    : public IDynamicStripObjectControlRuntime {
public:
    void CleanupResources(DynamicStripObjectState&) override {}
    void ProcessEvent(std::uint32_t, DynamicStripObjectState&) override {}
    [[nodiscard]] std::uint32_t DeleteSelf(
        DynamicStripObjectState&, std::uint32_t) override { return 0u; }
};

class DynamicStripObjectControl final {
public:
    // IDA 0x60f130.
    [[nodiscard]] static std::uint32_t StopAndDestroy(
        DynamicStripObjectState& state,
        IDynamicStripObjectControlRuntime& runtime);
};

}  // namespace pal4
