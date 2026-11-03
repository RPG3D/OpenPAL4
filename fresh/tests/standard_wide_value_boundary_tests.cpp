#include <cstdio>
#include <ctime>
#include <cwchar>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void Strings() {
    const std::wstring original(L"AB\0CD", 5);
    std::wstring copy = original;
    copy.erase(0, 0);
    Require(copy == original && copy.size() == 5,
            "copy mutation discarded characters after embedded NUL");
    // VC6 shared-buffer _Split recomputes length with wcslen before erase.
    copy.erase(1, 1);
    Require(copy == std::wstring(L"A\0CD", 4) && original.size() == 5,
            "erase corrupted binary string value or its source copy");
    std::wstring value = L"abcdef";
    value.assign(value, 1, 3);
    Require(value == L"bcd", "self substring assignment failed");
    value.append(value, 1, 2);
    Require(value == L"bcdcd", "self substring append failed");
    value.insert(1, 2, L'\0');
    Require(value == std::wstring(L"b\0\0cdcd", 7), "counted insertion lost embedded NULs");
    value.resize(9);
    Require(value.size() == 9 && value[7] == 0 && value[8] == 0,
            "resize did not value-initialize added characters");
    value.resize(2);
    Require(value == std::wstring(L"b\0", 2), "resize truncated at C-string boundary");
    value.assign(3, L'X');
    value.append(2, L'Y');
    Require(value == L"XXXYY", "counted assign/append failed");
    bool caught = false;
    try { value.erase(value.size() + 1, 0); }
    catch (const std::out_of_range&) { caught = true; }
    Require(caught && value == L"XXXYY", "invalid erase position was not rejected");
    wchar_t storage[] = {L'A', L'B', L'\0', L'C', L'D', L'Z'};
    Require(std::wmemmove(storage + 1, storage, 5) == storage + 1 &&
            std::wstring(storage, 6) == std::wstring(L"AAB\0CD", 6),
            "overlapping backward wide memory move failed");
    Require(std::wmemmove(storage, storage + 1, 5) == storage &&
            std::wstring(storage, 5) == original, "overlapping forward wide memory move failed");
}

void InputIterators() {
    std::wstringbuf buffer(std::wstring(L"A\0B", 3), std::ios::in);
    std::istreambuf_iterator<wchar_t> it(&buffer), end;
    Require(*it == L'A' && *it == L'A' && buffer.sgetc() == L'A',
            "repeated peek consumed a code unit");
    Require(*it++ == L'A' && *it == L'\0', "postincrement did not retain old value");
    ++it;
    Require(*it == L'B', "embedded NUL ended iterator range");
    ++it;
    Require(it == end, "iterator did not reach EOF");

    const wchar_t units[] = {L'A', static_cast<wchar_t>(0xffff), L'B'};
    std::wstringbuf sentinel(std::wstring(units, 3), std::ios::in);
    std::istreambuf_iterator<wchar_t> special(&sentinel);
    Require(*special == L'A', "sentinel fixture failed");
    ++special;
    Require(special == end, "MSVC U+FFFF input EOF collision changed");
    Require(sentinel.sbumpc() == std::char_traits<wchar_t>::eof() && sentinel.sgetc() == L'B',
            "U+FFFF should remain stored at iterator EOF and raw sbumpc should advance past it");
}

template<bool International>
class Currency final : public std::moneypunct<wchar_t, International> {
public:
    explicit Currency(bool spaced = false) : spaced_(spaced) {}
private:
    wchar_t do_decimal_point() const override { return L'.'; }
    wchar_t do_thousands_sep() const override { return L','; }
    std::string do_grouping() const override { return "\3"; }
    std::wstring do_curr_symbol() const override { return International ? L"USD" : L"$"; }
    std::wstring do_positive_sign() const override { return L""; }
    std::wstring do_negative_sign() const override { return L"()"; }
    int do_frac_digits() const override { return 2; }
    std::money_base::pattern Pattern() const {
        using M = std::money_base;
        return spaced_ ? M::pattern{{M::sign, M::symbol, M::space, M::value}}
                       : M::pattern{{M::sign, M::symbol, M::value, M::none}};
    }
    std::money_base::pattern do_pos_format() const override { return Pattern(); }
    std::money_base::pattern do_neg_format() const override { return Pattern(); }
    bool spaced_;
};

void MoneyInput() {
    const std::locale locale(std::locale::classic(), new Currency<false>);
    struct Case { const wchar_t* input; const wchar_t* result; bool fail; wchar_t next; };
    const Case cases[] = {
        {L"$12!", L"1200", false, L'!'},
        {L"$12.3!", L"unchanged", true, L'!'},
        {L"$12.345!", L"1234", false, L'5'},
        {L"($1,234.56)!", L"-123456", false, L'!'},
        {L"$12,34.56!", L"unchanged", true, L'.'},
    };
    for (const auto& test : cases) {
        std::wistringstream input(test.input);
        input.imbue(locale);
        std::wstring amount = L"unchanged";
        input >> std::showbase >> std::get_money(amount);
        Require(input.fail() == test.fail && amount == test.result,
                "money fraction/grouping result contract changed");
        input.clear();
        Require(input.peek() == test.next, "money scanner consumed an unexpected delimiter");
    }
    // VC6 pads short fractions and consumes/discards excess digits instead.
}

void MoneyPatterns() {
    const std::locale national(std::locale::classic(), new Currency<false>(true));
    const std::locale locale(national, new Currency<true>(true));
    std::wostringstream output;
    output.imbue(locale);
    output << std::showbase << std::put_money(std::wstring(L"-1234"));
    Require(output.str() == L"($ 12.34)", "money space or trailing sign character failed");
    std::wostringstream international;
    international.imbue(locale);
    international << std::showbase << std::put_money(std::wstring(L"-1234"), true);
    Require(international.str() == L"(USD 12.34)", "international facet selection failed");
    std::wostringstream padded;
    padded.imbue(locale);
    padded << std::showbase << std::internal << std::setfill(L'.') << std::setw(14)
           << std::put_money(std::wstring(L"-1234"));
    Require(padded.str() == L"($......12.34)" && padded.width() == 0,
            "internal monetary padding or width reset failed");
    std::wistringstream valid(L"(USD 12.34)!");
    valid.imbue(locale);
    std::wstring digits;
    valid >> std::showbase >> std::get_money(digits, true);
    Require(!valid.fail() && digits == L"-1234" && valid.peek() == L'!',
            "international sign/space input failed");
    std::wistringstream missing_space(L"($12.34)!");
    missing_space.imbue(locale);
    digits = L"unchanged";
    missing_space >> std::showbase >> std::get_money(digits);
    Require(missing_space.fail() && digits == L"unchanged", "required monetary space was ignored");
    // Original pattern-space input is optional; output emits padding only.
}

void TimeFields() {
    const auto& locale = std::locale::classic();
    const auto& facet = std::use_facet<std::time_get<wchar_t>>(locale);
    std::wistringstream signed_hour(L"+04:05:06!");
    signed_hour.imbue(locale);
    std::tm value{};
    value.tm_hour = 17;
    auto state = std::ios::goodbit;
    auto next = facet.get_time(std::istreambuf_iterator<wchar_t>(signed_hour), {}, signed_hour, state, &value);
    Require(!(state & std::ios::failbit) && value.tm_hour == 4 && value.tm_min == 5 &&
            value.tm_sec == 6 && *next == L'!', "signed time field or delimiter failed");
    // Both VC6 and current MSVC accept an optional sign before bounds checks.
    std::wistringstream long_hour(L"0004:05:06!");
    long_hour.imbue(locale);
    state = std::ios::goodbit;
    next = facet.get_time(std::istreambuf_iterator<wchar_t>(long_hour), {}, long_hour, state, &value);
    Require((state & std::ios::failbit) && *next == L'0',
            "host two-digit hour consumption limit changed");
    // VC6 _Getint strips all leading zeros before scanning the remaining digits.
    std::wistringstream invalid(L"04:05:99!");
    invalid.imbue(locale);
    value = {};
    value.tm_sec = 12;
    state = std::ios::goodbit;
    next = facet.get_time(std::istreambuf_iterator<wchar_t>(invalid), {}, invalid, state, &value);
    Require((state & std::ios::failbit) && value.tm_hour == 4 && value.tm_min == 5 && value.tm_sec == 12,
            "time-field partial assignment or bound validation changed");
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one wide value boundary case");
        const std::string_view name(argv[1]);
        if (name == "strings") Strings();
        else if (name == "input_iterators") InputIterators();
        else if (name == "money_input") MoneyInput();
        else if (name == "money_patterns") MoneyPatterns();
        else if (name == "time_fields") TimeFields();
        else throw std::runtime_error("unknown wide value boundary case");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
