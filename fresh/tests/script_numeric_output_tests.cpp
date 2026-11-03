#include "pal4/script_numeric_output.h"

#include <cstdint>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct CharPunctuation final : std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '.'; }
    std::string do_grouping() const override { return "\3"; }
    std::string do_falsename() const override { return "no"; }
    std::string do_truename() const override { return "yes"; }
};

struct WidePunctuation final : std::numpunct<wchar_t> {
    std::string do_grouping() const override { return "\3"; }
    std::wstring do_falsename() const override { return L"no"; }
    std::wstring do_truename() const override { return L"yes"; }
};

void NumericFormats() {
    pal4::ScriptNumericFormat format;
    pal4::ScriptNumericOutput::EnsureFacets(format.locale);
    Check(pal4::ScriptNumericOutput::FormatSigned(-17, format) == "-17",
          "signed decimal formatting");
    format.flags = std::ios_base::hex | std::ios_base::showbase |
                   std::ios_base::uppercase;
    Check(pal4::ScriptNumericOutput::FormatUnsigned(42u, format) == "0X2A",
          "unsigned hex prefix and uppercase");
    format.flags = std::ios_base::dec | std::ios_base::showpos |
                   std::ios_base::internal;
    format.width = 6;
    format.fill = '0';
    Check(pal4::ScriptNumericOutput::FormatSigned(42, format) == "+00042",
          "internal sign padding");

    format = {};
    format.flags = std::ios_base::fixed;
    format.precision = 2;
    Check(pal4::ScriptNumericOutput::FormatFloat(1.25, format) == "1.25",
          "fixed floating point precision");
    format.precision = 3;
    Check(pal4::ScriptNumericOutput::FormatLongFloat(1.25L, format) ==
              "1.250",
          "long floating point precision");
    format.flags = std::ios_base::scientific | std::ios_base::uppercase;
    format.precision = 1;
    const auto scientific =
        pal4::ScriptNumericOutput::FormatFloat(12.5, format);
    Check(scientific.find('E') != std::string::npos,
          "scientific floating point uses uppercase exponent");

    format = {};
    const int value = 3;
    Check(!pal4::ScriptNumericOutput::FormatPointer(&value, format).empty(),
          "pointer formatter delegates to standard num_put");
    format.flags = std::ios_base::left;
    format.width = 5;
    format.fill = '_';
    Check(pal4::ScriptNumericOutput::PadText("ab", format) == "ab___",
          "raw text width and left padding");

    std::ostringstream output;
    pal4::ScriptNumericOutput::PutChar(output, 'A');
    pal4::ScriptNumericOutput::Fill(output, '-', 3u);
    pal4::ScriptNumericOutput::Fill(output, '?', 0u);
    Check(output.str() == "A---", "single character and repeated fill");
}

void LocaleFacets() {
    const std::locale char_locale(std::locale::classic(),
                                  new CharPunctuation());
    const std::locale locale(char_locale, new WidePunctuation());
    pal4::ScriptNumericOutput::EnsureFacets(locale);
    Check(pal4::ScriptNumericOutput::DecimalPoint(locale) == ',' &&
              pal4::ScriptNumericOutput::ThousandsSeparator(locale) == '.' &&
              pal4::ScriptNumericOutput::Grouping(locale) == "\3" &&
              pal4::ScriptNumericOutput::TrueName(locale) == "yes" &&
              pal4::ScriptNumericOutput::FalseName(locale) == "no",
          "numpunct character virtual results");
    Check(pal4::ScriptNumericOutput::WideGrouping(locale) == "\3" &&
              pal4::ScriptNumericOutput::WideTrueName(locale) == L"yes" &&
              pal4::ScriptNumericOutput::WideFalseName(locale) == L"no",
          "wide numpunct virtual results");

    pal4::ScriptNumericFormat format;
    format.locale = locale;
    Check(pal4::ScriptNumericOutput::FormatUnsigned(1234567u, format) ==
              "1.234.567",
          "numeric grouping follows locale facet");
    format.flags = std::ios_base::fixed;
    format.precision = 1;
    Check(pal4::ScriptNumericOutput::FormatFloat(1.5, format) == "1,5",
          "floating decimal point follows locale facet");
    format.flags = std::ios_base::boolalpha;
    Check(pal4::ScriptNumericOutput::FormatBool(true, format) == "yes" &&
              pal4::ScriptNumericOutput::FormatBool(false, format) == "no",
          "boolean names follow locale facet");
}

}  // namespace

int main() {
    try {
        NumericFormats();
        LocaleFacets();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
