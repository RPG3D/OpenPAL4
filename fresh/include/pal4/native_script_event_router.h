#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace pal4 {
// Host STL owns subscriptions. Tokens identify host keys/handlers, not original
// EXE addresses. Game registration ordering follows574C30/529650; dispatch
// follows5631D0. Owners must unsubscribe before their borrowed callbacks die.
class NativeScriptEventRouter final {
public:
    using Callback = std::function<void(std::uint32_t)>;
    NativeScriptEventRouter();
    ~NativeScriptEventRouter();
    NativeScriptEventRouter(const NativeScriptEventRouter&) = delete;
    NativeScriptEventRouter& operator=(const NativeScriptEventRouter&) = delete;
    [[nodiscard]] std::uint32_t Key(std::string_view name);
    [[nodiscard]] std::uint32_t Subscribe(std::uint32_t key, std::uint32_t mode, Callback);
    [[nodiscard]] bool Unsubscribe(std::uint32_t handle) noexcept;
    [[nodiscard]] std::uint32_t Dispatch(std::uint32_t key, std::uint32_t payload);
    [[nodiscard]] std::uint32_t Dispatch(std::string_view name, std::uint32_t payload);
    void Pause(bool paused) noexcept;
    [[nodiscard]] bool Paused() const noexcept;
    void Clear() noexcept;
    [[nodiscard]] std::uint32_t SubscriptionCount() const noexcept;
    [[nodiscard]] bool Contains(std::uint32_t handle) const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
