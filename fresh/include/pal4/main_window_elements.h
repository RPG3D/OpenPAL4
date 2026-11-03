#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/main_window_lifecycle.h"

namespace pal4 {

class IMainWindowElementsRuntime {
public:
    virtual ~IMainWindowElementsRuntime() = default;
    virtual void InitializeNameBuffer() = 0;
    [[nodiscard]] virtual std::string_view LayoutPrefix(
        const MainWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(std::size_t bytes) = 0;
    virtual void ConstructCallback(std::uint32_t callback,
                                   std::uint32_t handler,
                                   std::uint32_t owner) = 0;
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
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class MainWindowElementsRuntimeStub : public IMainWindowElementsRuntime {
public:
    void InitializeNameBuffer() override {}
    [[nodiscard]] std::string_view LayoutPrefix(
        const MainWindowState&) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(std::string_view) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t AllocateCallback(std::size_t) override {
        return 0u;
    }
    void ConstructCallback(std::uint32_t, std::uint32_t,
                           std::uint32_t) override {}
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CleanupNameBuffer() override {
        return 0u;
    }
};

class MainWindowElements final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843650u;

    [[nodiscard]] static std::uint32_t Initialize(
        MainWindowState& state, std::uint32_t object_address,
        IMainWindowElementsRuntime& runtime);
};

}  // namespace pal4
