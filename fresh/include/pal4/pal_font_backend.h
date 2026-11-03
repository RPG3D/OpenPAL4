#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PalFontDescription {
    std::int32_t height{};
    std::int32_t width{};
    std::int32_t weight{};
    std::int32_t fourth{};
    std::uint32_t face_name{};
    std::int32_t sixth{};
    std::array<std::uint8_t, 4> options{};
};

class IPalFontBackendRuntime {
public:
    virtual ~IPalFontBackendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentD3DDevice() = 0;
    [[nodiscard]] virtual std::int32_t CreateFromDescription(
        std::uint32_t device, std::int32_t height,
        std::int32_t width, std::int32_t weight,
        std::int32_t sixth, std::int32_t fourth,
        const std::array<std::uint8_t, 4>& options,
        std::uint32_t face_name,
        std::uint32_t font_object) = 0;
};

class PalFontBackendRuntimeStub final : public IPalFontBackendRuntime {
public:
    [[nodiscard]] std::uint32_t CurrentD3DDevice() override;
    [[nodiscard]] std::int32_t CreateFromDescription(
        std::uint32_t device, std::int32_t height,
        std::int32_t width, std::int32_t weight,
        std::int32_t sixth, std::int32_t fourth,
        const std::array<std::uint8_t, 4>& options,
        std::uint32_t face_name,
        std::uint32_t font_object) override;
};

class PalFontBackend final {
public:
    // 0x419e50. Backend receives sixth before fourth integer argument.
    [[nodiscard]] static bool CreateFromDescription(
        std::uint32_t font_object, const PalFontDescription& description,
        IPalFontBackendRuntime& runtime);
};

class IRenderStateObjectRuntime {
public:
    virtual ~IRenderStateObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CallSlot(
        std::uint32_t object, std::uint32_t byte_offset) = 0;
    [[nodiscard]] virtual std::uint32_t DrawRectangleSlot(
        std::uint32_t object, std::uint32_t first, std::int32_t command,
        std::int32_t sentinel,
        const std::array<std::int32_t, 4>& rectangle,
        std::int32_t last, std::int32_t penultimate) = 0;
};

class RenderStateObjectRuntimeStub final : public IRenderStateObjectRuntime {
public:
    [[nodiscard]] std::uint32_t CallSlot(
        std::uint32_t object, std::uint32_t byte_offset) override;
    [[nodiscard]] std::uint32_t DrawRectangleSlot(
        std::uint32_t object, std::uint32_t first,
        std::int32_t command, std::int32_t sentinel,
        const std::array<std::int32_t, 4>& rectangle,
        std::int32_t last, std::int32_t penultimate) override;
};

class RenderStateObject final {
public:
    // 0x419ea0, 0x419eb0, 0x419ec0, 0x419ee0.
    [[nodiscard]] static std::uint32_t GetTextureSlot40(
        std::uint32_t object, IRenderStateObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetTextureSlot44(
        std::uint32_t object, IRenderStateObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t& object, IRenderStateObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t DrawRectangle(
        std::uint32_t object, std::int32_t command,
        const std::array<std::int32_t, 4>& rectangle,
        std::int32_t penultimate, std::int32_t last,
        IRenderStateObjectRuntime& runtime);
};

}  // namespace pal4
