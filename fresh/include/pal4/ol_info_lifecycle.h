#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct OlInfoState {
    std::array<std::uint8_t, 0x54> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IOlInfoLifecycleRuntime {
public:
    virtual ~IOlInfoLifecycleRuntime() = default;
    virtual void LoadLayout(OlInfoState& state, std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializePanel(OlInfoState& state) = 0;
    virtual void DestroyDialog(OlInfoState& state) = 0;
};

class OlInfoLifecycleRuntimeStub : public IOlInfoLifecycleRuntime {
public:
    void LoadLayout(OlInfoState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void InitializePanel(OlInfoState&) override {}
    void DestroyDialog(OlInfoState&) override {}
};

class OlInfoLifecycle final {
public:
    static constexpr std::uint32_t kBaseMemberVtable = 0x8435a4u;
    static constexpr std::uint32_t kObjectVtable = 0x8436a8u;
    static constexpr std::uint32_t kMemberVtable = 0x843690u;

    [[nodiscard]] static OlInfoState& Construct(
        OlInfoState& state, std::uint32_t name,
        IOlInfoLifecycleRuntime& runtime);
};

}  // namespace pal4
