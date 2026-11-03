#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGameArenaObjectListRuntime {
public:
    virtual ~IGameArenaObjectListRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool TextEqualsIgnoreCase(
        std::uint32_t slotTextAddress, std::string_view text) = 0;
    virtual void InitializeText(std::uint32_t slotTextAddress,
                                std::string_view text) = 0;
    virtual void ReleaseObjectChildren(std::uint32_t object) = 0;
    virtual void DestroyObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FreeObject(std::uint32_t object) = 0;
};

class GameArenaObjectListRuntimeStub final : public IGameArenaObjectListRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::string ReadCString(std::uint32_t) override { return {}; }
    [[nodiscard]] bool TextEqualsIgnoreCase(
        std::uint32_t, std::string_view) override { return false; }
    void InitializeText(std::uint32_t, std::string_view) override {}
    void ReleaseObjectChildren(std::uint32_t) override {}
    void DestroyObject(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FreeObject(std::uint32_t) override { return 0u; }
};

class GameArenaObjectList final {
public:
    // IDA 0x51bdc0, 0x51bed0, 0x51c040 and 0x51c060.
    [[nodiscard]] static std::uint32_t FindByName(
        std::uint32_t arena, IGameArenaObjectListRuntime& runtime);
    static void AddPending(
        std::uint32_t arena, IGameArenaObjectListRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t arena, IGameArenaObjectListRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t arena, IGameArenaObjectListRuntime& runtime);
};

}  // namespace pal4
