#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

class ISaveWindowCallbackObjectRuntime {
public:
    virtual ~ISaveWindowCallbackObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::size_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyBase(std::uint32_t object) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class SaveWindowCallbackObjectRuntimeStub :
    public ISaveWindowCallbackObjectRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::size_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void DestroyBase(std::uint32_t) override {}
    void Free(std::uint32_t) override {}
};

class SaveWindowCallbackObject final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x8435d0u;
    static constexpr std::uint32_t kVtable = 0x8437b8u;

    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source,
        ISaveWindowCallbackObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint8_t flags,
        ISaveWindowCallbackObjectRuntime& runtime);
};

}  // namespace pal4
