#pragma once

#include "pal4/string_buffer.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace pal4 {

struct RwTextureObjectState final {
    std::uint32_t vtable{};
    std::uint8_t active{};
    std::uint8_t padding_05[3]{};
    float frame_rate{};
    float elapsed{};
    std::uint32_t clump{};
    std::uint8_t name_flag{};
    std::uint8_t padding_15[3]{};
    StringBufferState name{};
    std::uint8_t prefix_flag{};
    std::uint8_t padding_25[3]{};
    StringBufferState prefix{};
    std::uint32_t texture_count{};
    std::uint32_t texture_index{};
    std::uint32_t textures{};
};
static_assert(sizeof(RwTextureObjectState) == 64u);
static_assert(offsetof(RwTextureObjectState, name) == 24u);
static_assert(offsetof(RwTextureObjectState, prefix) == 40u);

class IRwTextureObjectRuntime {
public:
    virtual ~IRwTextureObjectRuntime() = default;
    virtual void InitializeEmpty(StringBufferState& text) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LoadClump(
        const std::string& path, const std::string& source,
        std::uint32_t options) = 0;
    virtual void AssignText(StringBufferState& text,
                            const std::string& value) = 0;
    virtual void PrepareImageName(const std::string& source) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTextureHandles(
        std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t ReadTexture(
        const std::string& filename) = 0;
    virtual void StoreTexture(std::uint32_t array, std::uint32_t index,
                              std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::string ReadText(
        const StringBufferState& text) = 0;
    [[nodiscard]] virtual std::uint32_t WorldForClump(
        std::uint32_t clump) = 0;
    virtual void RemoveClump(std::uint32_t world,
                             std::uint32_t clump) = 0;
    virtual void DestroyClump(std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::string NormalizeResourceName(
        const std::string& path) = 0;
    virtual void RemoveTextureByName(std::uint32_t dictionary,
                                     const std::string& name) = 0;
    [[nodiscard]] virtual std::uint32_t TextureAt(
        std::uint32_t array, std::uint32_t index) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    [[nodiscard]] virtual std::uint32_t RenderClump(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint8_t ApplyTexture(
        std::uint32_t clump, std::uint32_t texture) = 0;
};

class RwTextureObjectRuntimeStub final : public IRwTextureObjectRuntime {
public:
    void InitializeEmpty(StringBufferState&) override {}
    [[nodiscard]] std::uint8_t ReadByte(std::uint32_t) override { return 0; }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t LoadClump(
        const std::string&, const std::string&, std::uint32_t) override {
        return 0;
    }
    void AssignText(StringBufferState&, const std::string&) override {}
    void PrepareImageName(const std::string&) override {}
    [[nodiscard]] std::uint32_t AllocateTextureHandles(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t ReadTexture(
        const std::string&) override { return 0; }
    void StoreTexture(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
    [[nodiscard]] std::string ReadText(
        const StringBufferState&) override { return {}; }
    [[nodiscard]] std::uint32_t WorldForClump(
        std::uint32_t) override { return 0; }
    void RemoveClump(std::uint32_t, std::uint32_t) override {}
    void DestroyClump(std::uint32_t) override {}
    [[nodiscard]] std::string NormalizeResourceName(
        const std::string& path) override { return path; }
    void RemoveTextureByName(std::uint32_t,
                             const std::string&) override {}
    [[nodiscard]] std::uint32_t TextureAt(
        std::uint32_t, std::uint32_t) override { return 0; }
    void DestroyTexture(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RenderClump(
        std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint8_t ApplyTexture(
        std::uint32_t, std::uint32_t) override { return 0; }
};

class RwTextureObject final {
public:
    static constexpr std::uint32_t kVtableAddress = 0x84b988u;

    [[nodiscard]] static RwTextureObjectState& Construct(
        RwTextureObjectState& state, IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        RwTextureObjectState& state, IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static bool Read(
        RwTextureObjectState& state, const char* source,
        const char* segment, std::uint32_t clump_options,
        const char* frame_prefix, float frame_duration,
        std::uint32_t texture_count, IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static bool Cleanup(
        RwTextureObjectState& state, std::uint32_t dictionary,
        IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Advance(
        RwTextureObjectState& state, float delta,
        IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static std::uint8_t Render(
        const RwTextureObjectState& state,
        IRwTextureObjectRuntime& runtime);
    [[nodiscard]] static std::uint8_t ApplyCurrentTexture(
        const RwTextureObjectState& state,
        IRwTextureObjectRuntime& runtime);
};

}  // namespace pal4
