#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class NumericPunctuation final : public std::numpunct<wchar_t> {
    wchar_t do_decimal_point() const override { return L','; }
    wchar_t do_thousands_sep() const override { return L'_'; }
    std::string do_grouping() const override { return "\3"; }
    std::wstring do_truename() const override { return L"\u771f"; }
    std::wstring do_falsename() const override { return L"\u5047"; }
};

void NumericInput() {
    const std::locale locale(std::locale::classic(), new NumericPunctuation);
    std::wistringstream input(L"\u771f 70_000 12,5!");
    input.imbue(locale);
    bool flag = false;
    long integer = 0;
    double real = 0;
    input >> std::boolalpha >> flag >> integer >> real;
    Require(flag && integer == 70000 && real == 12.5 && input.peek() == L'!',
            "wide locale numeric input or delimiter consumption failed");

    std::wistringstream overflow(L"999999999999999999999999");
    overflow.imbue(std::locale::classic());
    integer = 77;
    overflow >> integer;
    Require(overflow.fail() && overflow.eof() && integer == std::numeric_limits<long>::max(),
            "host wide overflow assignment contract changed");
    std::wistringstream invalid(L"2");
    invalid.imbue(std::locale::classic());
    flag = false;
    invalid >> flag;
    Require(invalid.fail() && flag, "host invalid wide bool contract changed");
    // The original wide overloads leave destinations unchanged on failure.
}

class FailingOutput final : public std::wstreambuf {
    int_type overflow(int_type) override { return traits_type::eof(); }
};

void NumericOutput() {
    const std::locale locale(std::locale::classic(), new NumericPunctuation);
    std::wostringstream output;
    output.imbue(locale);
    output << std::boolalpha << std::setfill(L'.') << std::setw(4) << true
           << L' ' << 70000L << L' ' << std::fixed << std::setprecision(2) << 12.5;
    Require(output.str() == L"...\u771f 70_000 12,50" && output.width() == 0,
            "wide bool/numeric output lost locale, width reset or long range");
    std::wostringstream aligned;
    aligned.imbue(std::locale::classic());
    aligned << std::internal << std::showpos << std::setfill(L'.') << std::setw(6) << 42L;
    Require(aligned.str() == L"+...42", "internal alignment misplaced numeric sign");

    int object = 0;
    std::wstringstream pointer;
    pointer.imbue(std::locale::classic());
    pointer << static_cast<void*>(&object);
    void* parsed = nullptr;
    pointer >> parsed;
    Require(!pointer.fail() && parsed == &object, "wide host pointer text round trip failed");
    FailingOutput buffer;
    std::wostream failed(&buffer);
    failed.imbue(locale);
    const auto& facet = std::use_facet<std::num_put<wchar_t>>(locale);
    const auto iterator = facet.put(std::ostreambuf_iterator<wchar_t>(failed), failed, L' ', 42L);
    Require(iterator.failed(), "numeric facet failed to propagate iterator failure");
}

class CurrencyPunctuation final : public std::moneypunct<wchar_t, false> {
    wchar_t do_decimal_point() const override { return L'.'; }
    wchar_t do_thousands_sep() const override { return L','; }
    std::string do_grouping() const override { return "\3"; }
    std::wstring do_curr_symbol() const override { return L"$"; }
    std::wstring do_positive_sign() const override { return L""; }
    std::wstring do_negative_sign() const override { return L"-"; }
    int do_frac_digits() const override { return 2; }
    pattern do_pos_format() const override { return {{sign, symbol, value, none}}; }
    pattern do_neg_format() const override { return {{sign, symbol, value, none}}; }
};

void Money() {
    const std::locale locale(std::locale::classic(), new CurrencyPunctuation);
    std::wostringstream output;
    output.imbue(locale);
    output << std::showbase << std::put_money(-1234.0L) << L' '
           << std::put_money(std::wstring(L"-1234"));
    Require(output.str() == L"-$12.34 -$12.34", "money numeric/string formatting disagree");
    std::wistringstream input(L"-$12.34 -$12.34");
    input.imbue(locale);
    long double amount = 0;
    std::wstring digits;
    input >> std::showbase >> std::get_money(amount) >> std::get_money(digits);
    Require(!input.fail() && amount == -1234.0L && digits == L"-1234",
            "money input lost sign or minor-unit representation");
    const auto& punctuation = std::use_facet<std::moneypunct<wchar_t>>(locale);
    Require(punctuation.frac_digits() == 2 && punctuation.pos_format().field[0] == std::money_base::sign &&
            punctuation.neg_format().field[1] == std::money_base::symbol,
            "money pattern and fractional-digit APIs were confused with numpunct names");
}

void CollationAndMessages() {
    const auto& classic = std::locale::classic();
    const auto& facet = std::use_facet<std::collate<wchar_t>>(classic);
    const std::wstring a(L"A\0B", 3), b(L"A\0C", 3);
    Require(facet.compare(a.data(), a.data() + a.size(), b.data(), b.data() + b.size()) < 0,
            "collation ignored characters after embedded NUL");
    const auto ka = facet.transform(a.data(), a.data() + a.size());
    const auto kb = facet.transform(b.data(), b.data() + b.size());
    Require(ka < kb, "collation transform disagrees with compare");
    // Hash numbers are implementation-defined and are not VC6 persistence keys.
    const auto& messages = std::use_facet<std::messages<wchar_t>>(classic);
    Require(messages.get(-1, 1, 0, L"fallback") == L"fallback" &&
            messages.get(-1, 1, 1, L"fallback") == L"fallback",
            "invalid host catalog should retain fallback message");
}

void TimeInput() {
    const auto& locale = std::locale::classic();
    const auto& facet = std::use_facet<std::time_get<wchar_t>>(locale);
    std::wistringstream input(L"04:05:06!");
    input.imbue(locale);
    std::tm value{};
    auto state = std::ios::goodbit;
    auto next = facet.get_time(std::istreambuf_iterator<wchar_t>(input), {}, input, state, &value);
    Require(!(state & std::ios::failbit) && value.tm_hour == 4 && value.tm_min == 5 && value.tm_sec == 6 &&
            next != std::istreambuf_iterator<wchar_t>() && *next == L'!', "time fields or delimiter failed");
    std::wistringstream future(L"2036!");
    future.imbue(locale);
    state = std::ios::goodbit;
    next = facet.get_year(std::istreambuf_iterator<wchar_t>(future), {}, future, state, &value);
    Require(!(state & std::ios::failbit) && value.tm_year == 136 && *next == L'!',
            "host year behavior changed; original VC6 rejects years beyond 2035");
    std::wistringstream names(L"November Sunday");
    names.imbue(locale);
    value = {};
    names >> std::get_time(&value, L"%B %A");
    Require(!names.fail() && value.tm_mon == 10 && value.tm_wday == 0, "wide month/weekday names failed");
}

void NumericScannerEdges() {
    const std::locale locale(std::locale::classic(), new NumericPunctuation);
    std::wistringstream grouped(L"1_234,5!");
    grouped.imbue(locale);
    double real = 0;
    grouped >> real;
    Require(!grouped.fail() && real == 1234.5 && grouped.peek() == L'!',
            "host floating grouping contract changed");
    // VC6 _Getffld does not consume thousands separators: this input stops at '_'.
    std::wistringstream malformed(L"1_23!");
    malformed.imbue(locale);
    long integer = 77;
    malformed >> integer;
    Require(malformed.fail() && integer == 123, "host invalid grouping assignment changed");
    malformed.clear();
    Require(malformed.peek() == L'!', "invalid grouping consumed the delimiter");

    std::wistringstream bases(L"0x2a 077 09!");
    bases.imbue(std::locale::classic());
    long hex = 0, octal = 0, zero = -1;
    bases >> std::setbase(0) >> hex >> octal >> zero;
    Require(!bases.fail() && hex == 42 && octal == 63 && zero == 0 && bases.peek() == L'9',
            "automatic integer base or stopping position changed");
    std::wistringstream exponent(L"1e+!");
    exponent.imbue(std::locale::classic());
    real = 77;
    exponent >> real;
    Require(exponent.fail() && real == 0, "host incomplete exponent assignment changed");
    exponent.clear();
    Require(exponent.peek() == L'!', "incomplete exponent consumed delimiter");
}

void NumericFormatEdges() {
    const std::locale locale(std::locale::classic(), new NumericPunctuation);
    std::wostringstream grouped;
    grouped.imbue(locale);
    grouped << std::fixed << std::setprecision(1) << 1234.5;
    Require(grouped.str() == L"1_234,5", "host floating output grouping changed");
    // VC6 _Fput replaces the decimal point but never inserts thousands separators.
    std::wostringstream precise;
    precise.imbue(std::locale::classic());
    precise << std::fixed << std::setprecision(40) << (1.0 / 3.0);
    Require(precise.str() == L"0.3333333333333333148296162562473909929395",
            "host high precision double output changed");
    // VC6 formats at most 36 digits with sprintf, then _Fput inserts extra zeros.
    std::wostringstream prefix;
    prefix.imbue(std::locale::classic());
    prefix << std::hex << std::uppercase << std::showbase << std::internal
           << std::setfill(L'.') << std::setw(8) << 42L;
    Require(prefix.str() == L"0X....2A" && prefix.width() == 0,
            "hex prefix/internal padding/width reset failed");
    std::wostringstream scientific;
    scientific.imbue(std::locale::classic());
    scientific << std::scientific << std::uppercase << std::showpos << std::setprecision(2) << 12.5;
    Require(scientific.str() == L"+1.25E+01", "host exponent spelling changed");
}

class RecoveringOutput final : public std::wstreambuf {
public:
    int attempts = 0;
    std::wstring accepted;
private:
    int_type overflow(int_type value) override {
        if (++attempts == 1) return traits_type::eof();
        accepted.push_back(traits_type::to_char_type(value));
        return traits_type::not_eof(value);
    }
};

class FixedWideOutput final : public std::wstreambuf {
public:
    FixedWideOutput() { setp(storage, storage + 2); }
    std::wstring Written() const { return {pbase(), pptr()}; }
private:
    wchar_t storage[2]{};
};

void OutputIteratorEdges() {
    RecoveringOutput buffer;
    std::ostreambuf_iterator<wchar_t> iterator(&buffer);
    iterator = L'A';
    Require(iterator.failed(), "iterator did not retain initial failure");
    iterator = L'B';
    Require(iterator.failed() && buffer.attempts == 2 && buffer.accepted == L"B",
            "MSVC iterator retry/sticky failure contract changed");
    // Original 0x7511C5 and this MSVC host both retry while retaining failure.
    FixedWideOutput fixed;
    std::ostreambuf_iterator<wchar_t> direct(&fixed);
    direct = L'\0';
    Require(!direct.failed(), "embedded NUL was treated as EOF");
    direct = static_cast<wchar_t>(0xffff);
    Require(fixed.Written() == std::wstring(L"\0\xffff", 2) && direct.failed(),
            "MSVC 16-bit wchar EOF collision contract changed");
    // Both reviewed VC6 code and this MSVC host can store U+FFFF yet report failure.
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one wide formatting case");
        const std::string_view name(argv[1]);
        if (name == "numeric_input") NumericInput();
        else if (name == "numeric_output") NumericOutput();
        else if (name == "money") Money();
        else if (name == "collation_messages") CollationAndMessages();
        else if (name == "time_input") TimeInput();
        else if (name == "scanner_edges") NumericScannerEdges();
        else if (name == "format_edges") NumericFormatEdges();
        else if (name == "iterator_edges") OutputIteratorEdges();
        else throw std::runtime_error("unknown wide formatting case");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
