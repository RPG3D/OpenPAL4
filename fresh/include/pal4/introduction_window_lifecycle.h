#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct IntroductionWindowState {
    std::array<std::uint8_t, 0xb8> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IIntroductionWindowLifecycleRuntime {
public:
    virtual ~IIntroductionWindowLifecycleRuntime() = default;
    virtual void LoadLayout(IntroductionWindowState& state,
                            std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void ConstructMember(IntroductionWindowState& state,
                                 std::uint32_t member_offset) = 0;
    virtual void ConstructCollections(IntroductionWindowState& state,
                                      std::uint32_t first_offset,
                                      std::uint32_t element_size,
                                      std::uint32_t count) = 0;
    virtual void InitializePanelElements(IntroductionWindowState& state) = 0;
    virtual void InsertFirst(std::uint32_t collection_address,
                             std::uint32_t source,
                             std::uint32_t id) = 0;
    virtual void InsertNext(std::uint32_t collection_address,
                            std::uint32_t source, std::uint32_t enabled,
                            std::uint32_t id) = 0;
    virtual void DestroyCollections(IntroductionWindowState& state,
                                    std::uint32_t first_offset,
                                    std::uint32_t element_size,
                                    std::uint32_t count) = 0;
    virtual void CleanupMember(IntroductionWindowState& state,
                               std::uint32_t member_offset) = 0;
    virtual void FreeMemberStorage(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialog(
        IntroductionWindowState& state) = 0;
};

class IntroductionWindowLifecycleRuntimeStub :
    public IIntroductionWindowLifecycleRuntime {
public:
    void LoadLayout(IntroductionWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void ConstructMember(IntroductionWindowState&,
                         std::uint32_t) override {}
    void ConstructCollections(IntroductionWindowState&, std::uint32_t,
                              std::uint32_t, std::uint32_t) override {}
    void InitializePanelElements(IntroductionWindowState&) override {}
    void InsertFirst(std::uint32_t, std::uint32_t,
                     std::uint32_t) override {}
    void InsertNext(std::uint32_t, std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    void DestroyCollections(IntroductionWindowState&, std::uint32_t,
                            std::uint32_t, std::uint32_t) override {}
    void CleanupMember(IntroductionWindowState&, std::uint32_t) override {}
    void FreeMemberStorage(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyDialog(
        IntroductionWindowState&) override { return 0u; }
};

class IntroductionWindowLifecycle final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x84353cu;
    static constexpr std::uint32_t kMemberVtable = 0x843514u;
    static constexpr std::uint32_t kTailVtable = 0x843554u;

    [[nodiscard]] static IntroductionWindowState& Construct(
        IntroductionWindowState& state, std::uint32_t object_address,
        std::uint32_t name, IIntroductionWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        IntroductionWindowState& state,
        IIntroductionWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
