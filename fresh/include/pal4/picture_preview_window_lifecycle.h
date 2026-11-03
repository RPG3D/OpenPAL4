#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct PicturePreviewWindowState {
    std::array<std::uint8_t, 0x64> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IPicturePreviewWindowLifecycleRuntime {
public:
    virtual ~IPicturePreviewWindowLifecycleRuntime() = default;
    virtual void LoadLayout(PicturePreviewWindowState& state,
                            std::uint32_t name, std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeComponents(PicturePreviewWindowState& state) = 0;
    virtual void DestroyDialogBase(PicturePreviewWindowState& state) = 0;
};

class PicturePreviewWindowLifecycleRuntimeStub :
    public IPicturePreviewWindowLifecycleRuntime {
public:
    void LoadLayout(PicturePreviewWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializeComponents(PicturePreviewWindowState&) override {}
    void DestroyDialogBase(PicturePreviewWindowState&) override {}
};

class PicturePreviewWindowLifecycle final {
public:
    static constexpr std::uint32_t kInitialMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x843760u;
    static constexpr std::uint32_t kMemberVtable = 0x843748u;

    [[nodiscard]] static PicturePreviewWindowState& Construct(
        PicturePreviewWindowState& state, std::uint32_t name,
        IPicturePreviewWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
