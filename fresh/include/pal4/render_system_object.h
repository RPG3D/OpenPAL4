#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class RenderSystemObjectState final {
public:
    static constexpr std::uint32_t kBytes = 0x124u;

    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes() const {
        return bytes_;
    }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IRenderSystemObjectRuntime {
public:
    virtual ~IRenderSystemObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual RenderSystemObjectState& Access(
        std::uint32_t address) = 0;
    virtual void ClearObject(std::uint32_t address) = 0;
    virtual void InitializeString(std::uint32_t string_address) = 0;
    virtual void InitializeSecondaryBase(std::uint32_t address) = 0;
    virtual void ClearCollection(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void CleanupString(std::uint32_t string_address) = 0;
    virtual void CleanupBase(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class RenderSystemObject final {
public:
    static constexpr std::uint32_t kStringOffset = 0x118u;
    static constexpr std::uint32_t kManagerFactory = 0x40a040u;
    static constexpr std::uint32_t kManagerDestructor = 0x40a0b0u;
    static constexpr std::uint32_t kBufferInitializer = 0x40a010u;

    [[nodiscard]] static std::uint32_t Create(
        IRenderSystemObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, IRenderSystemObjectRuntime& runtime);
    static void Destruct(std::uint32_t address,
                         IRenderSystemObjectRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IRenderSystemObjectRuntime& runtime);
};

}  // namespace pal4
