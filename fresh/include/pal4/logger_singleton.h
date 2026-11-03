#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct LoggerSingletonState final {
    std::uint8_t guard = 0;
    std::uint32_t instance = 0;
};

class ILoggerSingletonRuntime {
public:
    virtual ~ILoggerSingletonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view type_name,
        std::uint32_t singleton_id, std::uint32_t factory_address,
        std::uint32_t type_info_address) = 0;
};

class LoggerSingletonAccess final {
public:
    static constexpr std::uint32_t kFactoryAddress = 0x402190;
    static constexpr std::uint32_t kTypeInfoAddress = 0x4021f0;

    [[nodiscard]] static std::uint32_t Get(
        LoggerSingletonState& state, ILoggerSingletonRuntime& runtime);
};

}  // namespace pal4
