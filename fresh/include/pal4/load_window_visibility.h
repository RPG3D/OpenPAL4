#pragma once

#include "pal4/load_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class ILoadWindowVisibilityRuntime {
public:
    virtual ~ILoadWindowVisibilityRuntime() = default;
    virtual void SetParent(LoadWindowState& state,
                           std::uint32_t showing,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void PopulateSaveList(LoadWindowState& state,
                                  std::int32_t page,
                                  std::uint32_t refresh) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::int32_t InsertActive(
        std::uint32_t collection, std::uint32_t member) = 0;
    [[nodiscard]] virtual std::int32_t RemoveActive(
        std::uint32_t collection, std::uint32_t member) = 0;
};

class LoadWindowVisibilityRuntimeStub :
    public ILoadWindowVisibilityRuntime {
public:
    void SetParent(LoadWindowState&, std::uint32_t,
                   std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GlobalData() override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void PopulateSaveList(LoadWindowState&, std::int32_t,
                          std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    [[nodiscard]] std::int32_t InsertActive(
        std::uint32_t, std::uint32_t) override { return 0; }
    [[nodiscard]] std::int32_t RemoveActive(
        std::uint32_t, std::uint32_t) override { return 0; }
};

class LoadWindowVisibility final {
public:
    [[nodiscard]] static std::int32_t SetVisibility(
        LoadWindowState& state, std::uint32_t object_address,
        std::uint32_t showing, std::uint32_t parent,
        ILoadWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
