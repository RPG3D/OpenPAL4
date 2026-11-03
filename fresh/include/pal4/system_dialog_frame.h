#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISystemDialogFrameRuntime {
public:
    virtual ~ISystemDialogFrameRuntime() = default;
    virtual void LoadLayout(std::uint32_t frame,
                            std::uint32_t name) = 0;
    virtual void DestroyBase(std::uint32_t frame) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginName(
        std::uint32_t source) = 0;
    virtual void ResetName(std::uint32_t buffer,
                           std::uint32_t source) = 0;
    virtual void AppendName(std::uint32_t buffer,
                            std::string_view suffix) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::uint32_t frame, std::uint32_t buffer) = 0;
    virtual void EndName(std::uint32_t buffer) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::uint32_t event,
        std::uint32_t callback) = 0;
    virtual void ReleaseConnection(std::uint32_t connection) = 0;
};

class SystemDialogFrame final {
public:
    static constexpr std::uint32_t kFrameVtable = 0x843c64u;
    static constexpr std::uint32_t kCallbackVtable = 0x843c5cu;
    static constexpr std::uint32_t kClickedEvent = 0x8411b8u;
    static constexpr std::uint32_t kKeyDownEvent = 0x84103cu;
    static constexpr std::uint32_t kOnOkClick = 0x483a80u;
    static constexpr std::uint32_t kOnKeyDown = 0x483a90u;

    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t frame, std::uint32_t name,
        ISystemDialogFrameRuntime& runtime);
};

}  // namespace pal4
