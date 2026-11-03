#pragma once

#include "pal4/renderware_presentation.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>

namespace pal4 {

class GameRenderSystem;

class ITextureDictionaryDestroyRuntime {
public:
    virtual ~ITextureDictionaryDestroyRuntime() = default;
    virtual void DestroyTextureDictionary(std::uint32_t dictionary) = 0;
};

class IRenderWareApi : public ITextureDictionaryDestroyRuntime {
public:
    ~IRenderWareApi() override = default;
    virtual void CloseCharset() = 0;
    virtual void ForEachTextureDictionary(GameRenderSystem& system,
                                          std::uint32_t data) = 0;
    virtual void CloseFileSystemManager() = 0;
    virtual void EngineStop() = 0;
    virtual void EngineClose() = 0;
    virtual void EngineShutdown() = 0;
};

class IGameDisplayModeRuntime {
public:
    virtual ~IGameDisplayModeRuntime() = default;
    [[nodiscard]] virtual std::int32_t ChangeDisplaySettings(
        std::uint32_t mode, std::uint32_t flags) = 0;
};

class IGameRenderPlatform : public IGameDisplayModeRuntime {
public:
    ~IGameRenderPlatform() override = default;
    [[nodiscard]] virtual std::int32_t FloatToLong(float value) = 0;
    virtual void ClientToScreen(std::uint32_t window, std::int32_t& x,
                                std::int32_t& y) = 0;
    [[nodiscard]] virtual std::int32_t SetCursorPos(std::int32_t x,
                                                    std::int32_t y) = 0;
    [[nodiscard]] virtual std::uint32_t EndTimePeriod(
        std::uint32_t period) = 0;
};

class ICameraViewRuntime {
public:
    virtual ~ICameraViewRuntime() = default;
    virtual void AdjustCameraView(std::uint32_t camera,
                                  std::array<std::int32_t, 4>& rectangle,
                                  float view_window,
                                  std::uint32_t mode) = 0;
};

class GameRenderState final {
public:
    static constexpr std::uint32_t kBytes = 0x5a8u;

    explicit GameRenderState(std::uint32_t address)
        : address_(address), model_address_(address) {}
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
        address_ = model_address_;
    }
    [[nodiscard]] std::uint32_t Address() const {
        std::lock_guard lock(mutex_);
        return address_;
    }
    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const {
        std::lock_guard lock(mutex_);
        const auto index = offset / 4u;
        std::uint32_t value = words_.at(index);
        if (external_ != nullptr)
            std::memcpy(&value, external_ + index * 4u, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) {
        std::lock_guard lock(mutex_);
        const auto index = offset / 4u;
        words_.at(index) = value;
        if (external_ != nullptr)
            std::memcpy(external_ + index * 4u, &value, sizeof(value));
    }
    [[nodiscard]] std::uint32_t ReadShared(std::uint32_t offset) {
        return Read(offset);
    }
    void WriteShared(std::uint32_t offset, std::uint32_t value) {
        Write(offset, value);
    }

private:
    mutable std::mutex mutex_;
    std::uint32_t address_;
    std::uint32_t model_address_;
    std::array<std::uint32_t, kBytes / 4u> words_{};
    std::byte* external_ = nullptr;
};

class GameRenderSystem final {
public:
    explicit GameRenderSystem(GameRenderState& state) : state_(state) {}

    [[nodiscard]] std::uint32_t ShowGameRaster(
        std::uint32_t camera, IRenderWarePresentation& renderware) const;
    [[nodiscard]] std::int32_t SetViewportCenter(
        float x, float y, IGameRenderPlatform& platform);
    [[nodiscard]] std::uint32_t DestroyTexture(
        std::uint32_t dictionary,
        ITextureDictionaryDestroyRuntime& renderware) const;
    [[nodiscard]] std::int32_t ShutdownRenderSystem(
        IRenderWareApi& renderware, IGameDisplayModeRuntime& platform);
    [[nodiscard]] std::uint32_t EndTimePeriod(
        IGameRenderPlatform& platform) const;
    static void ResizeRenderScreen(
        std::array<std::int32_t, 4> rectangle, std::uint32_t camera,
        const std::array<std::uint32_t, 12>& config,
        ICameraViewRuntime& runtime);

private:
    GameRenderState& state_;
};

}  // namespace pal4
