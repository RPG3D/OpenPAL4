#pragma once

#include <cstdint>

namespace pal4 {

class IGameDatabaseLoggingRuntime {
public:
    virtual ~IGameDatabaseLoggingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Prepare(
        const char* format, const char* file, std::uint32_t line,
        std::uint32_t message) = 0;
    [[nodiscard]] virtual std::uint32_t Append(
        std::uint32_t prepared) = 0;
};

class GameDatabaseLogging final {
public:
    // IDA 0x50cd40 and 0x50cd70.
    [[nodiscard]] static std::uint32_t Database(
        std::uint32_t message, IGameDatabaseLoggingRuntime& runtime);
    [[nodiscard]] static std::uint32_t Assertion(
        std::uint32_t message, IGameDatabaseLoggingRuntime& runtime);
};

}  // namespace pal4
