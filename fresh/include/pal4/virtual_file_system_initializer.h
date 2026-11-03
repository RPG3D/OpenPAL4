#pragma once

#include "pal4/game_config_initializer.h"

#include <cstdint>
#include <string>
#include <string_view>

#undef GetCurrentDirectory
#undef GetLogicalDrives
#undef GetDriveType

namespace pal4 {

class IVirtualFileSystemRuntime : public IGameConfigLogger {
public:
    ~IVirtualFileSystemRuntime() override = default;

    [[nodiscard]] virtual std::uint32_t GetCurrentDirectory(
        std::uint32_t buffer_length, std::string& output) = 0;
    virtual void AppendPathSeparator(std::string& path) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCallbacks(
        std::uint32_t size, std::string_view path,
        std::string_view alias) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterCallbacks(
        std::uint32_t callbacks) = 0;
    virtual void UppercasePath(std::string& path) = 0;
    [[nodiscard]] virtual std::uint32_t GetLogicalDrives() = 0;
    [[nodiscard]] virtual std::uint32_t GetDriveType(
        std::string_view root) = 0;
    virtual void SetGlobalCallback(std::uint32_t callbacks) = 0;
    [[nodiscard]] virtual std::uint32_t GetGlobalCallback() = 0;
    [[nodiscard]] virtual std::uint32_t InitializePackFileManager() = 0;
    virtual void ProcessPackFiles(std::uint32_t manager,
                                  std::uint32_t callbacks) = 0;
};

class VirtualFileSystemInitializer final {
public:
    explicit VirtualFileSystemInitializer(GameRenderState& state)
        : state_(state) {}

    [[nodiscard]] std::uint32_t Initialize(
        IVirtualFileSystemRuntime& runtime);

private:
    GameRenderState& state_;
};

}  // namespace pal4
