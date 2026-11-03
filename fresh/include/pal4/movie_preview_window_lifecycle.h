#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct MoviePreviewWindowState {
    std::array<std::uint8_t, 0x70> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IMoviePreviewWindowLifecycleRuntime {
public:
    virtual ~IMoviePreviewWindowLifecycleRuntime() = default;
    virtual void LoadLayout(MoviePreviewWindowState& state,
                            std::uint32_t name, std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeComponents(MoviePreviewWindowState& state) = 0;
    virtual void DestroyDialogBase(MoviePreviewWindowState& state) = 0;
    virtual void DestroyWindow(MoviePreviewWindowState& state) = 0;
    virtual void FreeMemory(std::uint32_t address) = 0;
};

class MoviePreviewWindowLifecycleRuntimeStub :
    public IMoviePreviewWindowLifecycleRuntime {
public:
    void LoadLayout(MoviePreviewWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializeComponents(MoviePreviewWindowState&) override {}
    void DestroyDialogBase(MoviePreviewWindowState&) override {}
    void DestroyWindow(MoviePreviewWindowState&) override {}
    void FreeMemory(std::uint32_t) override {}
};

class MoviePreviewWindowLifecycle final {
public:
    static constexpr std::uint32_t kInitialMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x843728u;
    static constexpr std::uint32_t kMemberVtable = 0x843710u;

    [[nodiscard]] static MoviePreviewWindowState& Construct(
        MoviePreviewWindowState& state, std::uint32_t name,
        IMoviePreviewWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        MoviePreviewWindowState& state, std::uint32_t object_address,
        std::uint32_t flags, IMoviePreviewWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
