#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameArenaConfigDeserializeRuntime {
public:
    virtual ~IGameArenaConfigDeserializeRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string_view ReadSerializedMarker(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::string_view name) = 0;
    [[nodiscard]] virtual bool CheckVersionAndType(
        std::uint32_t source, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::string_view ReadAttribute(
        std::uint32_t node, std::string_view name,
        std::string_view default_value) = 0;
    virtual void LoadSceneResources(std::uint32_t arena,
                                    std::string_view world,
                                    std::string_view area,
                                    std::uint32_t mode) = 0;
    virtual void LoadSceneRuntime(std::uint32_t arena,
                                  std::string_view scene) = 0;
    virtual void InitializePlayerScene(std::uint32_t arena) = 0;
    virtual void ReleaseAttribute(std::string_view name) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
    virtual void ReleaseInput() = 0;
};

class GameArenaConfigDeserializeRuntimeStub final
    : public IGameArenaConfigDeserializeRuntime {
public:
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    [[nodiscard]] std::string_view ReadSerializedMarker(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::uint32_t CreateNode(std::string_view) override {
        return 0;
    }
    [[nodiscard]] bool CheckVersionAndType(std::uint32_t,
                                            std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::string_view ReadAttribute(
        std::uint32_t, std::string_view,
        std::string_view) override { return {}; }
    void LoadSceneResources(std::uint32_t, std::string_view,
                            std::string_view, std::uint32_t) override {}
    void LoadSceneRuntime(std::uint32_t, std::string_view) override {}
    void InitializePlayerScene(std::uint32_t) override {}
    void ReleaseAttribute(std::string_view) override {}
    void DestroyNode(std::uint32_t) override {}
    void ReleaseInput() override {}
};

class GameArenaConfigDeserializer final {
public:
    // IDA 0x51b000. Source is the original serialized-data handle.
    [[nodiscard]] static bool Deserialize(
        std::uint32_t arena, std::uint32_t source,
        IGameArenaConfigDeserializeRuntime& runtime);
};

}  // namespace pal4
