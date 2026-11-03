#include "pal4/exception_description_catalog.h"

namespace pal4 {

const char* ExceptionDescriptionCatalog::Lookup(const std::uint32_t code) {
    switch (code) {
    case 0x80000001u: return "{**EXCEPTION_GUARD_PAGE**}";
    case 0x80000002u: return "{**EXCEPTION_DATATYPE_MISALIGNMENT**}";
    case 0x80000003u: return "{**EXCEPTION_BREAKPOINT**}";
    case 0x80000004u: return "{**EXCEPTION_SINGLE_STEP**}";
    case 0xc0000005u: return "{**EXCEPTION_ACCESS_VIOLATION**}";
    case 0xc0000006u: return "{**EXCEPTION_IN_PAGE_ERROR**}";
    case 0xc0000008u: return "{**EXCEPTION_INVALID_HANDLE**}";
    case 0xc000001du: return "{**EXCEPTION_ILLEGAL_INSTRUCTION**}";
    case 0xc0000025u: return "{**EXCEPTION_NONCONTINUABLE_EXCEPTION**}";
    case 0xc0000026u: return "{**EXCEPTION_INVALID_DISPOSITION**}";
    case 0xc000008cu: return "{**EXCEPTION_ARRAY_BOUNDS_EXCEEDED**}";
    case 0xc000008du: return "{**EXCEPTION_FLT_DENORMAL_OPERAND**}";
    case 0xc000008eu: return "{**EXCEPTION_FLT_DIVIDE_BY_ZERO**}";
    case 0xc000008fu: return "{**EXCEPTION_FLT_INEXACT_RESULT**}";
    case 0xc0000090u: return "{**EXCEPTION_FLT_INVALID_OPERATION**}";
    case 0xc0000091u: return "{**EXCEPTION_FLT_OVERFLOW**}";
    case 0xc0000092u: return "{**EXCEPTION_FLT_STACK_CHECK**}";
    case 0xc0000093u: return "{**EXCEPTION_FLT_UNDERFLOW**}";
    case 0xc0000094u: return "{**EXCEPTION_INT_DIVIDE_BY_ZERO**}";
    case 0xc0000095u: return "{**EXCEPTION_INT_OVERFLOW**}";
    case 0xc0000096u: return "{**EXCEPTION_PRIV_INSTRUCTION**}";
    case 0xc00000fdu: return "{**EXCEPTION_STACK_OVERFLOW**}";
    default: return nullptr;
    }
}

}  // namespace pal4
