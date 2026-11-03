#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameDataFileLoaderRuntime {
public:
    virtual ~IGameDataFileLoaderRuntime() = default;
    [[nodiscard]] virtual bool LoadBootstrapEnabled() = 0;
    [[nodiscard]] virtual std::uint32_t LoadBootstrapScript(
        std::string_view path) = 0;
    virtual void ProcessBootstrapScript(std::uint32_t script) = 0;
    virtual void CleanupAllScripts() = 0;
    virtual void DestroyBootstrapScript(std::uint32_t script) = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(const char* path,
                                                 std::string_view mode) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t CreateStream(
        std::uint32_t file) = 0;
    virtual void DestroyStream(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::string_view ExpectedSignature() = 0;
    [[nodiscard]] virtual bool LoadScriptEntry(
        std::string_view name, std::uint32_t database,
        std::uint32_t stream, std::uint32_t context) = 0;
    [[nodiscard]] virtual bool LoadTableEntry(
        std::string_view name, std::uint32_t database,
        std::uint32_t stream, std::uint32_t context) = 0;
    virtual void RemoveTableEntry(std::uint32_t database,
                                  std::string_view name) = 0;
};

class GameDataFileLoader final {
public:
    // IDA 0x4d84f0: load the GameData database header, scripts and tables.
    [[nodiscard]] static bool Load(
        const char* path, std::uint32_t database, std::uint32_t context,
        IGameDataFileLoaderRuntime& runtime);
};

}  // namespace pal4
