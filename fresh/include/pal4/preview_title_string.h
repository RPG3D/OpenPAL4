#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace pal4 {

struct PreviewTitleStringState {
    std::array<std::uint8_t, 0x98> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IPreviewTitleStringRuntime {
public:
    virtual ~IPreviewTitleStringRuntime() = default;
    virtual void Grow(PreviewTitleStringState& state,
                      std::uint32_t length) = 0;
    virtual void WriteCodepoint(std::uint32_t address,
                                std::uint32_t codepoint) = 0;
    virtual void Destroy(PreviewTitleStringState& state) = 0;
    [[nodiscard]] virtual int RegisterAtExit(
        std::uint32_t cleanup_address) = 0;
};

class PreviewTitleStringRuntimeStub : public IPreviewTitleStringRuntime {
public:
    void Grow(PreviewTitleStringState&, std::uint32_t) override {}
    void WriteCodepoint(std::uint32_t, std::uint32_t) override {}
    void Destroy(PreviewTitleStringState&) override {}
    [[nodiscard]] int RegisterAtExit(std::uint32_t) override { return 0; }
};

class PreviewTitleString final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime,
        std::string_view name, std::uint32_t inline_address);
    static void Cleanup(PreviewTitleStringState& state, std::uint8_t& guard,
                        IPreviewTitleStringRuntime& runtime);
    [[nodiscard]] static int InitializeAndRegister(
        PreviewTitleStringState& state, IPreviewTitleStringRuntime& runtime,
        std::string_view name, std::uint32_t inline_address,
        std::uint32_t cleanup_address);
};

}  // namespace pal4
