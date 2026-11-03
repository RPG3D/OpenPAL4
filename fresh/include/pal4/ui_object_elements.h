#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/ui_object_lifecycle.h"

namespace pal4 {

class IUiObjectElementsRuntime {
public:
    virtual ~IUiObjectElementsRuntime() = default;
    virtual void InitializeTemporary(std::uint8_t byte,
                                     std::uint8_t error_flag) = 0;
    [[nodiscard]] virtual std::string_view LayoutPrefix(
        const UiObjectState& state) = 0;
    [[nodiscard]] virtual bool AllocateWebsiteName(
        std::size_t total_length, std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCloseCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t zero, std::uint32_t owner) = 0;
    virtual void FreeCallbackStorage(std::uint32_t storage) = 0;
    virtual void InitializeButtonCallback(
        std::uint32_t storage, std::uint32_t vtable,
        std::uint32_t handler, std::uint32_t zero,
        std::uint32_t owner) = 0;
    virtual void SetWindowUserId(std::uint32_t window,
                                 std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) = 0;
    virtual void ReleaseCloseReference(std::uint32_t reference) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteCallback(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    virtual void SetupMouseHandlers(UiObjectState& state,
                                    std::uint32_t member_offset,
                                    std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryBufferAddress() = 0;
    [[nodiscard]] virtual std::uint8_t ReadPrefixByte(
        std::uint32_t address) = 0;
    virtual void WritePrefixByte(std::uint32_t address,
                                 std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeTemporary(
        std::uint32_t address) = 0;
};

class UiObjectElementsRuntimeStub : public IUiObjectElementsRuntime {
public:
    void InitializeTemporary(std::uint8_t, std::uint8_t) override {}
    [[nodiscard]] std::string_view LayoutPrefix(
        const UiObjectState&) override { return {}; }
    [[nodiscard]] bool AllocateWebsiteName(std::size_t,
                                            std::uint8_t) override {
        return true;
    }
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructCloseCallback(
        std::uint32_t storage, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return storage; }
    void FreeCallbackStorage(std::uint32_t) override {}
    void InitializeButtonCallback(std::uint32_t, std::uint32_t,
                                   std::uint32_t, std::uint32_t,
                                   std::uint32_t) override {}
    void SetWindowUserId(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void ReleaseCloseReference(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    void SetupMouseHandlers(UiObjectState&, std::uint32_t,
                            std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TemporaryBufferAddress() override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t ReadPrefixByte(
        std::uint32_t) override { return 0u; }
    void WritePrefixByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t FreeTemporary(
        std::uint32_t) override { return 0u; }
};

class UiObjectElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x8434f8u;
    static constexpr std::uint32_t kCloseHandler = 0x44a150u;
    static constexpr std::uint32_t kButtonHandler = 0x445cd0u;

    [[nodiscard]] static std::uint32_t Initialize(
        UiObjectState& state, std::uint32_t object_address,
        std::uint8_t temporary_byte,
        IUiObjectElementsRuntime& runtime);
};

}  // namespace pal4
