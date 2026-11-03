#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct MainWindowState {
    std::array<std::uint8_t, 0x6c> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IMainWindowLifecycleRuntime {
public:
    virtual ~IMainWindowLifecycleRuntime() = default;
    virtual void LoadLayout(MainWindowState& state, std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructMember(MainWindowState& state,
                                 std::uint32_t offset) = 0;
    virtual void CleanupMember(MainWindowState& state,
                               std::uint32_t offset) = 0;
    virtual void DestroyDialog(MainWindowState& state) = 0;
    virtual void InitializeButtons(MainWindowState& state) = 0;
    virtual void DestroyTimerFrame(MainWindowState& state) = 0;
    virtual void FreeObject(std::uint32_t object_address) = 0;
};

class MainWindowLifecycleRuntimeStub : public IMainWindowLifecycleRuntime {
public:
    void LoadLayout(MainWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void ConstructMember(MainWindowState&, std::uint32_t) override {}
    void CleanupMember(MainWindowState&, std::uint32_t) override {}
    void DestroyDialog(MainWindowState&) override {}
    void InitializeButtons(MainWindowState&) override {}
    void DestroyTimerFrame(MainWindowState&) override {}
    void FreeObject(std::uint32_t) override {}
};

class MainWindowLifecycle final {
public:
    static constexpr std::uint32_t kBaseMemberVtable = 0x843628u;
    static constexpr std::uint32_t kObjectVtable = 0x843610u;
    static constexpr std::uint32_t kMemberVtable = 0x8435e8u;

    [[nodiscard]] static MainWindowState& Construct(
        MainWindowState& state, std::uint32_t name,
        IMainWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        MainWindowState& state, std::uint32_t object_address,
        std::uint32_t flags, IMainWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
