#include "pal4/cegui_resource_provider.h"

namespace pal4 {

std::uint32_t CeguiResourceRuntimeStub::Allocate(const std::uint32_t bytes) {
    const auto address = next_address_;
    next_address_ += bytes + 0x10u;
    return address;
}
void CeguiResourceRuntimeStub::Free(std::uint32_t /*address*/) {}
std::uint32_t CeguiResourceRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
void CeguiResourceRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
std::uint32_t CeguiResourceRuntimeStub::DefaultProviderVtable() {
    return 0u;
}
std::uint32_t CeguiResourceRuntimeStub::BaseProviderVtable() {
    return 0u;
}
void CeguiResourceRuntimeStub::DestroyString(
    std::uint32_t /*address*/) {}
std::uint32_t CeguiResourceRuntimeStub::ConstructAlternateStream(
    const std::uint32_t address) { return address; }
void CeguiResourceRuntimeStub::ConstructDefaultStream(
    std::uint32_t /*address*/) {}

std::uint32_t CeguiResourceProvider::Create(
    ICeguiResourceRuntime& runtime) {
    const auto object = runtime.Allocate(kBytes);
    if (object == 0u) {
        return 0u;
    }
    runtime.WriteDword(object + 8u, 32u);
    runtime.WriteDword(object + 12u, 0u);
    runtime.WriteDword(object + 20u, 0u);
    runtime.WriteDword(object + 16u, 0u);
    runtime.WriteDword(object + 4u, 0u);
    runtime.WriteDword(object + 24u, 0u);
    runtime.WriteDword(object, kVtable);
    return object;
}

void CeguiResourceProvider::Destroy(
    const std::uint32_t object, ICeguiResourceRuntime& runtime) {
    runtime.WriteDword(object, kVtable);
    runtime.WriteDword(object, runtime.DefaultProviderVtable());
    runtime.WriteDword(object, runtime.BaseProviderVtable());
    runtime.DestroyString(object + 4u);
}

std::uint32_t CeguiFileStreamFactory::Create(
    const std::uint32_t game_state_base,
    ICeguiResourceRuntime& runtime) {
    const auto use_alternate =
        runtime.ReadDword(game_state_base + 0x58cu) != 0u;
    const auto object = runtime.Allocate(kBytes);
    if (object == 0u) {
        return 0u;
    }
    try {
        if (use_alternate) {
            return runtime.ConstructAlternateStream(object);
        }
        runtime.ConstructDefaultStream(object);
        runtime.WriteDword(object, kDefaultVtable);
        return object;
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

}  // namespace pal4
