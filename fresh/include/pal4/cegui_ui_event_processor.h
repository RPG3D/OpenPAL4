#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICeguiUiEventRuntime {
public:
    virtual ~ICeguiUiEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t MapVirtualKey(
        std::uint32_t owner, std::uint32_t virtual_key) = 0;
    [[nodiscard]] virtual std::uint32_t MapImeCharacter(
        std::uint32_t wparam) = 0;
    [[nodiscard]] virtual std::uint32_t GetSystemSingleton() = 0;
    [[nodiscard]] virtual std::uint32_t GetSystemSingletonPtr() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool InjectKeyDown(
        std::uint32_t system, std::uint32_t key) = 0;
    [[nodiscard]] virtual bool InjectKeyUp(
        std::uint32_t system, std::uint32_t key) = 0;
    [[nodiscard]] virtual bool InjectChar(
        std::uint32_t system, std::uint32_t character) = 0;
    virtual void EnableMouseCapture(std::uint32_t owner) = 0;
    virtual void DisableMouseCapture(std::uint32_t owner) = 0;
    [[nodiscard]] virtual std::array<float, 2> TransformMouseCoordinates(
        std::uint32_t owner, const std::array<float, 2>& point) = 0;
    [[nodiscard]] virtual bool InjectMousePosition(
        std::uint32_t system, float x, float y) = 0;
    [[nodiscard]] virtual bool InjectMouseButtonDown(
        std::uint32_t system, std::uint32_t button) = 0;
    [[nodiscard]] virtual bool InjectMouseButtonUp(
        std::uint32_t system, std::uint32_t button) = 0;
    [[nodiscard]] virtual bool InjectMouseWheel(
        std::uint32_t system, float delta) = 0;
    virtual void RendererReadDisplaySize(std::uint32_t renderer) = 0;
    virtual void RendererFireDisplaySizeChanged(
        std::uint32_t renderer) = 0;
    virtual void RequestRedraw(std::uint32_t window) = 0;
};

class CeguiUiEventRuntimeStub final : public ICeguiUiEventRuntime {
public:
    [[nodiscard]] std::uint32_t MapVirtualKey(
        std::uint32_t owner, std::uint32_t virtual_key) override;
    [[nodiscard]] std::uint32_t MapImeCharacter(
        std::uint32_t wparam) override;
    [[nodiscard]] std::uint32_t GetSystemSingleton() override;
    [[nodiscard]] std::uint32_t GetSystemSingletonPtr() override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    [[nodiscard]] bool InjectKeyDown(
        std::uint32_t system, std::uint32_t key) override;
    [[nodiscard]] bool InjectKeyUp(
        std::uint32_t system, std::uint32_t key) override;
    [[nodiscard]] bool InjectChar(
        std::uint32_t system, std::uint32_t character) override;
    void EnableMouseCapture(std::uint32_t owner) override;
    void DisableMouseCapture(std::uint32_t owner) override;
    [[nodiscard]] std::array<float, 2> TransformMouseCoordinates(
        std::uint32_t owner,
        const std::array<float, 2>& point) override;
    [[nodiscard]] bool InjectMousePosition(
        std::uint32_t system, float x, float y) override;
    [[nodiscard]] bool InjectMouseButtonDown(
        std::uint32_t system, std::uint32_t button) override;
    [[nodiscard]] bool InjectMouseButtonUp(
        std::uint32_t system, std::uint32_t button) override;
    [[nodiscard]] bool InjectMouseWheel(
        std::uint32_t system, float delta) override;
    void RendererReadDisplaySize(std::uint32_t renderer) override;
    void RendererFireDisplaySizeChanged(
        std::uint32_t renderer) override;
    void RequestRedraw(std::uint32_t window) override;
};

class CeguiUiEventProcessor final {
public:
    // Keep native host forwarding aligned with the mouse branches below.
    [[nodiscard]] static constexpr bool IsMouseMessage(std::uint32_t message) noexcept {
        return message == 0x200u || message == 0x201u || message == 0x202u ||
               message == 0x204u || message == 0x205u || message == 0x207u ||
               message == 0x208u || message == 0x20au;
    }
    [[nodiscard]] static bool Process(
        std::uint32_t owner, std::uint32_t hwnd,
        std::uint32_t message, std::uint32_t wparam,
        std::uint32_t lparam, ICeguiUiEventRuntime& runtime);
};

}  // namespace pal4
