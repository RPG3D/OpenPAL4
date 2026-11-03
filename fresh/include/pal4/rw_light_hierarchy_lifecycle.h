#pragma once

#include "pal4/rw_light_operations.h"

#include <cstdint>

namespace pal4 {

struct RwLightHierarchyState final {
    std::uint32_t primary_light = 0;
    std::uint32_t secondary_light = 0;
};
static_assert(sizeof(RwLightHierarchyState) == 8u);

class IRwLightHierarchyRuntime {
public:
    virtual ~IRwLightHierarchyRuntime() = default;
    [[nodiscard]] virtual IRwLightRuntime& Lights() = 0;
    [[nodiscard]] virtual std::uint32_t SetActiveNormalMapLights(
        std::uint32_t secondary_light,
        std::uint32_t primary_light) = 0;
};

class RwLightHierarchyRuntimeStub final : public IRwLightHierarchyRuntime {
public:
    IRwLightRuntime& Lights() override { return lights_; }
    std::uint32_t SetActiveNormalMapLights(
        std::uint32_t, std::uint32_t) override { return 0; }

private:
    RwLightRuntimeStub lights_{};
};

class RwLightHierarchyLifecycle final {
public:
    static constexpr std::uint32_t kPrimaryLightAddress = 0x94d494u;
    static constexpr std::uint32_t kSecondaryLightAddress = 0x94d498u;

    [[nodiscard]] static std::uint32_t Initialize(
        RwLightHierarchyState& state,
        IRwLightHierarchyRuntime& runtime);
    [[nodiscard]] static std::uint32_t Shutdown(
        RwLightHierarchyState& state,
        IRwLightHierarchyRuntime& runtime);
};

}  // namespace pal4
