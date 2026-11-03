#pragma once

#include <cstdint>

namespace pal4 {

class IPalTestWindowMemberEventRuntime {
public:
    virtual ~IPalTestWindowMemberEventRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t HandleKeyEvent(
        std::uint32_t member, std::uint32_t event) = 0;
    virtual void CloseWithDefaultEventArgs(std::uint32_t owner) = 0;
};

class PalTestWindowMemberEventRuntimeStub :
    public IPalTestWindowMemberEventRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint8_t HandleKeyEvent(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void CloseWithDefaultEventArgs(std::uint32_t) override {}
};

class PalTestWindowMemberEvents final {
public:
    [[nodiscard]] static std::uint8_t OnEvent(
        std::uint32_t member, std::uint32_t event,
        IPalTestWindowMemberEventRuntime& runtime);
};

}  // namespace pal4
