#include "pal4/file_enumerator.h"

namespace pal4 {

std::uint32_t FileEnumerator::ClearStrings(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    constexpr std::uint32_t kEmptyString = 0x8dc780u;
    runtime.CopyString(object, kEmptyString);
    runtime.CopyString(object + 648u, kEmptyString);
    runtime.CopyString(object + 972u, kEmptyString);
    runtime.WriteByte(object + 1296u, 0u);
    return object;
}

std::uint8_t FileEnumerator::IsDone(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    const auto state = runtime.ReadByte(object + 1296u);
    if (state != 0u) return state;
    return static_cast<std::uint8_t>(Close(object, runtime));
}

std::uint32_t FileEnumerator::Close(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    const auto handle = runtime.ReadWord(object + 644u);
    auto result = handle;
    if (handle != 0u && handle != 0xffffffffu) {
        result = runtime.FindClose(handle);
        runtime.WriteWord(object + 644u, 0u);
    }
    runtime.WriteByte(object + 1296u, 1u);
    return result;
}

std::uint32_t FileEnumerator::Initialize(
    const std::uint32_t object, std::uint32_t pattern,
    IFileEnumeratorRuntime& runtime) {
    (void)Close(object, runtime);
    runtime.WriteByte(object + 1296u, 0u);
    if (pattern == 0u) pattern = 0x8c26bcu;  // "*.*"
    runtime.CopyString(object + 368u, pattern);
    const auto handle = runtime.FindFirst(pattern, object + 324u);
    runtime.WriteWord(object + 644u, handle);
    if (handle == 0xffffffffu ||
        !runtime.FullPath(object, pattern, 324u)) {
        (void)Close(object, runtime);
        return 0u;
    }
    auto backslash = runtime.FindLastCharacter(object, '\\');
    auto slash = runtime.FindLastCharacter(object, '/');
    if (slash == 0u && backslash == 0u) return 1u;
    if (slash == 0u) slash = object;
    if (backslash == 0u) backslash = object;
    runtime.WriteByte(slash >= backslash ? slash : backslash, 0u);
    return 1u;
}

std::uint32_t FileEnumerator::Next(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    const auto handle = runtime.ReadWord(object + 644u);
    if (handle == 0u) return 0u;
    return runtime.FindNext(handle, object + 324u);
}

bool FileEnumerator::IsSpecialDirectory(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    if (runtime.ReadWord(object + 644u) == 0u ||
        !IsDirectory(object, runtime) ||
        runtime.ReadByte(object + 368u) != '.')
        return false;
    const auto second = runtime.ReadByte(object + 369u);
    return second == 0u ||
           (second == '.' && runtime.ReadByte(object + 370u) == 0u);
}

bool FileEnumerator::IsDirectory(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    return (runtime.ReadByte(object + 324u) & 0x10u) != 0u;
}

std::uint32_t FileEnumerator::GetCurrentFileName(
    const std::uint32_t object, IFileEnumeratorRuntime& runtime) {
    runtime.CopyString(object + 648u, object + 368u);
    return object + 648u;
}

}  // namespace pal4
