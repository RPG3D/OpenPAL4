#pragma once

#include <cstdint>

namespace pal4 {

struct EffectGlobalTreeState final {
    std::uint8_t tag{}, second{}, unused2{}, unused3{};
    std::uint32_t head{};
    std::uint8_t flag{}, unused9{}, unused10{}, unused11{};
    std::uint32_t count{};
};
static_assert(sizeof(EffectGlobalTreeState) == 16u);

class IEffectGlobalTreeRuntime {
public:
    virtual ~IEffectGlobalTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilUsers() = 0;
    virtual void SetSharedNilUsers(std::uint32_t users) = 0;
    virtual void FirstElement(EffectGlobalTreeState& state) = 0;
    [[nodiscard]] virtual int RegisterCleanup() = 0;
};

class EffectGlobalTree final {
public:
    // IDA 0x64b670.
    [[nodiscard]] static EffectGlobalTreeState* Construct(
        EffectGlobalTreeState& state, std::uint8_t first,
        std::uint8_t second, IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64b570: both tag bytes come from the upper half of ECX.
    [[nodiscard]] static EffectGlobalTreeState* InitializeHelper(
        EffectGlobalTreeState& state, std::uint32_t register_value,
        IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64b560, including the atexit tail at 0x64b590.
    [[nodiscard]] static int Initialize(
        EffectGlobalTreeState& state, std::uint32_t register_value,
        IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64b5a0.
    static void Cleanup(EffectGlobalTreeState& state,
                        IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64b710: erase a 36-byte node and return its next iterator node.
    [[nodiscard]] static std::uint32_t Erase(
        EffectGlobalTreeState& state, std::uint32_t node,
        IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64baf0: recurse right, then free along the left chain.
    [[nodiscard]] static std::uint32_t DestroyRecursive(
        std::uint32_t node, IEffectGlobalTreeRuntime& runtime);
    // IDA 0x64bb30: advance an in-order iterator.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t node, IEffectGlobalTreeRuntime& runtime);
};

}  // namespace pal4
