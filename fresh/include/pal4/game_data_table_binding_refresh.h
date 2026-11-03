#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataTableBindingRefreshRuntime {
public:
    virtual ~IGameDataTableBindingRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void LogBind(std::string_view prefix,
                         std::uint32_t name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void ProcessPropertyRegistry(std::uint32_t binding) = 0;
    virtual void BindEffects(std::uint32_t binding) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLogBuffer() = 0;
};

}  // namespace pal4
