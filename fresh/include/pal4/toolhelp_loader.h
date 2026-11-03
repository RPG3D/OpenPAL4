#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ToolHelpState {
    std::uint32_t initialized = 0u;
    std::array<std::uint32_t, 7> functions{};
};

class IToolHelpRuntime {
public:
    virtual ~IToolHelpRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetModuleHandle(const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t GetProcAddress(
        std::uint32_t module, const char* name) = 0;
    virtual void SetLastErrorEx(std::uint32_t error, std::uint32_t type) = 0;
};

class ToolHelpLoader final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e08a4u;
    static constexpr std::array<std::uint32_t, 7> kFunctionAddresses{
        0x8e08a8u, 0x8e08acu, 0x8e08b0u, 0x8e08b4u,
        0x8e08b8u, 0x8e08bcu, 0x8e08c0u};

    explicit ToolHelpLoader(ToolHelpState& state) : state_(state) {}
    [[nodiscard]] std::uint32_t Initialize(IToolHelpRuntime& runtime);

private:
    ToolHelpState& state_;
};

}  // namespace pal4
