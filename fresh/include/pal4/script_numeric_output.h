#pragma once

#include <cstdint>
#include <ios>
#include <iosfwd>
#include <locale>
#include <string>
#include <string_view>

namespace pal4 {

struct ScriptNumericFormat {
    std::ios_base::fmtflags flags = std::ios_base::dec;
    std::streamsize precision = 6;
    std::streamsize width = 0;
    char fill = ' ';
    std::locale locale = std::locale::classic();
};

class ScriptNumericOutput final {
public:
    // IDA 0x56fa20-0x570ff2: source-level replacement for MSVC facets.
    static void EnsureFacets(const std::locale& locale);
    [[nodiscard]] static std::string FormatBool(
        bool value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string FormatSigned(
        std::int32_t value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string FormatUnsigned(
        std::uint32_t value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string FormatFloat(
        double value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string FormatLongFloat(
        long double value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string FormatPointer(
        const void* value, const ScriptNumericFormat& format);
    [[nodiscard]] static std::string PadText(
        std::string_view value, const ScriptNumericFormat& format);
    static void PutChar(std::ostream& stream, char value);
    static void Fill(std::ostream& stream, char value, std::size_t count);
    [[nodiscard]] static char DecimalPoint(const std::locale& locale);
    [[nodiscard]] static char ThousandsSeparator(const std::locale& locale);
    [[nodiscard]] static std::string Grouping(const std::locale& locale);
    [[nodiscard]] static std::string FalseName(const std::locale& locale);
    [[nodiscard]] static std::string TrueName(const std::locale& locale);
    [[nodiscard]] static std::string WideGrouping(const std::locale& locale);
    [[nodiscard]] static std::wstring WideFalseName(const std::locale& locale);
    [[nodiscard]] static std::wstring WideTrueName(const std::locale& locale);
};

}  // namespace pal4
