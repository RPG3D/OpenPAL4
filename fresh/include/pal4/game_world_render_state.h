#pragma once

#include <cstdint>

namespace pal4 {

class IGameWorldRenderStateRuntime {
public:
    virtual ~IGameWorldRenderStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetD3DDevice() = 0;
    virtual void SetRenderState(std::uint32_t device, std::uint32_t state,
                                std::uint32_t value) = 0;
};

class GameWorldRenderStateRuntimeStub final
    : public IGameWorldRenderStateRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t GetD3DDevice() override { return 0; }
    void SetRenderState(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
};

class GameWorldRenderState final {
public:
    // IDA 0x51acf0 and 0x51ad00, including their out-of-line tails.
    static void Apply(std::uint32_t world,
                      IGameWorldRenderStateRuntime& runtime);
    static void Clear(std::uint32_t world,
                      IGameWorldRenderStateRuntime& runtime);
};

}  // namespace pal4
