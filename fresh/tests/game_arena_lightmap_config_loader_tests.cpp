#include "pal4/game_arena_lightmap_config_loader.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameArenaLightmapConfigRuntime {
    std::vector<std::string> events;
    std::uint32_t file = 0x9000u;
    std::string opened_path;
    std::string released_path;

    std::string_view ReadCString(std::uint32_t field) override {
        events.push_back("text:" + std::to_string(field));
        if (field == 0x5004u) return std::string_view{"maps/\0ignored", 13};
        Check(field == 0x500cu, "scene field offset");
        return "qionghua";
    }
    std::uint32_t Open(std::string_view path,
                       std::string_view mode) override {
        Check(mode == "rb", "binary read mode");
        events.emplace_back("open");
        opened_path = path;
        return file;
    }
    void Read(std::uint32_t destination, std::uint32_t size,
              std::uint32_t count, std::uint32_t handle) override {
        Check(handle == file && count == 1u, "file and item count");
        events.push_back("read:" + std::to_string(destination) + ":" +
                         std::to_string(size));
    }
    void Close(std::uint32_t handle) override {
        Check(handle == file, "close opened file");
        events.emplace_back("close");
    }
    void ReleasePath(std::string_view path) override {
        events.emplace_back("release");
        released_path = path;
    }
};

void Success() {
    Runtime runtime;
    Check(pal4::GameArenaLightmapConfigLoader::Load(0x5000u, runtime),
          "opened config returns success");
    Check(runtime.opened_path == "maps/qionghua_ltMap.cfg" &&
              runtime.released_path == runtime.opened_path &&
              runtime.events == (std::vector<std::string>{
                  "text:20484", "text:20492", "open", "read:20908:12",
                  "read:20920:4", "close", "release"}),
          "file path, destinations and cleanup follow IDA order");
}

void OpenFailure() {
    Runtime runtime;
    runtime.file = 0u;
    Check(!pal4::GameArenaLightmapConfigLoader::Load(0x5000u, runtime),
          "failed open returns false");
    Check(runtime.events == (std::vector<std::string>{
              "text:20484", "text:20492", "open", "release"}) &&
              runtime.released_path == "maps/qionghua_ltMap.cfg",
          "failed open only releases temporary path");

    pal4::GameArenaLightmapConfigRuntimeStub stub;
    Check(!pal4::GameArenaLightmapConfigLoader::Load(0x5000u, stub),
          "native file stub links and fails safely");
}

}  // namespace

int main() {
    try {
        Success();
        OpenFailure();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
