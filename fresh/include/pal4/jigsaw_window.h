#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class JigsawWindowEvent { RenderingEnded, MouseButtonUp, Clicked };

class IJigsawWindowRuntime {
public:
    virtual ~IJigsawWindowRuntime() = default;
    virtual void LoadLayout(std::uint32_t object,
                            std::uint32_t layout_name) = 0;
    virtual void InitializePuzzle(std::uint32_t puzzle) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view WindowName(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindow(
        std::uint32_t object, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, JigsawWindowEvent event,
        std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
};

class JigsawWindow final {
public:
    // IDA 0x49f890.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name,
        IJigsawWindowRuntime& runtime);
};

}  // namespace pal4
