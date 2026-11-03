#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class UiObjectState final {
public:
    static constexpr std::uint32_t kVtable = 0x8434c8u;
    static constexpr std::uint32_t kMemberVtable = 0x8434a0u;
    static constexpr std::uint32_t kTailVtable = 0x8434e0u;

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const {
        return words_.at(offset / 4u);
    }
    void Write(std::uint32_t offset, std::uint32_t value) {
        words_.at(offset / 4u) = value;
    }

private:
    std::array<std::uint32_t, 0x5cu / 4u> words_{};
};

class IUiObjectLifecycleRuntime {
public:
    virtual ~IUiObjectLifecycleRuntime() = default;
    virtual void LoadLayout(UiObjectState& state,
                            std::uint32_t layout) = 0;
    virtual void ConstructMember(UiObjectState& state) = 0;
    virtual void InitializeElements(UiObjectState& state) = 0;
    virtual void DestroyMember(UiObjectState& state) = 0;
    virtual void DestroyLayout(UiObjectState& state) = 0;
    virtual void FreeMemberBuffer(std::uint32_t buffer) = 0;
};

class UiObjectLifecycleRuntimeStub : public IUiObjectLifecycleRuntime {
public:
    void LoadLayout(UiObjectState&, std::uint32_t) override {}
    void ConstructMember(UiObjectState&) override {}
    void InitializeElements(UiObjectState&) override {}
    void DestroyMember(UiObjectState&) override {}
    void DestroyLayout(UiObjectState&) override {}
    void FreeMemberBuffer(std::uint32_t) override {}
};

class UiObjectLifecycle final {
public:
    // IDA UIObject_Constructor at 0x4454a0.
    [[nodiscard]] static UiObjectState& Construct(
        UiObjectState& state, std::uint32_t layout,
        IUiObjectLifecycleRuntime& runtime);
    // IDA UIObject_Cleanup at 0x445520.
    static void CleanupMember(UiObjectState& state,
                              IUiObjectLifecycleRuntime& runtime);
};

}  // namespace pal4
