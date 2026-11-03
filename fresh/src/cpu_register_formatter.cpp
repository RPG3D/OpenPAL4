#include "pal4/cpu_register_formatter.h"

#include <array>
#include <cstdio>

namespace pal4 {

const char* CpuRegisterFormatter::Generate(
    const std::uint32_t exception_pointers,
    ICpuRegisterRuntime& runtime) {
    if (runtime.IsBadReadPointer(exception_pointers, 8u)) {
        return nullptr;
    }
    const std::uint32_t context = runtime.Read(exception_pointers + 4u);
    std::array<char, 512> buffer{};
    std::snprintf(
        buffer.data(), buffer.size(),
        "EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X  ESI=%08X\n"
        "EDI=%08X  EBP=%08X  ESP=%08X  EIP=%08X  FLG=%08X\n"
        "CS=%04X   DS=%04X  SS=%04X  ES=%04X   FS=%04X  GS=%04X",
        runtime.Read(context + 44u * 4u),
        runtime.Read(context + 41u * 4u),
        runtime.Read(context + 43u * 4u),
        runtime.Read(context + 42u * 4u),
        runtime.Read(context + 40u * 4u),
        runtime.Read(context + 39u * 4u),
        runtime.Read(context + 45u * 4u),
        runtime.Read(context + 49u * 4u),
        runtime.Read(context + 46u * 4u),
        runtime.Read(context + 48u * 4u),
        runtime.Read(context + 47u * 4u),
        runtime.Read(context + 38u * 4u),
        runtime.Read(context + 50u * 4u),
        runtime.Read(context + 37u * 4u),
        runtime.Read(context + 36u * 4u),
        runtime.Read(context + 35u * 4u));
    report_ = buffer.data();
    return report_.c_str();
}

}  // namespace pal4
