#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemToolbarFrameRuntime {
public:
    virtual ~ISystemToolbarFrameRuntime() = default;
    virtual void LoadLayout(std::uint32_t frame,
                            std::uint32_t name) = 0;
    virtual void ConstructMouseHelper(std::uint32_t helper) = 0;
    virtual void DestroyMouseHelper(std::uint32_t helper) = 0;
    virtual void DestroyBase(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t FindGlobalWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindChildWindow(
        std::uint32_t frame, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
    virtual void SetZOrderingEnabled(std::uint32_t window,
                                     bool enabled) = 0;
    virtual void SetupMouseHandlers(std::uint32_t helper,
                                    std::uint32_t window) = 0;
};

class SystemToolbarFrame final {
public:
    static constexpr std::uint32_t kFrameVtable = 0x843c44u;
    static constexpr std::uint32_t kMouseHelperVtable = 0x843c1cu;
    static constexpr std::uint32_t kCallbackVtable = 0x843c14u;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kOnButtonClick = 0x483090u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        ISystemToolbarFrameRuntime& runtime);
};

}  // namespace pal4
