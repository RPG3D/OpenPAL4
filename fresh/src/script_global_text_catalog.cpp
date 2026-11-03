#include "pal4/script_global_text_catalog.h"

#include <array>

namespace pal4 {
namespace {

// Source literals in launch.exe use GBK bytes.
constexpr std::array<std::string_view, 16> kPrimaryTexts{{
    "\xb5\xc8\xbc\xb6", "\xcb\xc0\xcd\xf6", "\xc9\xf1", "\xbe\xab",
    "\xc6\xf8", "\xc6\xf8", "\xc6\xd5\xcd\xa8\xb9\xa5\xbb\xf7",
    "\xb1\xa9\xbb\xf7\xb9\xa5\xbb\xf7", "\xb8\xf1\xb5\xb5",
    "\xb9\xa5\xbb\xf7\xce\xb4\xc3\xfc\xd6\xd0", "\xb6\xe3\xb1\xdc",
    "\xd4\xae\xbb\xa4", "\xba\xcf\xbb\xf7", "\xb7\xc0\xd3\xf9",
    "\xcc\xd3\xc5\xdc", "\xbe\xad\xd1\xe9",
}};

constexpr std::array<std::string_view, 3> kSecondaryDirectTexts{{
    "\xbd\xf0\xc7\xae", "\xc6\xc0\xb7\xd6", "\xba\xc4\xca\xb1",
}};

void Cleanup(std::uint32_t guard, std::uint32_t first,
             std::uint32_t count, IScriptGlobalTextRuntime& runtime) {
    const auto flags = runtime.ReadByte(guard);
    if ((flags & 1u) != 0u) return;
    runtime.WriteByte(guard, static_cast<std::uint8_t>(flags | 1u));
    runtime.DestroyRange(first, 16u, count,
                         ScriptGlobalTextCatalog::kTextDestructor);
}

}  // namespace

int ScriptGlobalTextCatalog::RegisterPrimary(
    std::uint8_t seed, IScriptGlobalTextRuntime& runtime) {
    static_cast<void>(InitializePrimary(seed, runtime));
    return runtime.RegisterAtExit(0x56efe0u);
}

std::uint32_t ScriptGlobalTextCatalog::InitializePrimary(
    std::uint8_t seed, IScriptGlobalTextRuntime& runtime) {
    for (std::uint32_t i = 0; i < kPrimaryTexts.size(); ++i) {
        const auto address = kPrimaryBase + i * 16u;
        runtime.WriteByte(address, seed);
        runtime.SetErrorFlag(address, 0u);
        runtime.SetExceptionMessage(address, kPrimaryTexts[i]);
    }
    return runtime.InitializeCString(kPrimaryBase + 16u * 16u,
                                     "Max", seed);
}

void ScriptGlobalTextCatalog::CleanupPrimary(
    IScriptGlobalTextRuntime& runtime) {
    Cleanup(kPrimaryGuard, kPrimaryBase, 17u, runtime);
}

int ScriptGlobalTextCatalog::RegisterSecondary(
    std::uint8_t seed, IScriptGlobalTextRuntime& runtime) {
    static_cast<void>(InitializeSecondary(seed, runtime));
    return runtime.RegisterAtExit(0x56f1b0u);
}

std::uint8_t ScriptGlobalTextCatalog::InitializeSecondary(
    std::uint8_t seed, IScriptGlobalTextRuntime& runtime) {
    for (std::uint32_t i = 0; i < kSecondaryDirectTexts.size(); ++i) {
        const auto address = kSecondaryBase + i * 16u;
        runtime.WriteByte(address, seed);
        runtime.WriteWord(address + 4u, 0u);
        runtime.WriteWord(address + 8u, 0u);
        runtime.WriteWord(address + 12u, 0u);
        runtime.SetExceptionMessage(address, kSecondaryDirectTexts[i]);
    }

    const std::array<std::string_view, 2> allocated_texts{{
        "\xd5\xbd\xb6\xb7\xbd\xe1\xb9\xfb", "Max"}};
    std::uint8_t result = 0u;
    for (std::uint32_t i = 0; i < allocated_texts.size(); ++i) {
        const auto address = kSecondaryBase + (i + 3u) * 16u;
        const auto text = allocated_texts[i];
        runtime.WriteByte(address, seed);
        runtime.SetErrorFlag(address, 0u);
        result = runtime.AllocateBuffer(
            address, static_cast<std::uint32_t>(text.size()), 1u);
        if (result != 0u) {
            const auto buffer = runtime.ReadWord(address + 4u);
            runtime.CopyBytes(buffer, text);
            runtime.WriteWord(address + 8u,
                              static_cast<std::uint32_t>(text.size()));
            runtime.WriteByte(buffer + static_cast<std::uint32_t>(text.size()),
                              0u);
            result = static_cast<std::uint8_t>(buffer);
        }
    }
    return result;
}

void ScriptGlobalTextCatalog::CleanupSecondary(
    IScriptGlobalTextRuntime& runtime) {
    Cleanup(kSecondaryGuard, kSecondaryBase, 5u, runtime);
}

}  // namespace pal4
