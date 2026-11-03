#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMainWindowUiRuntime {
public:
    virtual ~IMainWindowUiRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t WindowNameAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ProcessEventsAddress() = 0;
    [[nodiscard]] virtual std::uint32_t CloseMainAddress() = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
};

class MainWindowUiRuntimeStub : public IMainWindowUiRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t WindowNameAddress() override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ProcessEventsAddress() override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t CloseMainAddress() override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
};

class MainWindowUiRegistration final {
public:
    static constexpr std::uint32_t kLogoResultAddress = 0x8e43c2u;
    static constexpr std::uint32_t kMainResultAddress = 0x8e43c1u;
    static constexpr std::uint32_t kCloseResultAddress = 0x8e43c0u;
    static constexpr std::uint32_t kLogoCallbackVtable = 0x843494u;
    static constexpr std::uint32_t kMainCallbackVtable = 0x8435dcu;

    [[nodiscard]] static std::uint32_t RegisterLogo(
        IMainWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterLogoThunk(
        IMainWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterMain(
        IMainWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterMainThunk(
        IMainWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterClose(
        IMainWindowUiRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterCloseThunk(
        IMainWindowUiRuntime& runtime);
};

}  // namespace pal4
