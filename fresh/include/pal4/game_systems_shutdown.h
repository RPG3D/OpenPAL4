#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameShutdownLogger {
public:
    virtual ~IGameShutdownLogger() = default;
    [[nodiscard]] virtual std::uint32_t InitializeLogger() = 0;
    [[nodiscard]] virtual std::int32_t AppendLog(
        std::uint32_t logger, std::string_view format,
        std::string_view source_file, std::uint32_t source_line) = 0;
};

class IGameSystemsShutdownRuntime : public IGameShutdownLogger {
public:
    ~IGameSystemsShutdownRuntime() override = default;
    virtual void ShutdownRenderSystem() = 0;
};

class GameSystemsShutdown final {
public:
    [[nodiscard]] std::int32_t Shutdown(
        IGameSystemsShutdownRuntime& runtime) const;
};

}  // namespace pal4
