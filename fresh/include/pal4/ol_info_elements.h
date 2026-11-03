#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "pal4/ol_info_lifecycle.h"

namespace pal4 {

class IOlInfoElementsRuntime {
public:
    virtual ~IOlInfoElementsRuntime() = default;
    virtual void InitializeNameBuffer() = 0;
    [[nodiscard]] virtual std::string_view LayoutPrefix(
        const OlInfoState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
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
    [[nodiscard]] virtual std::uint32_t CleanupNameBuffer() = 0;
};

class OlInfoElementsRuntimeStub : public IOlInfoElementsRuntime {
public:
    void InitializeNameBuffer() override {}
    [[nodiscard]] std::string_view LayoutPrefix(
        const OlInfoState&) override { return {}; }
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t AllocateCallback(
        std::size_t) override { return 0u; }
    void WriteCallbackWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SubscribeClicked(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CleanupNameBuffer() override { return 0u; }
};

class OlInfoElements final {
public:
    static constexpr std::uint32_t kCloseCallback = 0x44a150u;
    static constexpr std::uint32_t kCallbackVtable = 0x8436c0u;

    [[nodiscard]] static std::uint32_t Initialize(
        OlInfoState& state, std::uint32_t object_address,
        IOlInfoElementsRuntime& runtime);
};

}  // namespace pal4
