#pragma once

#include <cstdint>

#include "pal4/picture_view_window_lifecycle.h"

namespace pal4 {

class IPictureViewWindowVisibilityRuntime {
public:
    virtual ~IPictureViewWindowVisibilityRuntime() = default;
    virtual void SetParent(PictureViewWindowState& state,
                           std::uint8_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t InsertActive(
        std::uint32_t array, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveActive(
        std::uint32_t array, std::uint32_t member) = 0;
};

class PictureViewWindowVisibilityRuntimeStub :
    public IPictureViewWindowVisibilityRuntime {
public:
    void SetParent(PictureViewWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::uint32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class PictureViewWindowVisibility final {
public:
    [[nodiscard]] static std::uint32_t SetVisible(
        PictureViewWindowState& state,
        std::uint32_t object_address, std::uint8_t showing,
        std::uint32_t parent,
        IPictureViewWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
