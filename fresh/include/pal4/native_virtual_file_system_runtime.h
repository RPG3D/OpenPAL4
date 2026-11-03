#pragma once

#undef GetCurrentDirectory
#undef GetDriveType
#undef GetLogicalDrives

#include "pal4/virtual_file_system_initializer.h"

namespace pal4 {

class IPackageFileMountRuntime {
public:
    virtual ~IPackageFileMountRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializePackFileManager() = 0;
    virtual void ProcessPackFiles(std::uint32_t manager,
                                  std::uint32_t file_system) = 0;
};

class NativeVirtualFileSystemRuntime : public IVirtualFileSystemRuntime {
public:
    NativeVirtualFileSystemRuntime(IGameConfigLogger& logger,
                                   IPackageFileMountRuntime& packages)
        : logger_(logger), packages_(packages) {}

    [[nodiscard]] std::uint32_t GetCurrentDirectory(
        std::uint32_t buffer_length, std::string& output) override;
    void AppendPathSeparator(std::string& path) override;
    [[nodiscard]] std::uint32_t CreateCallbacks(
        std::uint32_t size, std::string_view path,
        std::string_view alias) override;
    [[nodiscard]] std::uint32_t RegisterCallbacks(
        std::uint32_t callbacks) override;
    void UppercasePath(std::string& path) override;
    [[nodiscard]] std::uint32_t GetLogicalDrives() override;
    [[nodiscard]] std::uint32_t GetDriveType(
        std::string_view root) override;
    void SetGlobalCallback(std::uint32_t callbacks) override;
    [[nodiscard]] std::uint32_t GetGlobalCallback() override;
    [[nodiscard]] std::uint32_t InitializePackFileManager() override;
    void ProcessPackFiles(std::uint32_t manager,
                          std::uint32_t callbacks) override;
    [[nodiscard]] std::uint32_t InitializeLogger() override;
    void AppendLog(std::uint32_t logger, std::string_view format,
                   std::string_view source_file,
                   std::uint32_t source_line) override;

private:
    IGameConfigLogger& logger_;
    IPackageFileMountRuntime& packages_;
};

}  // namespace pal4
