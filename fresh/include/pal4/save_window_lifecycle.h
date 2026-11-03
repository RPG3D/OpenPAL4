#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct SaveWindowState {
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

class ISaveWindowLifecycleRuntime {
public:
    virtual ~ISaveWindowLifecycleRuntime() = default;
    virtual void LoadLayout(SaveWindowState& state,
                            std::uint32_t name, std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeComponents(SaveWindowState& state) = 0;
    virtual void DestroyDialogBase(SaveWindowState& state) = 0;
};

class SaveWindowLifecycleRuntimeStub : public ISaveWindowLifecycleRuntime {
public:
    void LoadLayout(SaveWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializeComponents(SaveWindowState&) override {}
    void DestroyDialogBase(SaveWindowState&) override {}
};

class SaveWindowLifecycle final {
public:
    static constexpr std::uint32_t kInitialMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x843798u;
    static constexpr std::uint32_t kMemberVtable = 0x843780u;

    [[nodiscard]] static SaveWindowState& Construct(
        SaveWindowState& state, std::uint32_t name,
        ISaveWindowLifecycleRuntime& runtime);
    [[nodiscard]] static constexpr std::uint8_t ReturnTrue(
        std::uint32_t) noexcept { return 1u; }
};

}  // namespace pal4
