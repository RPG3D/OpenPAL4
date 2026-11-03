#include "pal4/logic_error_record.h"
#include "pal4/std_codecvt_facet_resolver.h"

namespace pal4 {

void ExceptionCopyThrower::ThrowLogicErrorCopy(const LogicErrorObject& source) {
    throw source;
}

void ExceptionCopyThrower::ThrowOutOfRangeCopy(const OutOfRangeErrorObject& source) {
    throw source;
}

const StdCodecvtChar& StdCodecvtFacetResolver::Resolve(const std::locale& locale) {
    return std::use_facet<StdCodecvtChar>(locale);
}

}  // namespace pal4
