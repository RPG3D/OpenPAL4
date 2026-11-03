#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <cstdio>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class NumericPunctuation final : public std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
    char do_thousands_sep() const override { return '_'; }
    std::string do_grouping() const override { return "\3"; }
    std::string do_truename() const override { return "yes"; }
    std::string do_falsename() const override { return "no"; }
};

void Integers() {
    std::istringstream stream("0x2a! 077 -1 65535");
    stream.imbue(std::locale::classic());
    long value = 0;
    stream >> std::setbase(0) >> value;
    Require(value == 42 && stream.peek() == '!', "hex parse consumed the delimiter");
    stream.get();
    stream >> value;
    Require(value == 63, "automatic octal base failed");
    unsigned int unsigned_value = 0;
    unsigned short short_value = 0;
    stream >> std::dec >> unsigned_value >> short_value;
    Require(unsigned_value == std::numeric_limits<unsigned int>::max(), "unsigned sign conversion failed");
    Require(short_value == 65535 && stream.eof() && !stream.fail(), "ushort max/EOF contract failed");

    // Exercise the public facet API itself: it does not perform sentry whitespace skipping.
    std::istringstream raw(" 42");
    raw.imbue(std::locale::classic());
    auto state = std::ios::goodbit;
    value = 77;
    const auto& facet = std::use_facet<std::num_get<char>>(raw.getloc());
    auto next = facet.get(std::istreambuf_iterator<char>(raw), {}, raw, state, value);
    Require(state == std::ios::failbit && next != std::istreambuf_iterator<char>() && *next == ' ',
            "facet must leave leading whitespace unconsumed");
    // Modern STL writes zero on failed conversion; VC6 0x74D683 preserves output.
    Require(value == 0, "host invalid-integer assignment contract changed");

    std::istringstream overflow("999999999999999999999999");
    overflow.imbue(std::locale::classic());
    value = 77;
    overflow >> value;
    Require(overflow.fail() && overflow.eof() && value == std::numeric_limits<long>::max(),
            "host integer overflow must saturate and report failure");

    std::locale custom(std::locale::classic(), new NumericPunctuation);
    std::istringstream grouped("1_234 12_34");
    grouped.imbue(custom);
    grouped >> value;
    Require(value == 1234 && !grouped.fail(), "valid locale grouping failed");
    grouped >> value;
    Require(grouped.fail(), "invalid locale grouping was accepted");

    int object = 0;
    std::stringstream pointer;
    pointer.imbue(std::locale::classic());
    pointer << static_cast<void*>(&object);
    void* recovered = nullptr;
    pointer >> recovered;
    Require(!pointer.fail() && recovered == &object, "host pointer text round trip failed");
}

void FloatingAndBoolean() {
    std::locale custom(std::locale::classic(), new NumericPunctuation);
    std::istringstream stream("yes no 12,5e1!");
    stream.imbue(custom);
    bool yes = false, no = true;
    double value = 0;
    stream >> std::boolalpha >> yes >> no >> value;
    Require(yes && !no && value == 125 && stream.peek() == '!', "locale names/decimal/exponent parse failed");

    std::istringstream numeric("0 1 2");
    numeric.imbue(std::locale::classic());
    numeric >> no >> yes;
    Require(!no && yes && !numeric.fail(), "numeric bool 0/1 failed");
    bool invalid = false;
    numeric >> invalid;
    // VC6 0x74D34F keeps the destination unchanged here; modern STL sets true.
    Require(numeric.fail() && invalid, "host invalid numeric bool contract changed");

    std::istringstream malformed("1e+");
    malformed.imbue(std::locale::classic());
    value = 77;
    malformed >> value;
    Require(malformed.fail() && malformed.eof() && value == 0, "host incomplete exponent contract changed");

    std::istringstream widths("1.25 1.23456789012345");
    widths.imbue(std::locale::classic());
    float single = 0;
    long double extended = 0;
    widths >> single >> extended;
    Require(!widths.fail() && single == 1.25f && std::abs(extended - 1.23456789012345L) < 1e-14L,
            "floating overload selected the wrong destination precision");
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one numeric case");
        const std::string_view name(argv[1]);
        if (name == "integers") Integers();
        else if (name == "floating_boolean") FloatingAndBoolean();
        else throw std::runtime_error("unknown numeric case");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
