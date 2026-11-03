#pragma once

#include <array>
#include <cstdarg>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalConsoleAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPalConsoleAccessRuntime {
public:
    virtual ~IPalConsoleAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
};

class PalConsoleAccess final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e40e8u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e40e4u;
    static constexpr std::uint32_t kFactoryAddress = 0x418f20u;
    static constexpr std::uint32_t kDestroyAddress = 0x418f80u;

    [[nodiscard]] static std::uint32_t Get(
        PalConsoleAccessState& state, IPalConsoleAccessRuntime& runtime);
};

class PalConsoleState final {
public:
    static constexpr std::uint32_t kBytes = 0x4a84u;
    static constexpr std::uint32_t kCommandCount = 0x4a6cu;
    static constexpr std::uint32_t kHistoryBegin = 0x4a74u;
    static constexpr std::uint32_t kHistoryEnd = 0x4a78u;
    static constexpr std::uint32_t kHistoryCapacity = 0x4a7cu;

    [[nodiscard]] std::array<std::uint8_t, kBytes>& Bytes() noexcept {
        return bytes_;
    }
    [[nodiscard]] const std::array<std::uint8_t, kBytes>& Bytes()
        const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);
    void Fill(std::uint32_t offset, std::uint32_t length,
              std::uint8_t value);

private:
    std::array<std::uint8_t, kBytes> bytes_{};
};

class IPalConsoleObjectRuntime {
public:
    virtual ~IPalConsoleObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual PalConsoleState& Access(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void CopyString(std::uint32_t destination,
                            const char* source,
                            std::uint32_t count) = 0;
};

class IPalConsoleRenderRuntime {
public:
    virtual ~IPalConsoleRenderRuntime() = default;
    virtual void DrawFilledRectangle(
        std::uint32_t context, const std::array<std::int32_t, 4>& rectangle,
        const std::array<std::uint8_t, 4>& color) = 0;
    [[nodiscard]] virtual std::uint32_t GetFontManager() = 0;
    virtual void DrawFontRectangle(
        std::uint32_t manager, std::uint32_t text,
        const std::array<std::int32_t, 4>& rectangle,
        std::uint32_t alignment, std::uint32_t color,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t address) = 0;
};

class IPalConsoleOutputRuntime {
public:
    virtual ~IPalConsoleOutputRuntime() = default;
    virtual void FormatToBuffer(char* buffer, std::uint32_t capacity,
                                const char* format, std::va_list args) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t CopyString(
        std::uint32_t destination, const char* source,
        std::uint32_t count) = 0;
};

class PalConsoleHistory final {
public:
    [[nodiscard]] static std::uint32_t Cleanup(
        PalConsoleState& state, IPalConsoleObjectRuntime& runtime);
};

class PalConsole final {
public:
    [[nodiscard]] static std::uint32_t Create(
        std::uint8_t incoming_stack_byte, IPalConsoleObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t address, std::uint8_t incoming_stack_byte,
        IPalConsoleObjectRuntime& runtime);
    static void Destroy(std::uint32_t address,
                        IPalConsoleObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t address, IPalConsoleObjectRuntime& runtime);
    static void Render(std::uint32_t address, std::uint32_t context,
                       PalConsoleState& state,
                       IPalConsoleRenderRuntime& runtime);
    [[nodiscard]] static bool AddCommand(
        std::uint32_t address, std::uint32_t callback,
        const char* name, const char* description,
        IPalConsoleObjectRuntime& runtime);
    [[nodiscard]] static std::uint32_t AddOutputLine(
        std::uint32_t address, IPalConsoleOutputRuntime& runtime,
        const char* format, ...);
    [[nodiscard]] static std::uint32_t CopyPromptRectangle(
        PalConsoleState& state,
        const std::array<std::uint32_t, 4>& rectangle);
    [[nodiscard]] static std::uint32_t CopyBackgroundRectangle(
        PalConsoleState& state,
        const std::array<std::uint32_t, 4>& rectangle);
    [[nodiscard]] static std::uint32_t InitializeRectangles(
        PalConsoleState& state, std::int32_t width, std::int32_t height);
};

}  // namespace pal4
