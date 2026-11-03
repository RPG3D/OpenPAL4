#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/picture_view_window_lifecycle.h"

namespace pal4 {

class IPictureViewWindowElementsRuntime {
public:
    virtual ~IPictureViewWindowElementsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetImageFormatting(std::uint32_t image,
                                    std::uint32_t horizontal,
                                    std::uint32_t vertical) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    virtual void WriteCallbackWord(std::uint32_t address,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    virtual void BindMouseEvents(std::uint32_t member,
                                 std::uint32_t window) = 0;
};

class PictureViewWindowElementsRuntimeStub :
    public IPictureViewWindowElementsRuntime {
public:
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    void SetImageFormatting(std::uint32_t, std::uint32_t,
                            std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    void BindMouseEvents(std::uint32_t, std::uint32_t) override {}
};

class PictureViewWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843708u;

    static void Initialize(PictureViewWindowState& state,
                           std::uint32_t object_address,
                           IPictureViewWindowElementsRuntime& runtime);
};

}  // namespace pal4
