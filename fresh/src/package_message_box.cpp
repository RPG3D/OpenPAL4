#include "pal4/package_message_box.h"

#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace pal4 {
namespace {

[[nodiscard]] std::string FormatText(const char* format, va_list args) {
    if (format == nullptr) throw std::invalid_argument("package message format");
    va_list measured;
    va_copy(measured, args);
    const int length = std::vsnprintf(nullptr, 0, format, measured);
    va_end(measured);
    if (length < 0) throw std::runtime_error("package message formatting");
    std::vector<char> buffer(static_cast<std::size_t>(length) + 1u);
    std::vsnprintf(buffer.data(), buffer.size(), format, args);
    return std::string(buffer.data(), static_cast<std::size_t>(length));
}

}  // namespace

int PackageMessageBox::ShowFormatted(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::string text;
    try {
        text = FormatText(format, args);
        va_end(args);
    } catch (...) {
        va_end(args);
        throw;
    }
    return runtime_.Show(text.c_str(), "Message", 0u);
}

int PackageMessageBox::ShowSystemError(
    const std::uint32_t type, const char* format, ...) {
    const std::uint32_t error = runtime_.LastError();
    va_list args;
    va_start(args, format);
    std::string text;
    try {
        text = FormatText(format, args);
        va_end(args);
    } catch (...) {
        va_end(args);
        throw;
    }
    const std::string message = runtime_.SystemErrorMessage(error, 0x400u);
    const std::string combined = text + "\nSystem Error Code : " +
        std::to_string(error) + "\nError Message : " + message;
    return runtime_.Show(combined.c_str(), "Error", type);
}

}  // namespace pal4
