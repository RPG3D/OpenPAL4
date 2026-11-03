#pragma once

#include <stdexcept>

namespace pal4 {

// Native library objects own their text and exception state. No original-EXE
// pointers, vtables or reference-count bytes are exposed to the host.
using LogicErrorObject = std::logic_error;
using OutOfRangeErrorObject = std::out_of_range;

class ExceptionCopyThrower final {
public:
    [[noreturn]] static void ThrowLogicErrorCopy(const LogicErrorObject& source);
    [[noreturn]] static void ThrowOutOfRangeCopy(const OutOfRangeErrorObject& source);
};

}  // namespace pal4
