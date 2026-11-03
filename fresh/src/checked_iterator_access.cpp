#include "pal4/checked_iterator_access.h"

namespace pal4 {

std::uint32_t CheckedIteratorAccess::Container(
    const CheckedIteratorState& iterator,
    ICheckedIteratorRuntime& runtime) {
    return iterator.proxy == 0u ? 0u :
        runtime.ReadContainer(iterator.proxy);
}

}  // namespace pal4
