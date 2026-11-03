#include "pal4/exception_description_catalog.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Case {
    std::uint32_t code;
    std::string_view text;
};

void CheckEveryKnownCode() {
    constexpr std::array<Case, 22> cases{{
        {0x80000001u, "{**EXCEPTION_GUARD_PAGE**}"},
        {0x80000002u, "{**EXCEPTION_DATATYPE_MISALIGNMENT**}"},
        {0x80000003u, "{**EXCEPTION_BREAKPOINT**}"},
        {0x80000004u, "{**EXCEPTION_SINGLE_STEP**}"},
        {0xc0000005u, "{**EXCEPTION_ACCESS_VIOLATION**}"},
        {0xc0000006u, "{**EXCEPTION_IN_PAGE_ERROR**}"},
        {0xc0000008u, "{**EXCEPTION_INVALID_HANDLE**}"},
        {0xc000001du, "{**EXCEPTION_ILLEGAL_INSTRUCTION**}"},
        {0xc0000025u, "{**EXCEPTION_NONCONTINUABLE_EXCEPTION**}"},
        {0xc0000026u, "{**EXCEPTION_INVALID_DISPOSITION**}"},
        {0xc000008cu, "{**EXCEPTION_ARRAY_BOUNDS_EXCEEDED**}"},
        {0xc000008du, "{**EXCEPTION_FLT_DENORMAL_OPERAND**}"},
        {0xc000008eu, "{**EXCEPTION_FLT_DIVIDE_BY_ZERO**}"},
        {0xc000008fu, "{**EXCEPTION_FLT_INEXACT_RESULT**}"},
        {0xc0000090u, "{**EXCEPTION_FLT_INVALID_OPERATION**}"},
        {0xc0000091u, "{**EXCEPTION_FLT_OVERFLOW**}"},
        {0xc0000092u, "{**EXCEPTION_FLT_STACK_CHECK**}"},
        {0xc0000093u, "{**EXCEPTION_FLT_UNDERFLOW**}"},
        {0xc0000094u, "{**EXCEPTION_INT_DIVIDE_BY_ZERO**}"},
        {0xc0000095u, "{**EXCEPTION_INT_OVERFLOW**}"},
        {0xc0000096u, "{**EXCEPTION_PRIV_INSTRUCTION**}"},
        {0xc00000fdu, "{**EXCEPTION_STACK_OVERFLOW**}"},
    }};
    for (const Case& item : cases) {
        const char* actual = pal4::ExceptionDescriptionCatalog::Lookup(item.code);
        Check(actual != nullptr && std::string_view(actual) == item.text,
              "known code resolves to its exact IDA string literal");
    }
}

void CheckUnknownCodes() {
    constexpr std::array<std::uint32_t, 10> unknown{{
        0u, 0x80000000u, 0x80000005u, 0xc0000004u,
        0xc0000007u, 0xc000008bu, 0xc0000097u,
        0xc00000fcu, 0xc00000feu, 0xffffffffu,
    }};
    for (const std::uint32_t code : unknown) {
        Check(pal4::ExceptionDescriptionCatalog::Lookup(code) == nullptr,
              "unmapped code returns null for FormatMessage fallback");
    }
}

}  // namespace

int main() {
    CheckEveryKnownCode();
    CheckUnknownCodes();
    std::cout << "22 known and 10 unknown exception code cases passed\n";
}
