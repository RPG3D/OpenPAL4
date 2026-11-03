#pragma once

#include "pal4/game_render_system.h"
#include "pal4/video_device_selector.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace pal4 {

class WindowMessageConfig final {
public:
    // WinMain places g_GameState at least 156 bytes after g_GameConfig.
    static constexpr std::size_t kWordCount = 39u;
    static constexpr std::size_t kBytes = kWordCount * sizeof(std::uint32_t);

    class Word final {
    public:
        Word(WindowMessageConfig& owner, std::size_t index)
            : owner_(owner), index_(index) {}
        Word& operator=(std::uint32_t value) {
            owner_.Write(index_, value);
            return *this;
        }
        Word& operator=(const Word& other) {
            return *this = static_cast<std::uint32_t>(other);
        }
        [[nodiscard]] operator std::uint32_t() const {
            return owner_.Read(index_);
        }

    private:
        WindowMessageConfig& owner_;
        std::size_t index_;
    };

    [[nodiscard]] bool BindExternal(std::uint32_t address,
                                    std::span<std::byte> storage) {
        std::lock_guard lock(mutex_);
        if (external_ != nullptr || storage.size() < kBytes) return false;
        std::memcpy(storage.data(), words_.data(), kBytes);
        external_ = storage.data();
        address_ = address;
        return true;
    }
    void UnbindExternal() {
        std::lock_guard lock(mutex_);
        if (external_ == nullptr) return;
        std::memcpy(words_.data(), external_, kBytes);
        external_ = nullptr;
        address_ = 0u;
    }
    [[nodiscard]] std::uint32_t Address() const {
        std::lock_guard lock(mutex_);
        return address_;
    }
    [[nodiscard]] constexpr std::size_t size() const { return kWordCount; }
    [[nodiscard]] Word operator[](std::size_t index) { return {*this, index}; }
    [[nodiscard]] std::uint32_t operator[](std::size_t index) const {
        return Read(index);
    }
    [[nodiscard]] const std::uint32_t* data() const {
        std::lock_guard lock(mutex_);
        if (external_ != nullptr)
            std::memcpy(words_.data(), external_, kBytes);
        return words_.data();
    }

private:
    [[nodiscard]] std::uint32_t Read(std::size_t index) const {
        std::lock_guard lock(mutex_);
        std::uint32_t value = words_.at(index);
        if (external_ != nullptr)
            std::memcpy(&value, external_ + index * sizeof(value), sizeof(value));
        return value;
    }
    void Write(std::size_t index, std::uint32_t value) {
        std::lock_guard lock(mutex_);
        words_.at(index) = value;
        if (external_ != nullptr)
            std::memcpy(external_ + index * sizeof(value), &value, sizeof(value));
    }

    mutable std::mutex mutex_;
    mutable std::array<std::uint32_t, kWordCount> words_{};
    std::byte* external_ = nullptr;
    std::uint32_t address_ = 0u;
};
using KeyboardInputArgument =
    std::variant<std::uint32_t, const std::uint32_t*>;

class IWindowMessageGameServices {
public:
    virtual ~IWindowMessageGameServices() = default;
    virtual void SetInputMode(std::uint32_t keyboard, std::uint32_t mouse,
                              std::uint32_t flags) = 0;
    virtual void InitializeInputState() = 0;
    [[nodiscard]] virtual std::pair<bool, bool> InputDevicesReady() = 0;
    virtual void SetAudioActive(bool active) = 0;
    virtual void HandleKeyboardInput(std::uint32_t type,
                                     KeyboardInputArgument argument) = 0;
    virtual void HandleMouseInput(std::uint32_t type,
                                  const std::uint32_t* data) = 0;
    virtual void SetUICursorFromTable(std::uint32_t table_address) = 0;
    [[nodiscard]] virtual bool CurrentAudioActive() = 0;
    virtual void ReplayCurrentSound() = 0;
    [[nodiscard]] virtual std::string ComposeMusicBasePath() = 0;
    [[nodiscard]] virtual bool MusicEnabled() = 0;
    virtual void SetMusicEnabled(bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t PlayMusic(std::string_view path,
                                                   std::uint32_t flags) = 0;
    virtual void StoreMusicHandle(std::uint32_t handle) = 0;
    virtual void GetRenderSystem() = 0;
    [[nodiscard]] virtual std::uint32_t GetActiveCamera() = 0;
    virtual void ResizeRenderScreen(std::int32_t width,
                                    std::int32_t height,
                                    std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t GetFilesystemCallback() = 0;
    [[nodiscard]] virtual std::uint32_t CreateUncdFilesystemCallback(
        std::uint32_t id, std::string_view label) = 0;
    virtual void RegisterFilesystemCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FindFilesystemCallback(
        std::string_view label) = 0;
    virtual void SetFilesystemCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual bool ConsoleActive() = 0;
    virtual void HandleUIMessage(std::uint32_t window, std::uint32_t message,
                                 std::uint32_t wparam,
                                 std::uint32_t lparam) = 0;
};

class IWindowMessageRuntime : public IWindowMessageGameServices {
public:
    ~IWindowMessageRuntime() override = default;
    virtual void AcceptDroppedFiles(std::uint32_t window) = 0;
    virtual void EnumCurrentDisplayMode(DisplayModeRecord& mode) = 0;
    virtual void ChangeDisplayMode(const DisplayModeRecord& mode,
                                   std::uint32_t flags) = 0;
    virtual void ReleaseCursorClip() = 0;
    virtual void ClipCursorToWindow(std::uint32_t window) = 0;
    virtual void ShowWindow(std::uint32_t window, std::uint32_t command) = 0;
    virtual void SetForegroundWindow(std::uint32_t window) = 0;
    virtual void SetFocus(std::uint32_t window) = 0;
    virtual void Sleep(std::uint32_t milliseconds) = 0;
    [[nodiscard]] virtual std::uint32_t TimeGetTime() = 0;
    virtual void SetCapture(std::uint32_t window) = 0;
    virtual void ReleaseCapture() = 0;
    [[nodiscard]] virtual ClientRect ReadSizingRect(
        std::uint32_t address) = 0;
    virtual void SetWindowPos(std::uint32_t window, std::int32_t width,
                              std::int32_t height, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t DroppedFileCount(
        std::uint32_t drop_handle) = 0;
    [[nodiscard]] virtual std::string DroppedFile(
        std::uint32_t drop_handle, std::uint32_t index) = 0;
    virtual void FinishDrop(std::uint32_t drop_handle) = 0;
    [[nodiscard]] virtual std::uint32_t DefWindowProc(
        std::uint32_t window, std::uint32_t message,
        std::uint32_t wparam, std::uint32_t lparam) = 0;
};

class WindowMessageProcessor final {
public:
    WindowMessageProcessor(GameRenderState& game, WindowMessageConfig& config,
                           DisplayModeRecord& prior_mode,
                           std::uint32_t& resize_marker)
        : game_(game), config_(config), prior_mode_(prior_mode),
          resize_marker_(resize_marker) {}

    [[nodiscard]] static std::uint8_t DecodeKeyboardScanCode(
        std::uint32_t ignored, std::uint32_t key_lparam);
    [[nodiscard]] static std::uint32_t MapScanCode(std::uint8_t scan_code);
    [[nodiscard]] std::uint32_t Process(
        std::uint32_t window, std::uint32_t message,
        std::uint32_t wparam, std::uint32_t lparam,
        IWindowMessageRuntime& runtime);
    [[nodiscard]] std::uint32_t HandledFlag() const { return handled_flag_; }
    // Secondary-display integration runs must survive focus changes.
    void KeepRunningWhenInactiveForTests() noexcept {
        keep_running_when_inactive_ = true;
    }

private:
    void HandleActivation(std::uint32_t window, std::uint32_t wparam,
                          IWindowMessageRuntime& runtime);
    void HandleKey(std::uint32_t message, std::uint32_t wparam,
                   std::uint32_t lparam, IWindowMessageRuntime& runtime);
    void HandleMouse(std::uint32_t window, std::uint32_t message,
                     std::uint32_t wparam, std::uint32_t lparam,
                     IWindowMessageRuntime& runtime);
    void HandleDrop(std::uint32_t drop_handle,
                    IWindowMessageRuntime& runtime);
    void ToggleMusic(IWindowMessageRuntime& runtime);
    void SetMouseSnapshot(std::uint32_t lparam);
    void UpdateModifier(std::uint32_t code, std::uint32_t value);

    GameRenderState& game_;
    WindowMessageConfig& config_;
    DisplayModeRecord& prior_mode_;
    std::uint32_t& resize_marker_;
    std::uint32_t handled_flag_ = 0;
    bool keep_running_when_inactive_ = false;
};

}  // namespace pal4
