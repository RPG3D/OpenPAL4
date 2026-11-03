#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/picture_preview_window_lifecycle.h"

namespace pal4 {

class IPicturePreviewWindowElementsRuntime {
public:
    virtual ~IPicturePreviewWindowElementsRuntime() = default;
    [[nodiscard]] virtual std::string ReadPrefix(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::string_view name) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(std::size_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::string_view event,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class PicturePreviewWindowElementsRuntimeStub :
    public IPicturePreviewWindowElementsRuntime {
public:
    [[nodiscard]] std::string ReadPrefix(std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(std::string_view) override {
        return 0u;
    }
    void SetAlwaysOnTop(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t AllocateCallback(std::size_t) override {
        return 0u;
    }
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Subscribe(std::uint32_t, std::string_view,
                                          std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(std::uint32_t) override {
        return 0u;
    }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    void SetVisible(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
};

class PicturePreviewWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843778u;
    static constexpr std::uint32_t kMouseMoveHandler = 0x44c4b0u;
    static constexpr std::uint32_t kMouseClickHandler = 0x44c4d0u;
    static constexpr std::uint32_t kCloseHandler = 0x44a150u;

    static void Initialize(PicturePreviewWindowState& state,
                           std::uint32_t object_address,
                           IPicturePreviewWindowElementsRuntime& runtime);
};

}  // namespace pal4
