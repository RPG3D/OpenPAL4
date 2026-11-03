#pragma once

#include "pal4/clump_object.h"

#include <cstdint>

namespace pal4 {

class IClumpObjectControlRuntime {
public:
    virtual ~IClumpObjectControlRuntime() = default;
    virtual void StopFacet(ClumpObjectState& state,
                           std::uint32_t offset) = 0;
    virtual void CleanupXml(ClumpObjectState& state,
                            std::uint32_t offset) = 0;
    // The native text allocator receives the zero-length g_EmptyErrorString.
    virtual void AssignEmptyText(ClumpObjectState& state,
                                 std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t SystemInstance() = 0;
    virtual void CleanupClump(std::uint32_t manager,
                              std::uint32_t world) = 0;
    virtual void CleanupTexture(std::uint32_t manager) = 0;
    virtual void DeleteManager(std::uint32_t manager,
                               std::uint32_t flag) = 0;
    virtual void FreeAnimation(std::uint32_t animation) = 0;
    virtual void NotifyOwner(std::uint32_t owner,
                             ClumpObjectState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        ClumpObjectState& state, std::uint32_t flag) = 0;
};

class ClumpObjectControlRuntimeStub final : public IClumpObjectControlRuntime {
public:
    void StopFacet(ClumpObjectState&, std::uint32_t) override {}
    void CleanupXml(ClumpObjectState&, std::uint32_t) override {}
    void AssignEmptyText(ClumpObjectState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SystemInstance() override { return 0u; }
    void CleanupClump(std::uint32_t, std::uint32_t) override {}
    void CleanupTexture(std::uint32_t) override {}
    void DeleteManager(std::uint32_t, std::uint32_t) override {}
    void FreeAnimation(std::uint32_t) override {}
    void NotifyOwner(std::uint32_t, ClumpObjectState&) override {}
    [[nodiscard]] std::uint32_t DeleteObject(
        ClumpObjectState&, std::uint32_t) override { return 0u; }
};

}  // namespace pal4
