#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class GameDataPath : std::uint8_t {
    Data,
    World,
    Actor,
    Object,
    Weapon,
    Sound,
    Music,
    EditScene,
    EditScript,
    Effect,
    NormalTexture,
    ReflectionTexture,
    DualTexture,
};

struct GameDataPathSpec final {
    std::uint32_t register_entry;
    std::uint32_t initialize_entry;
    std::uint32_t cleanup_entry;
    std::uint32_t global_address;
    std::string_view path;
};

class IGameDataStaticPathsRuntime {
public:
    virtual ~IGameDataStaticPathsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitializeString(
        std::uint32_t global_address, std::string_view value) = 0;
    [[nodiscard]] virtual int RegisterAtexit(
        std::uint32_t cleanup_entry) = 0;
    virtual void CleanupString(std::uint32_t global_address) = 0;
};

class GameDataStaticPathsRuntimeStub final
    : public IGameDataStaticPathsRuntime {
public:
    [[nodiscard]] std::uint32_t InitializeString(
        std::uint32_t, std::string_view) override { return 0; }
    [[nodiscard]] int RegisterAtexit(std::uint32_t) override { return 0; }
    void CleanupString(std::uint32_t) override {}
};

class GameDataStaticPaths final {
public:
    [[nodiscard]] static const GameDataPathSpec& Spec(GameDataPath path);
    [[nodiscard]] static std::uint32_t Initialize(
        GameDataPath path, IGameDataStaticPathsRuntime& runtime);
    [[nodiscard]] static int Register(
        GameDataPath path, IGameDataStaticPathsRuntime& runtime);
    static void Cleanup(GameDataPath path,
                        IGameDataStaticPathsRuntime& runtime);
};

}  // namespace pal4
