#pragma once

#include "pal4/cegui_renderer.h"

#include <cstdint>
#include <map>
#include <optional>

namespace pal4 {

class ICeguiRendererNodeRuntime {
public:
    virtual ~ICeguiRendererNodeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructObject36(
        std::uint32_t address, CeguiRendererState& owner) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void ConfigureObject(std::uint32_t object,
                                 std::uint32_t first,
                                 std::uint32_t second) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(float value) = 0;
    virtual void ZeroBuffer(std::uint32_t address,
                            std::uint32_t bytes) = 0;
    virtual void ApplyGrid(std::uint32_t object,
                           std::uint32_t buffer,
                           std::int32_t width,
                           std::int32_t height) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyObject(
        std::uint32_t object, std::uint32_t flags) = 0;
};

class CeguiRendererNodeStub : public ICeguiRendererNodeRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t ConstructObject36(
        std::uint32_t address, CeguiRendererState& owner) override;
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    void ConfigureObject(std::uint32_t object,
                         std::uint32_t first,
                         std::uint32_t second) override;
    [[nodiscard]] std::int32_t FloatToInt(float value) override;
    void ZeroBuffer(std::uint32_t address,
                    std::uint32_t bytes) override;
    void ApplyGrid(std::uint32_t object,
                   std::uint32_t buffer,
                   std::int32_t width,
                   std::int32_t height) override;
    [[nodiscard]] std::uint32_t DestroyObject(
        std::uint32_t object, std::uint32_t flags) override;

private:
    std::uint32_t next_address_ = 0x10000u;
    std::map<std::uint32_t, std::uint32_t> words_;
};

class CeguiRendererNodes final {
public:
    [[nodiscard]] static std::uint32_t Create(
        CeguiRendererState& state, ICeguiRendererNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateAndConfigure(
        CeguiRendererState& state, std::uint32_t first,
        std::uint32_t second, ICeguiRendererNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateZeroGrid(
        CeguiRendererState& state, float side,
        ICeguiRendererNodeRuntime& runtime);
    [[nodiscard]] static std::optional<std::uint32_t> Remove(
        CeguiRendererState& state, std::uint32_t object,
        ICeguiRendererNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t Drain(
        CeguiRendererState& state, ICeguiRendererNodeRuntime& runtime);
};

}  // namespace pal4
