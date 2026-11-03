#pragma once

#include "pal4/cylinder_object.h"

#include <cstdint>

namespace pal4 {

class ICylinderObjectControlRuntime {
public:
    virtual ~ICylinderObjectControlRuntime() = default;
    virtual void StopFacet(CylinderObjectState& state,
                           std::uint32_t offset) = 0;
    virtual void DestroyModel(std::uint32_t model) = 0;
    virtual void FreeModel(std::uint32_t model) = 0;
    virtual void DestroyDocument(std::uint32_t document) = 0;
    virtual void FreeDocument(std::uint32_t document) = 0;
    virtual void ProcessEvent(std::uint32_t system,
                              CylinderObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteSelf(
        CylinderObjectState& state, std::uint32_t flags) = 0;
};

class CylinderObjectControlRuntimeStub final
    : public ICylinderObjectControlRuntime {
public:
    void StopFacet(CylinderObjectState&, std::uint32_t) override {}
    void DestroyModel(std::uint32_t) override {}
    void FreeModel(std::uint32_t) override {}
    void DestroyDocument(std::uint32_t) override {}
    void FreeDocument(std::uint32_t) override {}
    void ProcessEvent(std::uint32_t, CylinderObjectState&) override {}
    [[nodiscard]] std::uint32_t DeleteSelf(
        CylinderObjectState&, std::uint32_t) override { return 0u; }
};

class CylinderObjectControl final {
public:
    // IDA 0x60d2f0.
    [[nodiscard]] static std::uint32_t StopAndDestroy(
        CylinderObjectState& state, ICylinderObjectControlRuntime& runtime);
};

}  // namespace pal4
