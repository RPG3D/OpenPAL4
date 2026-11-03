#include "pal4/virtual_file_system_initializer.h"

namespace pal4 {
namespace {

constexpr std::string_view kSourceFile =
    R"(D:\\VcProject\\Pal4\\project\\Sourcecode\\RwCasing\\rwc_Initialize.cpp)";
constexpr std::string_view kDirectoryError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInstallFileSystem failed <CurrentDirectory error>]\n\n\n";
constexpr std::string_view kRegistrationError =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  Error[RWCInstallFileSystem failed <RtFSManagerRegister>]\n\n\n";
constexpr std::string_view kSuccess =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  RWCInstallFileSystem success\n\n\n";

void Log(IGameConfigLogger& runtime, const std::string_view format,
         const std::uint32_t line) {
    runtime.AppendLog(runtime.InitializeLogger(), format, kSourceFile, line);
}

}  // namespace

std::uint32_t VirtualFileSystemInitializer::Initialize(
    IVirtualFileSystemRuntime& runtime) {
    std::string current_directory;
    const std::uint32_t length =
        runtime.GetCurrentDirectory(0x104u, current_directory);
    if (length == 0u || length > 0x104u) {
        Log(runtime, kDirectoryError, 0x2f0u);
        return 0u;
    }

    runtime.AppendPathSeparator(current_directory);
    const std::uint32_t base_callbacks =
        runtime.CreateCallbacks(0x14u, current_directory, "rwc");
    if (base_callbacks == 0u) {
        return 0u;
    }
    if (runtime.RegisterCallbacks(base_callbacks) == 0u) {
        Log(runtime, kRegistrationError, 0x301u);
        return 0u;
    }

    runtime.UppercasePath(current_directory);
    std::uint32_t drives = runtime.GetLogicalDrives() >> 1u;
    std::uint32_t drive_index = 2u;
    while (drives != 0u) {
        if ((drives & 1u) != 0u) {
            const char letter = static_cast<char>(drive_index + 0x40u);
            const std::string root{letter, ':', '\\'};
            const std::uint32_t type = runtime.GetDriveType(root);
            if (type > 1u) {
                const std::string drive_path{letter, ':'};
                const std::string alias(1u, letter);
                const std::uint32_t drive_callbacks =
                    runtime.CreateCallbacks(0x14u, drive_path, alias);
                if (drive_callbacks == 0u ||
                    runtime.RegisterCallbacks(drive_callbacks) == 0u) {
                    return 0u;
                }
                if (current_directory.size() < 2u ||
                    current_directory[1] != ':') {
                    runtime.SetGlobalCallback(base_callbacks);
                } else if (current_directory[0] == letter) {
                    runtime.SetGlobalCallback(drive_callbacks);
                }
            }
        }
        ++drive_index;
        drives >>= 1u;
    }

    if (state_.Read(0x58cu) != 0u) {
        const std::uint32_t manager = runtime.InitializePackFileManager();
        runtime.ProcessPackFiles(manager, runtime.GetGlobalCallback());
    }
    Log(runtime, kSuccess, 0x34du);
    return 1u;
}

}  // namespace pal4
