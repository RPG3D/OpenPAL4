#include <cstdio>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

using WideCodecvt = std::codecvt<wchar_t, char, std::mbstate_t>;

void Conversion() {
    const auto locale = std::locale::classic();
    const auto& conversion = std::use_facet<WideCodecvt>(locale);
    const char bytes[] = {'A', '\0', 'Z'};
    wchar_t wide[] = {L'?', L'?', L'?', L'G'};
    std::mbstate_t state{};
    const char* from_next = nullptr;
    wchar_t* to_next = nullptr;
    auto result = conversion.in(state, bytes, bytes + 3, from_next, wide, wide + 1, to_next);
    Require(result == std::codecvt_base::partial && from_next == bytes + 1 && to_next == wide + 1,
            "host input capacity/result contract changed");
    Require(wide[0] == L'A' && wide[1] == L'?', "input conversion exceeded capacity");
    result = conversion.in(state, from_next, bytes + 3, from_next, wide + 1, wide + 3, to_next);
    Require(result == std::codecvt_base::ok && from_next == bytes + 3 && to_next == wide + 3 &&
            std::wstring_view(wide, 3) == std::wstring_view(L"A\0Z", 3) && wide[3] == L'G',
            "input resumption or embedded NUL failed");

    char output[] = {'?', '?', '?', 'G'};
    const wchar_t* wide_next = nullptr;
    char* out_next = nullptr;
    state = {};
    result = conversion.out(state, wide, wide + 3, wide_next, output, output + 1, out_next);
    Require(result == std::codecvt_base::partial && wide_next == wide + 1 && out_next == output + 1,
            "host output capacity/result contract changed");
    result = conversion.out(state, wide_next, wide + 3, wide_next, output + 1, output + 3, out_next);
    Require(result == std::codecvt_base::ok && wide_next == wide + 3 && out_next == output + 3 &&
            std::string_view(output, 3) == std::string_view(bytes, 3) && output[3] == 'G',
            "output resumption or embedded NUL failed");
    state = {};
    Require(conversion.length(state, bytes, bytes + 3, 2) == 2,
            "host length must count external bytes within internal character budget");
    Require(!conversion.always_noconv(), "wide/byte conversion cannot report always_noconv");
}

void GbkConversion() {
    // Explicit Windows code page; no process-global locale mutation.
    const std::locale locale(".936");
    const auto& conversion = std::use_facet<WideCodecvt>(locale);
    const char bytes[] = {static_cast<char>(0xd6), static_cast<char>(0xd0), 'A'};
    wchar_t output[] = {L'?', L'?', L'G'};
    std::mbstate_t state{};
    const char* next = nullptr;
    wchar_t* out_next = nullptr;
    auto result = conversion.in(state, bytes, bytes + 3, next, output, output + 2, out_next);
    Require(result == std::codecvt_base::ok && next == bytes + 3 && out_next == output + 2 &&
            output[0] == L'\x4e2d' && output[1] == L'A' && output[2] == L'G',
            "explicit CP936 decoding failed");
    state = {};
    Require(conversion.length(state, bytes, bytes + 3, 1) == 2,
            "CP936 length must count two external bytes for one internal character");
    char encoded[] = {'G', 'G', 'G', 'G'};
    const wchar_t* wide_next = nullptr;
    char* encoded_next = nullptr;
    state = {};
    result = conversion.out(state, output, output + 1, wide_next, encoded, encoded + 1, encoded_next);
    Require(result == std::codecvt_base::partial && wide_next == output && encoded_next == encoded,
            "undersized CP936 output must not consume a wide character");
    state = {};
    result = conversion.out(state, output, output + 2, wide_next, encoded, encoded + 3, encoded_next);
    Require(result == std::codecvt_base::ok && std::string_view(encoded, 3) == std::string_view(bytes, 3) &&
            wide_next == output + 2 && encoded_next == encoded + 3 && encoded[3] == 'G',
            "CP936 output round trip failed");
}

void Classification() {
    const auto& facet = std::use_facet<std::ctype<wchar_t>>(std::locale::classic());
    const wchar_t input[] = {L'A', L'7', L' ', L'\0'};
    std::ctype_base::mask masks[4]{};
    Require(facet.is(input, input + 4, masks) == input + 4 &&
            (masks[0] & std::ctype_base::alpha) && (masks[1] & std::ctype_base::digit) &&
            (masks[2] & std::ctype_base::space), "wide range classification failed");
    Require(facet.scan_is(std::ctype_base::digit, input, input + 4) == input + 1 &&
            facet.scan_not(std::ctype_base::alpha, input, input + 4) == input + 1,
            "wide scans returned the wrong position");
    wchar_t lower[] = {L'A', L'Z'};
    Require(facet.tolower(lower, lower + 2) == lower + 2 && lower[0] == L'a' && lower[1] == L'z',
            "wide lowercase range failed");
    Require(facet.toupper(L'a') == L'A' && facet.widen('Z') == L'Z', "wide single-character mapping failed");
    Require(facet.narrow(L'\x4e2d', '?') == '?', "host narrow must honor its fallback character");
    char narrow[2]{};
    const wchar_t source[] = {L'A', L'\x4e2d'};
    Require(facet.narrow(source, source + 2, '?', narrow) == source + 2 &&
            narrow[0] == 'A' && narrow[1] == '?', "host range narrow fallback failed");
}

class CommaPunctuation final : public std::numpunct<wchar_t> {
    wchar_t do_decimal_point() const override { return L','; }
};

class MarkerCtype final : public std::ctype<wchar_t> {
    wchar_t do_tolower(wchar_t value) const override { return value == L'A' ? L'!' : value; }
};

void CompositionAndTime() {
    const auto classic = std::locale::classic();
    const std::locale punctuation(classic, new CommaPunctuation);
    const std::locale source(punctuation, new MarkerCtype);
    const std::locale numeric(classic, source, std::locale::numeric);
    // Compare observable category behavior; lazily supplied default facets do
    // not have to share object addresses across independently composed locales.
    Require(std::use_facet<std::numpunct<wchar_t>>(numeric).decimal_point() == L',' &&
            std::use_facet<std::ctype<wchar_t>>(source).tolower(L'A') == L'!' &&
            std::use_facet<std::ctype<wchar_t>>(numeric).tolower(L'A') == L'a',
            "locale category composition crossed unrelated facets");
    std::wostringstream stream;
    stream.imbue(numeric);
    stream << 1.5;
    Require(stream.str() == L"1,5", "wide stream did not use composed numeric facet");
    std::tm date{};
    date.tm_year = 124; date.tm_mon = 10; date.tm_mday = 3;
    date.tm_hour = 4; date.tm_min = 5; date.tm_sec = 6;
    std::wostringstream time;
    time.imbue(classic);
    time << std::put_time(&date, L"%Y-%m-%d %H:%M:%S");
    Require(!time.fail() && time.str() == L"2024-11-03 04:05:06", "wide time formatting failed");
}

class BorrowedPunctuation final : public std::numpunct<wchar_t> {
public:
    explicit BorrowedPunctuation(int& deaths) : std::numpunct<wchar_t>(1), deaths_(deaths) {}
    ~BorrowedPunctuation() override { ++deaths_; }
private:
    std::wstring do_truename() const override { return L"owned"; }
    int& deaths_;
};

void BorrowedFacetLifetime() {
    int deaths = 0;
    {
        BorrowedPunctuation facet(deaths);
        {
            const std::locale owner(std::locale::classic(), &facet);
            std::locale copy = owner;
            Require(&std::use_facet<std::numpunct<wchar_t>>(copy) == &facet,
                    "locale copy did not retain the explicitly installed facet");
            std::wostringstream output;
            output.imbue(copy);
            copy = std::locale::classic();
            output << std::boolalpha << true;
            Require(output.str() == L"owned", "stream lost its borrowed punctuation facet");
        }
        Require(deaths == 0, "locale destroyed a facet constructed with positive refs");
    }
    Require(deaths == 1, "caller-owned facet was not destroyed exactly once");
    // Native refs=1 controls ownership; never import VC6 +4/-1 refcount images.
}

void TextMatching() {
    const auto& locale = std::locale::classic();
    const auto& facet = std::use_facet<std::time_get<wchar_t>>(locale);
    struct Case { const wchar_t* input; int month; bool fail; };
    const Case cases[] = {{L"november!", 10, false}, {L"March!", 2, false}, {L"Ma!", 8, true}};
    for (const auto& test : cases) {
        std::wistringstream input(test.input);
        input.imbue(locale);
        std::tm value{};
        value.tm_mon = 8;
        auto state = std::ios::goodbit;
        const auto next = facet.get_monthname(std::istreambuf_iterator<wchar_t>(input), {}, input, state, &value);
        Require(static_cast<bool>(state & std::ios::failbit) == test.fail && value.tm_mon == test.month,
                "host month name case/prefix handling changed");
        Require(next != std::istreambuf_iterator<wchar_t>() && *next == L'!',
                "month-name matcher consumed the delimiter");
    }
    // Original _Getloctxt compares 16-bit code units directly, without case folding.
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one wide locale case");
        const std::string_view name(argv[1]);
        if (name == "conversion") Conversion();
        else if (name == "gbk_conversion") GbkConversion();
        else if (name == "classification") Classification();
        else if (name == "composition_time") CompositionAndTime();
        else if (name == "borrowed_lifetime") BorrowedFacetLifetime();
        else if (name == "text_matching") TextMatching();
        else throw std::runtime_error("unknown wide locale case");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
