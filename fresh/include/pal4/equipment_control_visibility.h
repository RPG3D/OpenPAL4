#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/equipment_window.h"

namespace pal4 {

class IEquipmentControlVisibilityRuntime {
public:
    virtual ~IEquipmentControlVisibilityRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t IconImage(
        std::uint32_t index, std::uint32_t variant,
        std::uint32_t flags) = 0;
    virtual void SetImage(std::uint32_t window, std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view image) = 0;
    virtual void SetModelVisible(EquipmentWindowState& state,
                                 bool visible) = 0;
};

class EquipmentControlVisibilityRuntimeStub :
    public IEquipmentControlVisibilityRuntime {
public:
    void SetVisible(std::uint32_t, bool) override {}
    std::uint32_t IconImage(std::uint32_t, std::uint32_t,
                            std::uint32_t) override { return 0; }
    void SetImage(std::uint32_t, std::uint32_t) override {}
    std::uint32_t GetImage(std::string_view, std::string_view) override {
        return 0;
    }
    void SetModelVisible(EquipmentWindowState&, bool) override {}
};

class EquipmentControlVisibility final {
public:
    static void SetVisible(EquipmentWindowState& state, bool visible,
                           IEquipmentControlVisibilityRuntime& runtime);
};

}  // namespace pal4
