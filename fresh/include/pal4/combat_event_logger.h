#pragma once

#include <cstdint>
#include <cstdarg>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatEventLoggerRuntime {
public:
    virtual ~ICombatEventLoggerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t OpaqueStackByte() = 0;
    virtual void InitializeIos(std::uint32_t ios,
                               std::uint32_t buffer,
                               std::uint32_t mode) = 0;
    virtual void ConstructFileBuffer(std::uint32_t buffer,
                                     std::uint32_t mode) = 0;
    virtual void InitializeStreamBuffer(std::uint32_t buffer) = 0;
    virtual void InitializeExtendedBuffer(std::uint32_t buffer,
                                          std::uint32_t first,
                                          std::uint32_t second,
                                          std::uint8_t converted_flags) = 0;
    virtual void LogEvent(std::uint32_t logger, std::uint32_t level,
                          std::uint32_t message,
                          std::uint32_t format) = 0;
    [[nodiscard]] virtual int CloseFile(std::uint32_t handle) = 0;
    virtual void InitializeFileBuffer(std::uint32_t buffer,
                                      std::uint32_t handle,
                                      std::uint32_t mode) = 0;
    virtual void ClearIos(std::uint32_t ios, std::uint32_t state,
                          bool raise) = 0;
    virtual void CleanupBuffer(std::uint32_t buffer) = 0;
    virtual void DestroyStreamBuffer(std::uint32_t buffer) = 0;
    virtual void DestroyIos(std::uint32_t ios) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CloseFileBuffer(std::uint32_t buffer) = 0;
    virtual void DestroyTextRecord(std::uint32_t record,
                                   std::uint32_t flags) = 0;
    virtual void DestroyLocale(std::uint32_t locale) = 0;
};

struct CombatLogTime {
    std::uint32_t second;
    std::uint32_t minute;
    std::uint32_t hour;
    std::uint32_t day;
    std::uint32_t month_zero_based;
    std::uint32_t year_since_1900;
};

class ICombatEventLogRuntime {
public:
    virtual ~ICombatEventLogRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void FormatMessage(std::uint32_t destination, const char* format,
                               std::va_list arguments) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::optional<CombatLogTime> LocalTime() = 0;
    virtual void ResetBuffer(std::uint32_t buffer) = 0;
    virtual void WriteBuffer(std::uint32_t buffer,
                             std::string_view text) = 0;
    virtual void FlushBuffer(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint8_t RecordEvent(
        std::uint32_t logger, std::uint32_t insertion_point,
        std::uint32_t level, std::string_view text) = 0;
    virtual void WritePrimary(std::uint32_t stream,
                              std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t FlushPrimary(
        std::uint32_t stream) = 0;
};

class ICombatLogFileRuntime {
public:
    virtual ~ICombatLogFileRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual int CloseFile(std::uint32_t handle) = 0;
    virtual void InitializeFileBuffer(std::uint32_t buffer,
                                      std::uint32_t handle,
                                      std::uint32_t mode) = 0;
    virtual void SetIoStreamState(std::uint32_t ios, std::uint32_t state,
                                  bool raise) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::string_view filename, std::uint32_t mode) = 0;
    virtual void GetCodecvtFacet(std::uint32_t buffer) = 0;
    virtual void InsertPrimary(std::uint32_t stream,
                               std::uint32_t text) = 0;
    virtual void FlushPrimary(std::uint32_t stream) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
};

class CombatEventLogger final {
public:
    // IDA 0x57de60.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t logger, ICombatEventLoggerRuntime& runtime);
    // IDA 0x57e020.
    static void Cleanup(std::uint32_t logger,
                        ICombatEventLoggerRuntime& runtime);
    // IDA 0x57e1e0. The native stream and CRT calls are runtime boundaries.
    [[nodiscard]] static std::uint32_t Log(
        std::uint32_t logger, std::uint32_t level,
        std::uint32_t message, ICombatEventLogRuntime& runtime,
        const char* format = nullptr, ...);
    // IDA 0x57e610.
    [[nodiscard]] static std::uint32_t SetLogFile(
        std::uint32_t logger, std::string_view filename,
        std::uint32_t mode, ICombatLogFileRuntime& runtime);
};

}  // namespace pal4
