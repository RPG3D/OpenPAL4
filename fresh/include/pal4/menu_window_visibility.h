#pragma once

#include <cstdint>

namespace pal4 {

struct MenuWindowPoint {
    float x;
    float y;
};

class IMenuWindowVisibilityRuntime {
public:
    virtual ~IMenuWindowVisibilityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void AdjustHeight(std::uint32_t menu) = 0;
    virtual void SetParent(std::uint32_t menu,
                           std::uint32_t first,
                           std::uint32_t second) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window,
                                bool on_top) = 0;
    [[nodiscard]] virtual MenuWindowPoint ScreenToWindow(
        std::uint32_t root, MenuWindowPoint screen) = 0;
    virtual void SetPosition(std::uint32_t window,
                             MenuWindowPoint point) = 0;
    [[nodiscard]] virtual bool IsDisabled(std::uint32_t window,
                                          bool inherited) = 0;
    virtual void SelectNavigation(std::uint32_t navigation,
                                  std::uint32_t virtual_slot) = 0;
    virtual void SetModalState(std::uint32_t root, bool modal) = 0;
    virtual void CleanupButtonRecord(std::uint32_t record) = 0;
    virtual void RemoveChild(std::uint32_t parent,
                             std::uint32_t child) = 0;
    virtual void DestroyWindow(std::uint32_t window) = 0;
    virtual void AddChild(std::uint32_t parent,
                          std::uint32_t child) = 0;
};

class MenuWindowVisibility final {
public:
    // IDA 0x4b49b0.
    static void Show(std::uint32_t menu, MenuWindowPoint screen,
                     IMenuWindowVisibilityRuntime& runtime);
    // IDA 0x4b4a50.
    static void HideAndCleanup(
        std::uint32_t menu, IMenuWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
