#include "pal4/archive_object_virtual_dispatch.h"

namespace pal4 {

std::int32_t ArchiveObjectVirtualDispatch::ReleaseSlot12(
    std::uint32_t object) {
    return runtime_.Invoke(object, 12u);
}

std::int32_t ArchiveObjectVirtualDispatch::ReleaseSlot8(
    std::uint32_t object) {
    return runtime_.Invoke(object, 8u);
}

}  // namespace pal4
