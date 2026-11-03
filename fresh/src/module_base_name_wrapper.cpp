#include "pal4/module_base_name_wrapper.h"

#include <array>
#include <cstring>

namespace pal4 {

std::uint32_t ModuleBaseNameWrapper::Get(
    const std::uint32_t process, const std::uint32_t module,
    const std::uint32_t output, const std::uint32_t capacity,
    IModuleBaseNameRuntime& runtime) {
    if (runtime.CheckOperatingSystemPlatform() == 1u) {
        if (runtime.LoadDebugFunctions() == 0u) {
            runtime.SetLastErrorEx(0x45au, 1u);
            return 0u;
        }
        return runtime.GetModuleBaseNameA(process, module, output, capacity);
    }

    if (runtime.IsBadWritePointer(output, capacity) == 1u) {
        runtime.SetLastError(0x57u);
        return 0u;
    }

    std::array<char, 264> filename{};
    const std::uint32_t file_length = runtime.GetModuleFileNameA(
        module, filename.data(), 0x104u);
    if (file_length == 0u) {
        return 0u;
    }

    const char* last_separator = std::strrchr(filename.data(), '\\');
    const char* basename = last_separator != nullptr
        ? last_separator + 1 : filename.data();
    const std::uint32_t length =
        static_cast<std::uint32_t>(std::strlen(basename));
    const std::uint32_t copy_count =
        static_cast<std::int32_t>(capacity) >=
                static_cast<std::int32_t>(length + 1u)
            ? length + 1u : capacity;
    runtime.CopyN(output, basename, copy_count);
    runtime.WriteByte(output + copy_count, 0u);
    return copy_count - 1u;
}

}  // namespace pal4
