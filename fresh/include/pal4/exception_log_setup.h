#pragma once

#include "pal4/exception_log.h"

#include <cstdint>

namespace pal4 {

class IExceptionLogSetupRuntime {
public:
    virtual ~IExceptionLogSetupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetModuleHandle(const char* name) = 0;
    [[nodiscard]] virtual bool AddModule(std::uint32_t module) = 0;
    [[nodiscard]] virtual std::uint32_t SetSeTranslator(std::uint32_t handler) = 0;
};

class ExceptionLogSetup final {
public:
    static constexpr std::uint32_t kGlobalHandlerAddress = 0x404a50u;
    static constexpr std::uint32_t kHandlerSlotAddress = 0x9502a0u;

    [[nodiscard]] static std::uint32_t Install(
        const char* path, ExceptionLog& log, IExceptionLogRuntime& log_runtime,
        IExceptionLogSetupRuntime& runtime);
};

}  // namespace pal4
