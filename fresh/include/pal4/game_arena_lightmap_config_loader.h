#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameArenaLightmapConfigRuntime {
public:
    virtual ~IGameArenaLightmapConfigRuntime() = default;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t arena_field) = 0;
    [[nodiscard]] virtual std::uint32_t Open(std::string_view path,
                                              std::string_view mode) = 0;
    virtual void Read(std::uint32_t destination, std::uint32_t size,
                      std::uint32_t count, std::uint32_t file) = 0;
    virtual void Close(std::uint32_t file) = 0;
    virtual void ReleasePath(std::string_view path) = 0;
};

class GameArenaLightmapConfigRuntimeStub final
    : public IGameArenaLightmapConfigRuntime {
public:
    [[nodiscard]] std::string_view ReadCString(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::uint32_t Open(std::string_view,
                                      std::string_view) override { return 0; }
    void Read(std::uint32_t, std::uint32_t, std::uint32_t,
              std::uint32_t) override {}
    void Close(std::uint32_t) override {}
    void ReleasePath(std::string_view) override {}
};

class GameArenaLightmapConfigLoader final {
public:
    // IDA 0x51abf0.
    [[nodiscard]] static bool Load(std::uint32_t arena,
                                   IGameArenaLightmapConfigRuntime& runtime);
};

}  // namespace pal4
