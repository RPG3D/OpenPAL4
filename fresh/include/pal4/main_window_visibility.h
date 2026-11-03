#pragma once

#include "pal4/main_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IMainWindowVisibilityRuntime {
public:
    virtual ~IMainWindowVisibilityRuntime() = default;
    virtual void SetParent(MainWindowState& state,
                           std::uint32_t showing,
                           std::uint32_t parent) = 0;
    virtual void RegisterMouse(std::uint32_t member,
                               std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::int32_t InsertActive(
        std::uint32_t collection, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::int32_t RemoveActive(
        std::uint32_t collection, std::uint32_t member) = 0;
};

class MainWindowVisibilityRuntimeStub :
    public IMainWindowVisibilityRuntime {
public:
    void SetParent(MainWindowState&, std::uint32_t,
                   std::uint32_t) override {}
    void RegisterMouse(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GlobalData() override { return 0u; }
    [[nodiscard]] std::int32_t ReadSignedWord(
        std::uint32_t) override { return 0; }
    void SetVisible(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::int32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0; }
};

class MainWindowVisibility final {
public:
    [[nodiscard]] static std::int32_t SetVisibility(
        MainWindowState& state, std::uint32_t object_address,
        std::uint32_t showing, std::uint32_t parent,
        IMainWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
