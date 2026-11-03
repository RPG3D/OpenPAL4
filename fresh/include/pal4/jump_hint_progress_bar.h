#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IJumpHintProgressDisplayRuntime {
public:
    virtual ~IJumpHintProgressDisplayRuntime() = default;
    virtual void SetWidth(std::uint32_t window, float width) = 0;
    [[nodiscard]] virtual std::uint32_t SetX(std::uint32_t window, float x) = 0;
    [[nodiscard]] virtual std::uint32_t IsVisible(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t SetVisibleBase(
        std::uint32_t object, std::uint32_t visible, std::uint32_t option) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
};

class IJumpHintProgressRuntime : public IJumpHintProgressDisplayRuntime {
public:
    virtual ~IJumpHintProgressRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            const char* layout) = 0;
    [[nodiscard]] virtual std::string ReadName(
        std::uint32_t name_field) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, const std::string& name) = 0;
    [[nodiscard]] virtual float GetX(std::uint32_t window) = 0;
    [[nodiscard]] virtual float GetWidth(std::uint32_t window) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window,
                                bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class JumpHintProgressBar final {
public:
    static constexpr std::uint32_t kVtable = 0x8442a0u;

    // IDA 0x4b52b0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IJumpHintProgressRuntime& runtime);
    // IDA 0x4b5420.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IJumpHintProgressRuntime& runtime);
    // IDA 0x4b5430.
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t object, std::uint32_t visible,
        IJumpHintProgressDisplayRuntime& runtime);
    // IDA 0x4b5460.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta,
        IJumpHintProgressDisplayRuntime& runtime);
    // IDA 0x4b54b0.
    [[nodiscard]] static std::uint32_t SetProgress(
        std::uint32_t object, float progress,
        IJumpHintProgressDisplayRuntime& runtime);
    // IDA 0x4b5500.
    [[nodiscard]] static float GetProgress(
        std::uint32_t object, IJumpHintProgressDisplayRuntime& runtime);
};

}  // namespace pal4
