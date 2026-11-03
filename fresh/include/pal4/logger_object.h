#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ILoggerObjectRuntime {
public:
    virtual ~ILoggerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void InitializeEmptyPathBuffer(std::uint32_t object) = 0;
    virtual void AssignPathBuffer(std::uint32_t object,
                                  std::string_view path) = 0;
    virtual void CleanupPathBuffer(std::uint32_t object) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class ILoggerFileSetupRuntime : public IStringBufferRuntime {
public:
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenWriteFile(
        std::uint32_t filename_address, std::string_view mode) = 0;
    [[nodiscard]] virtual std::string GetModuleFileNameA(
        std::uint32_t capacity) = 0;
    [[nodiscard]] virtual std::string GetDateText() = 0;
    [[nodiscard]] virtual std::string GetTimeText() = 0;
    virtual void Fprintf(std::uint32_t stream, std::string_view format,
                         std::string_view argument) = 0;
    virtual void CloseFile(std::uint32_t stream) = 0;
};

class ILoggerAppendRuntime {
public:
    virtual ~ILoggerAppendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(
        std::uint32_t filename_address, std::string_view mode) = 0;
    [[nodiscard]] virtual std::string Vsprintf(
        std::uint32_t format_address,
        std::uint32_t arguments_address) = 0;
    virtual void Fprintf(std::uint32_t stream, std::string_view format) = 0;
    [[nodiscard]] virtual int CloseFile(std::uint32_t stream) = 0;
};

class LoggerObject final {
public:
    static constexpr std::string_view kInitialPath = "logFile.txt";
    static constexpr std::uint32_t kObjectSize = 0x0c;

    [[nodiscard]] static std::uint32_t Create(
        ILoggerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeDefaultPath(
        std::uint32_t object, ILoggerObjectRuntime& runtime);
    static void SetFilePathAndWriteHeader(std::uint32_t object,
                                          std::uint32_t path_address,
                                          ILoggerFileSetupRuntime& runtime);
    [[nodiscard]] static int AppendFormatted(
        std::uint32_t object, std::uint32_t format_address,
        std::uint32_t arguments_address, ILoggerAppendRuntime& runtime);
    static void Destroy(std::uint32_t object,
                        ILoggerObjectRuntime& runtime);
};

}  // namespace pal4
