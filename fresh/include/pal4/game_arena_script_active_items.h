#pragma once

#include <cstdint>
#include <initializer_list>

namespace pal4 {

enum class GameArenaScriptActiveItemCall : std::uint8_t {
    SetVirtual156,
    SetMaterialFlags,
    AddAnimatedObject,
    SetStateFlag,
};

class IGameArenaScriptActiveItemRuntime {
public:
    virtual ~IGameArenaScriptActiveItemRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool NamesEqualIgnoreCase(
        std::uint32_t left, std::uint32_t right) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void Call(GameArenaScriptActiveItemCall operation,
                      std::initializer_list<std::uint32_t> arguments) = 0;
};

class GameArenaScriptActiveItemRuntimeStub final
    : public IGameArenaScriptActiveItemRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool NamesEqualIgnoreCase(std::uint32_t,
                                             std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void Free(std::uint32_t) override {}
    void Call(GameArenaScriptActiveItemCall,
              std::initializer_list<std::uint32_t>) override {}
};

class GameArenaScriptActiveItems final {
public:
    // IDA 0x51cc00: +0x26c array, object +0x184 must be nonzero.
    [[nodiscard]] static std::uint32_t AddSceneObject(
        std::uint32_t script, std::uint32_t object, std::uint32_t context,
        IGameArenaScriptActiveItemRuntime& runtime);
    // IDA 0x51ceb0: +0x27c array, object +0x184 must be zero.
    [[nodiscard]] static std::uint32_t AddAnimatedObject(
        std::uint32_t script, std::uint32_t object, std::uint32_t context,
        IGameArenaScriptActiveItemRuntime& runtime);
    // IDA 0x51d1a0: +0x28c array, object +0x1f0 must be nonzero.
    [[nodiscard]] static std::uint32_t AddCharacter(
        std::uint32_t script, std::uint32_t object, std::uint32_t context,
        IGameArenaScriptActiveItemRuntime& runtime);
    // IDA 0x51d460: copy [first,last) as 12-byte records.
    [[nodiscard]] static std::uint32_t CopyRange(
        std::uint32_t first, std::uint32_t last, std::uint32_t destination,
        IGameArenaScriptActiveItemRuntime& runtime);
    // IDA 0x51d4a0: repeat one 12-byte source record count times.
    [[nodiscard]] static std::uint32_t Fill(
        std::uint32_t destination, std::uint32_t count, std::uint32_t source,
        IGameArenaScriptActiveItemRuntime& runtime);
};

}  // namespace pal4
