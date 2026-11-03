#include "pal4/memory_allocator_function_table.h"

namespace pal4 {

MemoryAllocatorFunctionTable& MemoryAllocatorFunctionTable::Initialize() {
    entries_ = {kAllocAddress, kFreeAddress, kReallocAddress,
                kZeroAllocAddress};
    return *this;
}

}  // namespace pal4
