#pragma once

#include <cstdint>

#include "pal4/ui_object_lifecycle.h"

namespace pal4 {

class IUiObjectVisibilityRuntime {
public:
    virtual ~IUiObjectVisibilityRuntime() = default;
    virtual void SetParent(UiObjectState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertActive(std::uint32_t collection_address,
                              std::uint32_t member_address) = 0;
    [[nodiscard]] virtual int RemoveActive(
        std::uint32_t collection_address,
        std::uint32_t member_address) = 0;
    [[nodiscard]] virtual int SetTextAndVisibility(
        UiObjectState& state, std::uint32_t window) = 0;
};

class UiObjectVisibilityRuntimeStub : public IUiObjectVisibilityRuntime {
public:
    void SetParent(UiObjectState&, std::uint8_t,
                   std::uint32_t) override {}
    void SetVisible(std::uint32_t, bool) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] int RemoveActive(std::uint32_t,
                                    std::uint32_t) override { return 0; }
    [[nodiscard]] int SetTextAndVisibility(UiObjectState&,
                                           std::uint32_t) override {
        return 0;
    }
};

class UiObjectVisibility final {
public:
    static void SetWindowVisibility(UiObjectState& state, bool visible,
                                    IUiObjectVisibilityRuntime& runtime);
    [[nodiscard]] static int SetVisibility(
        UiObjectState& state, std::uint32_t object_address,
        std::uint8_t showing, std::uint32_t parent,
        IUiObjectVisibilityRuntime& runtime);
};

}  // namespace pal4
