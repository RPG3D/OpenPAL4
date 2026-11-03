#include <cstdio>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void Collation() {
    const auto& facet = std::use_facet<std::collate<char>>(std::locale::classic());
    const char high[] = {static_cast<char>(0x80)};
    const char low[] = {static_cast<char>(0x7f)};
    // Original 0x75526A uses signed jg/jl. This native contract intentionally
    // records the incompatibility; it is not an original-runtime equivalence test.
    Require(facet.compare(high, high + 1, low, low + 1) > 0 &&
            facet.compare(low, low + 1, high, high + 1) < 0,
            "host classic collation no longer orders high bytes unsigned");
    const char first[] = {'A', '\0', 'B'};
    const char second[] = {'A', '\0', 'C'};
    Require(facet.compare(first, first + 3, second, second + 3) < 0 &&
            facet.compare(first, first + 2, second, second + 2) == 0 &&
            facet.compare(first, first + 2, first, first + 3) < 0,
            "collation lost explicit lengths or stopped at embedded NUL");
    Require(facet.transform(first, first + 3) == std::string(first, 3),
            "host classic transform did not retain the full byte range");
    // Hashes remain opaque; VC6's signed-byte rotate/add is not a host promise.
}

class NumericMarker final : public std::numpunct<char> {
    char do_decimal_point() const override { return ','; }
};
template<bool International>
class MonetaryMarker final : public std::moneypunct<char, International> {
    std::string do_curr_symbol() const override { return International ? "INT" : "NAT"; }
};

void CategoryComposition() {
    const auto classic = std::locale::classic();
    const std::locale numeric_source(classic, new NumericMarker);
    const std::locale national_source(numeric_source, new MonetaryMarker<false>);
    const std::locale source(national_source, new MonetaryMarker<true>);
    const std::locale numeric(classic, source, std::locale::numeric);
    const std::locale monetary(classic, source, std::locale::monetary);
    Require(std::use_facet<std::numpunct<char>>(numeric).decimal_point() == ',' &&
            std::use_facet<std::numpunct<char>>(monetary).decimal_point() == '.',
            "native category composition mixed numeric and monetary facets");
    Require(std::use_facet<std::moneypunct<char, false>>(numeric).curr_symbol() ==
                std::use_facet<std::moneypunct<char, false>>(classic).curr_symbol() &&
            std::use_facet<std::moneypunct<char, true>>(numeric).curr_symbol() ==
                std::use_facet<std::moneypunct<char, true>>(classic).curr_symbol() &&
            std::use_facet<std::moneypunct<char, false>>(monetary).curr_symbol() == "NAT" &&
            std::use_facet<std::moneypunct<char, true>>(monetary).curr_symbol() == "INT",
            "native category composition selected the wrong monetary punctuation");
    std::ostringstream output;
    output.imbue(numeric);
    output << 1.5;
    Require(output.str() == "1,5", "composed numeric facet did not reach stream formatting");
    // Original _Makexloc chooses money_get/put on bit 4 and moneypunct on bit 8.
    // No VC6 category constants or object layouts cross the native API.
}

void StringInsertion() {
    std::string value("AB\0CD", 5);
    const auto original = value;
    Require(&value.insert(1, 3, 'x') == &value && value == std::string("AxxxB\0CD", 8) &&
            original == std::string("AB\0CD", 5) && value.data()[value.size()] == '\0',
            "counted insertion damaged suffix, copy or terminator");
    value.insert(2, 2, '\0');
    Require(value == std::string("Ax\0\0xxB\0CD", 10), "inserted NULs lost counted string data");
    const auto saved = value;
    Require(&value.insert(value.size(), 0, '?') == &value && value == saved,
            "zero-count insertion at end changed the value");
    bool range_error = false;
    try { value.insert(value.size() + 1, 0, '?'); }
    catch (const std::out_of_range&) { range_error = true; }
    Require(range_error && value == saved, "invalid position must throw even for zero count");
    bool length_error = false;
    try { value.insert(1, value.max_size(), '?'); }
    catch (const std::length_error&) { length_error = true; }
    Require(length_error && value == saved, "oversized insertion must preserve the original value");
    // 0x756DC4 is insert(pos,count,char), with range/length guards before mutation.
    // Native string handles overlap/storage; no private VC6 COW logic is copied.
}
}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "expected one narrow locale case");
        const std::string_view name(argv[1]);
        if (name == "collation") Collation();
        else if (name == "category_composition") CategoryComposition();
        else if (name == "string_insertion") StringInsertion();
        else throw std::runtime_error("unknown narrow locale case");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
