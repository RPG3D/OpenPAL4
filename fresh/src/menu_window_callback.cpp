#include "pal4/menu_window_callback.h"

namespace pal4 {

namespace {

std::uint32_t Construct(const std::uint32_t memory,
                        const std::uint32_t owner,
                        const std::uint32_t argument,
                        const std::uint32_t function,
                        const std::uint32_t vtable,
                        IMenuWindowCallbackRuntime& runtime) {
    runtime.WriteWord(memory + 8u, owner);
    runtime.WriteWord(memory + 12u, argument);
    runtime.WriteWord(memory + 16u, function);
    runtime.WriteWord(memory, vtable);
    return memory;
}

}  // namespace

std::uint32_t MenuWindowCallback::ConstructAction(
    const std::uint32_t memory, const std::uint32_t owner,
    const std::uint32_t argument, const std::uint32_t function,
    IMenuWindowCallbackRuntime& runtime) {
    return Construct(memory, owner, argument, function,
                     kActionVtable, runtime);
}

std::uint32_t MenuWindowCallback::CloneAction(
    const std::uint32_t source, IMenuWindowCallbackRuntime& runtime) {
    const auto memory = runtime.Allocate(kBytes);
    if (memory == 0u) return 0u;
    runtime.WriteWord(memory, kBaseVtable);
    for (std::uint32_t offset = 8u; offset != 20u; offset += 4u)
        runtime.WriteWord(memory + offset,
                          runtime.ReadWord(source + offset));
    runtime.WriteWord(memory, kActionVtable);
    return memory;
}

std::uint32_t MenuWindowCallback::ConstructButton(
    const std::uint32_t memory, const std::uint32_t owner,
    const std::uint32_t argument, const std::uint32_t function,
    IMenuWindowCallbackRuntime& runtime) {
    return Construct(memory, owner, argument, function,
                     kButtonVtable, runtime);
}

}  // namespace pal4
