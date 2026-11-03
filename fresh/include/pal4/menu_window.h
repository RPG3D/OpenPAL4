#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMenuWindowRuntime {
public:
    virtual ~IMenuWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t window,
                            std::string_view name) = 0;
    virtual void ConstructNavigation(std::uint32_t navigation) = 0;
    [[nodiscard]] virtual std::uint8_t InitialStackByte() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCloseCallback(
        std::uint32_t storage, std::uint32_t function,
        std::uint32_t argument, std::uint32_t owner) = 0;
    virtual void RegisterCloseEvent(std::string_view name,
                                    std::uint32_t callback) = 0;
    virtual void ReleaseCloseCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::string_view ReadPrefix(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void Subscribe(std::uint32_t window,
                           std::string_view event_name,
                           std::uint32_t callback) = 0;
    virtual void ReleaseSubscriptionCallback(
        std::uint32_t callback) = 0;
    virtual void DestroyString(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t window) = 0;
};

class MenuWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x844254u;
    static constexpr std::uint32_t kNavigationVtable = 0x84426cu;
    static constexpr std::uint32_t kEventCallbackVtable = 0x84424cu;

    // IDA 0x4b3af0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, IMenuWindowRuntime& runtime);
    // IDA 0x4b3e50.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, IMenuWindowRuntime& runtime);
};

}  // namespace pal4
