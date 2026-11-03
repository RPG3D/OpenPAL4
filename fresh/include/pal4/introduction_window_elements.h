#pragma once

#include "pal4/introduction_window_lifecycle.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

class IIntroductionWindowElementsRuntime {
public:
    virtual ~IIntroductionWindowElementsRuntime() = default;
    virtual void InitializeTemporary(std::uint8_t initial_byte,
                                     std::uint8_t error_flag) = 0;
    [[nodiscard]] virtual std::string_view LayoutPrefix(
        const IntroductionWindowState& state) = 0;
    [[nodiscard]] virtual bool AllocateFinalName(
        std::size_t total_length, std::uint8_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t handler,
        std::uint32_t zero, std::uint32_t owner) = 0;
    virtual void InitializeManualCallback(
        std::uint32_t storage, std::uint32_t vtable,
        std::uint32_t handler, std::uint32_t zero,
        std::uint32_t owner) = 0;
    virtual void FreeCallbackStorage(std::uint32_t storage) = 0;
    virtual void SetWindowUserId(std::uint32_t window,
                                 std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeClicked(
        std::uint32_t window, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteCallback(std::uint32_t reference,
                                std::uint32_t flags) = 0;
    virtual void SetupMouseHandlers(std::uint32_t member_address,
                                    std::uint32_t window) = 0;
    [[nodiscard]] virtual std::uint32_t TemporaryBufferAddress() = 0;
    [[nodiscard]] virtual std::uint8_t ReadPrefixByte(
        std::uint32_t address) = 0;
    virtual void WritePrefixByte(std::uint32_t address,
                                 std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FreeTemporary(
        std::uint32_t address) = 0;
};

class IntroductionWindowElementsRuntimeStub :
    public IIntroductionWindowElementsRuntime {
public:
    void InitializeTemporary(std::uint8_t, std::uint8_t) override {}
    [[nodiscard]] std::string_view LayoutPrefix(
        const IntroductionWindowState&) override { return {}; }
    [[nodiscard]] bool AllocateFinalName(
        std::size_t, std::uint8_t) override { return true; }
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ConstructCallback(
        std::uint32_t storage, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return storage; }
    void InitializeManualCallback(std::uint32_t, std::uint32_t,
                                  std::uint32_t, std::uint32_t,
                                  std::uint32_t) override {}
    void FreeCallbackStorage(std::uint32_t) override {}
    void SetWindowUserId(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    void SetupMouseHandlers(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t TemporaryBufferAddress() override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t ReadPrefixByte(
        std::uint32_t) override { return 0u; }
    void WritePrefixByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t FreeTemporary(
        std::uint32_t) override { return 0u; }
};

class IntroductionWindowElements final {
public:
    static constexpr std::uint32_t kManualCallbackVtable = 0x84356cu;
    static constexpr std::uint32_t kCloseHandler = 0x44a150u;
    static constexpr std::uint32_t kMainHandler = 0x447120u;
    static constexpr std::uint32_t kSubHandler = 0x447140u;

    [[nodiscard]] static std::uint32_t Initialize(
        IntroductionWindowState& state, std::uint32_t object_address,
        std::uint8_t temporary_byte,
        IIntroductionWindowElementsRuntime& runtime);
};

}  // namespace pal4
