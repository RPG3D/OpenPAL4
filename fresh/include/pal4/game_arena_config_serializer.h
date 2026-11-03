#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameArenaConfigRuntime {
public:
    virtual ~IGameArenaConfigRuntime() = default;
    virtual void PrepareTarget(std::uint32_t target,
                               std::string_view marker) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual bool HasAttribute(std::uint32_t node,
                                            std::string_view name) = 0;
    virtual void SetAttribute(std::uint32_t node, std::string_view name,
                              std::string_view value) = 0;
    virtual void AddAttribute(std::uint32_t node, std::string_view name,
                              std::string_view value) = 0;
    [[nodiscard]] virtual std::uint8_t Write(std::uint32_t node,
                                              std::uint32_t target) = 0;
    virtual void DestroyNode(std::uint32_t node) = 0;
    virtual void ReleaseMarker(std::string_view marker) = 0;
};

class GameArenaConfigRuntimeStub final : public IGameArenaConfigRuntime {
public:
    void PrepareTarget(std::uint32_t, std::string_view) override {}
    [[nodiscard]] std::uint32_t CreateNode(std::string_view) override {
        return 0;
    }
    [[nodiscard]] std::string_view ReadCString(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] bool HasAttribute(std::uint32_t,
                                    std::string_view) override {
        return false;
    }
    void SetAttribute(std::uint32_t, std::string_view,
                      std::string_view) override {}
    void AddAttribute(std::uint32_t, std::string_view,
                      std::string_view) override {}
    [[nodiscard]] std::uint8_t Write(std::uint32_t,
                                      std::uint32_t) override { return 0; }
    void DestroyNode(std::uint32_t) override {}
    void ReleaseMarker(std::string_view) override {}
};

class GameArenaConfigSerializer final {
public:
    // IDA 0x51ad10. Returns Config_Write's low byte.
    [[nodiscard]] static std::uint8_t Serialize(
        std::uint32_t arena, std::uint32_t target,
        IGameArenaConfigRuntime& runtime);
};

}  // namespace pal4
