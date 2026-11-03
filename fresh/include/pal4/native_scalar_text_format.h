#pragma once

#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::detail {
// Validate a database format before passing one scalar to the public CRT.
inline void ValidateScalarTextFormat(std::string_view format, bool integer) {
    unsigned conversions = 0;
    for (std::size_t i = 0; i < format.size(); ++i) {
        if (format[i] != '%') continue;
        if (++i < format.size() && format[i] == '%') continue;
        while (i < format.size() && std::string_view("-+ #0.0123456789").find(format[i]) != std::string_view::npos)
            ++i;
        const auto allowed = integer ? std::string_view("diuoxX") : std::string_view("eEfFgGaA");
        if (i == format.size() || allowed.find(format[i]) == std::string_view::npos || ++conversions > 1u)
            throw std::invalid_argument("database scalar format argument mismatch");
    }
}

template <typename Value>
std::string FormatScalarText(std::string_view format, Value value, bool integer) {
    ValidateScalarTextFormat(format, integer);
    const std::string terminated(format);
    const auto count = std::snprintf(nullptr, 0u, terminated.c_str(), value);
    if (count < 0) throw std::runtime_error("database scalar formatting failed");
    std::vector<char> buffer(static_cast<std::size_t>(count) + 1u);
    if (std::snprintf(buffer.data(), buffer.size(), terminated.c_str(), value) != count)
        throw std::runtime_error("database scalar formatting changed size");
    return {buffer.data(), static_cast<std::size_t>(count)};
}
}  // namespace pal4::detail
