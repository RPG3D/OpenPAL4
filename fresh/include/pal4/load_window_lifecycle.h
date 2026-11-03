#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct LoadWindowState {
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

class ILoadWindowLifecycleRuntime {
public:
    virtual ~ILoadWindowLifecycleRuntime() = default;
    virtual void LoadLayout(LoadWindowState& state,
                            std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeListElements(LoadWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialog(
        LoadWindowState& state) = 0;
};

class LoadWindowLifecycleRuntimeStub : public ILoadWindowLifecycleRuntime {
public:
    void LoadLayout(LoadWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializeListElements(LoadWindowState&) override {}
    [[nodiscard]] std::uint32_t DestroyDialog(
        LoadWindowState&) override { return 0u; }
};

class LoadWindowLifecycle final {
public:
    static constexpr std::uint32_t kBaseMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x84358cu;
    static constexpr std::uint32_t kMemberVtable = 0x843574u;

    [[nodiscard]] static LoadWindowState& Construct(
        LoadWindowState& state, std::uint32_t name,
        ILoadWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        LoadWindowState& state, ILoadWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
