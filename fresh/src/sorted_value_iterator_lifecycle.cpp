#include "pal4/sorted_value_iterator_lifecycle.h"

namespace pal4 {

std::uint32_t SortedValueIteratorLifecycleRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
std::uint32_t SortedValueIteratorLifecycleRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
void SortedValueIteratorLifecycleRuntimeStub::WriteDword(
    std::uint32_t, std::uint32_t) {}
void SortedValueIteratorLifecycleRuntimeStub::Free(std::uint32_t) {}

void SortedValueIteratorLifecycle::SetBaseVtable(
    std::uint32_t address, ISortedValueIteratorLifecycleRuntime& runtime) {
    runtime.WriteDword(address, kBaseVtable);
}

std::uint32_t SortedValueIteratorLifecycle::CloneValid(
    std::uint32_t source, ISortedValueIteratorLifecycleRuntime& runtime) {
    const auto clone = runtime.Allocate(12u);
    if (clone == 0u)
        return 0u;
    runtime.WriteDword(clone, kBaseVtable);
    runtime.WriteDword(clone + 4u, runtime.ReadDword(source + 4u));
    runtime.WriteDword(clone + 8u, runtime.ReadDword(source + 8u));
    runtime.WriteDword(clone, kValidVtable);
    return clone;
}

std::uint32_t SortedValueIteratorLifecycle::CloneInvalid(
    std::uint32_t source, ISortedValueIteratorLifecycleRuntime& runtime) {
    const auto clone = runtime.Allocate(8u);
    if (clone == 0u)
        return 0u;
    runtime.WriteDword(clone, kBaseVtable);
    runtime.WriteDword(clone + 4u, runtime.ReadDword(source + 4u));
    runtime.WriteDword(clone, kInvalidVtable);
    return clone;
}

std::uint32_t SortedValueIteratorLifecycle::FieldAddress(
    std::uint32_t address) {
    return address + 4u;
}

std::uint32_t SortedValueIteratorLifecycle::DeleteBase(
    std::uint32_t address, std::uint32_t flags,
    ISortedValueIteratorLifecycleRuntime& runtime) {
    SetBaseVtable(address, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

std::uint32_t SortedValueIteratorLifecycle::DeleteDerived(
    std::uint32_t address, std::uint32_t flags,
    ISortedValueIteratorLifecycleRuntime& runtime) {
    SetBaseVtable(address, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

}  // namespace pal4
