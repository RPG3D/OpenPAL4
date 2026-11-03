#include "pal4/script_numeric_output.h"

#include <ostream>
#include <sstream>

namespace pal4 {
namespace {

std::ostringstream Stream(const ScriptNumericFormat& format) {
    std::ostringstream output;
    output.imbue(format.locale);
    output.flags(format.flags);
    output.precision(format.precision);
    output.width(format.width);
    output.fill(format.fill);
    return output;
}

template <typename T>
std::string Format(T value, const ScriptNumericFormat& format) {
    auto output = Stream(format);
    output << value;
    return output.str();
}

}  // namespace

void ScriptNumericOutput::EnsureFacets(const std::locale& locale) {
    static_cast<void>(std::use_facet<std::num_put<char>>(locale));
    static_cast<void>(std::use_facet<std::numpunct<char>>(locale));
}

std::string ScriptNumericOutput::FormatBool(
    bool value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::FormatSigned(
    std::int32_t value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::FormatUnsigned(
    std::uint32_t value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::FormatFloat(
    double value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::FormatLongFloat(
    long double value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::FormatPointer(
    const void* value, const ScriptNumericFormat& format) {
    return Format(value, format);
}

std::string ScriptNumericOutput::PadText(
    std::string_view value, const ScriptNumericFormat& format) {
    auto output = Stream(format);
    output << value;
    return output.str();
}

void ScriptNumericOutput::PutChar(std::ostream& stream, char value) {
    stream.put(value);
}

void ScriptNumericOutput::Fill(std::ostream& stream, char value,
                               std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) stream.put(value);
}

char ScriptNumericOutput::DecimalPoint(const std::locale& locale) {
    return std::use_facet<std::numpunct<char>>(locale).decimal_point();
}

char ScriptNumericOutput::ThousandsSeparator(const std::locale& locale) {
    return std::use_facet<std::numpunct<char>>(locale).thousands_sep();
}

std::string ScriptNumericOutput::Grouping(const std::locale& locale) {
    return std::use_facet<std::numpunct<char>>(locale).grouping();
}

std::string ScriptNumericOutput::FalseName(const std::locale& locale) {
    return std::use_facet<std::numpunct<char>>(locale).falsename();
}

std::string ScriptNumericOutput::TrueName(const std::locale& locale) {
    return std::use_facet<std::numpunct<char>>(locale).truename();
}

std::string ScriptNumericOutput::WideGrouping(const std::locale& locale) {
    return std::use_facet<std::numpunct<wchar_t>>(locale).grouping();
}

std::wstring ScriptNumericOutput::WideFalseName(const std::locale& locale) {
    return std::use_facet<std::numpunct<wchar_t>>(locale).falsename();
}

std::wstring ScriptNumericOutput::WideTrueName(const std::locale& locale) {
    return std::use_facet<std::numpunct<wchar_t>>(locale).truename();
}

}  // namespace pal4
