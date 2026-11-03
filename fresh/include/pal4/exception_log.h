#pragma once

#include <cstdarg>
#include <cstdint>
#include <string>

namespace pal4 {

class IExceptionLogRuntime {
public:
    virtual ~IExceptionLogRuntime() = default;
    [[nodiscard]] virtual std::string Format(const char* format, std::va_list args) = 0;
    [[nodiscard]] virtual std::uintptr_t Open(const char* path, const char* mode) = 0;
    virtual void Print(std::uintptr_t stream, const char* format) = 0;
    [[nodiscard]] virtual int Close(std::uintptr_t stream) = 0;
};

class ExceptionLog final {
public:
    static constexpr std::uint32_t kOriginalPathAddress = 0x8e0690u;
    static constexpr std::uint32_t kOriginalBufferAddress = 0x8e0288u;

    [[nodiscard]] int Create(const char* path, IExceptionLogRuntime& runtime);
    [[nodiscard]] int Append(IExceptionLogRuntime& runtime, const char* format, ...);
    [[nodiscard]] const std::string& Path() const { return path_; }
    [[nodiscard]] const std::string& Buffer() const { return buffer_; }

private:
    std::string path_;
    std::string buffer_;
};

}  // namespace pal4
