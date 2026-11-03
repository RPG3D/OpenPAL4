#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct MoviePlayWindowState {
    std::array<std::uint8_t, 0x44> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IMoviePlayWindowLifecycleRuntime {
public:
    virtual ~IMoviePlayWindowLifecycleRuntime() = default;
    virtual void LoadLayout(MoviePlayWindowState& state, std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructVideoPath(MoviePlayWindowState& state,
                                    std::uint32_t offset) = 0;
    virtual void CleanupVideoPath(MoviePlayWindowState& state,
                                  std::uint32_t offset) = 0;
    virtual void InitializeButtons(MoviePlayWindowState& state) = 0;
    virtual void DestroyDialog(MoviePlayWindowState& state) = 0;
};

class MoviePlayWindowLifecycleRuntimeStub :
    public IMoviePlayWindowLifecycleRuntime {
public:
    void LoadLayout(MoviePlayWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void ConstructVideoPath(MoviePlayWindowState&,
                            std::uint32_t) override {}
    void CleanupVideoPath(MoviePlayWindowState&,
                          std::uint32_t) override {}
    void InitializeButtons(MoviePlayWindowState&) override {}
    void DestroyDialog(MoviePlayWindowState&) override {}
};

class MoviePlayWindowLifecycle final {
public:
    static constexpr std::uint32_t kBaseMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x843670u;
    static constexpr std::uint32_t kMemberVtable = 0x843658u;

    [[nodiscard]] static MoviePlayWindowState& Construct(
        MoviePlayWindowState& state, std::uint32_t name,
        IMoviePlayWindowLifecycleRuntime& runtime);
    static void Destroy(MoviePlayWindowState& state,
                        IMoviePlayWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
