#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string_view>

#include "pal4/pal_font_backend.h"

namespace pal4 {

struct PalFontManagerAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPalFontManagerAccessRuntime {
public:
    virtual ~IPalFontManagerAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
};

class PalFontManagerAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e40f0u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e40ecu;
    static constexpr std::uint32_t kFactoryAddress = 0x419f80u;
    static constexpr std::uint32_t kDestroyAddress = 0x419fe0u;

    [[nodiscard]] static std::uint32_t Get(
        PalFontManagerAccessState& state,
        IPalFontManagerAccessRuntime& runtime);
};

class PalFontManagerState final {
public:
    static constexpr std::uint32_t kBytes = 0x1cu;
    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IPalFontManagerObjectRuntime {
public:
    virtual ~IPalFontManagerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual PalFontManagerState& Access(
        std::uint32_t address) = 0;
    virtual void CallEmptyDestructor(std::uint32_t address) = 0;
};

class PalFontManagerObjectRuntimeStub final
    : public IPalFontManagerObjectRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Free(
        std::uint32_t address) override;
    [[nodiscard]] PalFontManagerState& Access(
        std::uint32_t address) override;
    void CallEmptyDestructor(std::uint32_t address) override;

private:
    std::map<std::uint32_t, PalFontManagerState> objects_;
    std::uint32_t next_address_{0x1000u};
};

class PalFontManagerObject final {
public:
    // 0x419f80, 0x419fe0, 0x41a000, 0x41a070.
    [[nodiscard]] static std::uint32_t Create(
        std::uint8_t incoming_stack_byte,
        IPalFontManagerObjectRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IPalFontManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, std::uint8_t incoming_stack_byte,
        IPalFontManagerObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Tidy(
        std::uint32_t address,
        IPalFontManagerObjectRuntime& runtime);
};

class IPalFontManagerFontRuntime {
public:
    virtual ~IPalFontManagerFontRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Log(std::uint32_t source_line) = 0;
    virtual void CreateFont(std::uint32_t font_slot,
                            const PalFontDescription& description) = 0;
    virtual void InsertOne(PalFontManagerState& state,
                           std::uint32_t font_slot) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateArray(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadArray(
        std::uint32_t address) = 0;
    virtual void WriteArray(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void FreeArray(std::uint32_t address) = 0;
};

class PalFontManagerFontRuntimeStub final : public IPalFontManagerFontRuntime {
public:
    [[nodiscard]] std::uint32_t Log(std::uint32_t source_line) override;
    void CreateFont(std::uint32_t font_slot,
                    const PalFontDescription& description) override;
    void InsertOne(PalFontManagerState& state,
                   std::uint32_t font_slot) override;
    [[nodiscard]] std::uint32_t AllocateArray(
        std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t ReadArray(
        std::uint32_t address) override;
    void WriteArray(std::uint32_t address,
                    std::uint32_t value) override;
    void FreeArray(std::uint32_t address) override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::uint32_t next_address_{0x2000u};
};

class PalFontManagerFonts final {
public:
    // 0x41a0e0 through 0x41a3d0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object_address, PalFontManagerState& state,
        IPalFontManagerFontRuntime& runtime);
    [[nodiscard]] static std::uint32_t Release(
        PalFontManagerState& state, IRenderStateObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetTextures(
        const PalFontManagerState& state,
        IPalFontManagerFontRuntime& array_runtime,
        IRenderStateObjectRuntime& render_runtime);
    [[nodiscard]] static std::uint32_t GetTextureStates(
        const PalFontManagerState& state,
        IPalFontManagerFontRuntime& array_runtime,
        IRenderStateObjectRuntime& render_runtime);
    [[nodiscard]] static std::uint32_t DrawRectangle(
        const PalFontManagerState& state, std::int32_t command,
        const std::array<std::int32_t, 4>& rectangle,
        std::uint32_t font_index, std::int32_t penultimate,
        std::int32_t last, IPalFontManagerFontRuntime& array_runtime,
        IRenderStateObjectRuntime& render_runtime);
};

}  // namespace pal4
