#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/pal_test_window.h"

namespace pal4 {

class IPalTestWindowElementsRuntime {
public:
    virtual ~IPalTestWindowElementsRuntime() = default;
    [[nodiscard]] virtual std::string ReadPrefix(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(std::size_t bytes) = 0;
    virtual void FreeCallback(std::uint32_t callback) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::string_view event,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    virtual void SetAnswerTextColour(std::uint32_t window,
                                     std::uint32_t state,
                                     std::uint32_t argb) = 0;
    virtual void SetupMouseEventHandlers(std::uint32_t member,
                                         std::uint32_t window) = 0;
};

class PalTestWindowElementsRuntimeStub :
    public IPalTestWindowElementsRuntime {
public:
    [[nodiscard]] std::string ReadPrefix(std::uint32_t) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t AllocateCallback(std::size_t) override {
        return 0u;
    }
    void FreeCallback(std::uint32_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Subscribe(
        std::uint32_t, std::string_view, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(std::uint32_t) override {
        return 0u;
    }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    void SetAnswerTextColour(std::uint32_t, std::uint32_t,
                             std::uint32_t) override {}
    void SetupMouseEventHandlers(std::uint32_t,
                                 std::uint32_t) override {}
};

class PalTestWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x84381cu;
    static constexpr std::uint32_t kCloseHandler = 0x44a150u;
    static constexpr std::uint32_t kCategoryHandler = 0x44e460u;
    static constexpr std::uint32_t kOKHandler = 0x44e480u;
    static constexpr std::uint32_t kAnswerHandler = 0x44e4e0u;

    static void Initialize(PalTestWindowState& state,
                           std::uint32_t object_address,
                           IPalTestWindowElementsRuntime& runtime);
};

}  // namespace pal4
