#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MainWindowMemberState {
    std::array<std::uint8_t, 0x41> bytes{};
};

class IMainWindowMemberEventRuntime {
public:
    virtual ~IMainWindowMemberEventRuntime() = default;
    [[nodiscard]] virtual std::uint8_t HandleKeyEvent(
        MainWindowMemberState& state, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint8_t HandleSelection(
        MainWindowMemberState& state) = 0;
};

class MainWindowMemberEventRuntimeStub : public IMainWindowMemberEventRuntime {
public:
    [[nodiscard]] std::uint8_t HandleKeyEvent(
        MainWindowMemberState&, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint8_t HandleSelection(
        MainWindowMemberState&) override { return 0u; }
};

class MainWindowMemberEvents final {
public:
    [[nodiscard]] static std::uint8_t ForwardKey(
        MainWindowMemberState& state, std::uint32_t event,
        IMainWindowMemberEventRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetSelection(
        MainWindowMemberState& state,
        IMainWindowMemberEventRuntime& runtime);
};

}  // namespace pal4
