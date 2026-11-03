#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace pal4 {

struct PalTestWindowState {
    std::array<std::uint8_t, 0x8c> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IPalTestWindowRuntime {
public:
    virtual ~IPalTestWindowRuntime() = default;
    virtual void LoadLayout(PalTestWindowState& state, std::uint32_t name,
                            std::uint32_t first, std::uint32_t second) = 0;
    virtual void ConstructMember(PalTestWindowState& state) = 0;
    virtual void InitializeControls(PalTestWindowState& state) = 0;
    virtual void CleanupNameBlock(PalTestWindowState& state) = 0;
    virtual void CleanupMember(PalTestWindowState& state) = 0;
    virtual void DestroyDialogBase(PalTestWindowState& state) = 0;
    virtual void Free(std::uint32_t pointer) = 0;
    virtual void SetParent(PalTestWindowState& state, std::uint8_t showing,
                           std::uint32_t parent) = 0;
    virtual void ResetUI(PalTestWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertActive(std::uint32_t array, std::uint32_t member) = 0;
    virtual void RemoveActive(std::uint32_t array, std::uint32_t member) = 0;
    virtual void SetCurrentQuestion(PalTestWindowState& state,
                                    std::uint32_t question) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t manager,
                                              std::uint32_t dialog) = 0;
};

class PalTestWindowRuntimeStub : public IPalTestWindowRuntime {
public:
    void LoadLayout(PalTestWindowState&, std::uint32_t, std::uint32_t,
                    std::uint32_t) override {}
    void ConstructMember(PalTestWindowState&) override {}
    void InitializeControls(PalTestWindowState&) override {}
    void CleanupNameBlock(PalTestWindowState&) override {}
    void CleanupMember(PalTestWindowState&) override {}
    void DestroyDialogBase(PalTestWindowState&) override {}
    void Free(std::uint32_t) override {}
    void SetParent(PalTestWindowState&, std::uint8_t,
                   std::uint32_t) override {}
    void ResetUI(PalTestWindowState&) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertActive(std::uint32_t, std::uint32_t) override {}
    void RemoveActive(std::uint32_t, std::uint32_t) override {}
    void SetCurrentQuestion(PalTestWindowState&, std::uint32_t) override {}
    void UpdateSystemDialogVisibility(std::uint32_t,
                                      std::uint32_t) override {}
};

class PalTestWindow final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x8437ecu;
    static constexpr std::uint32_t kMemberVtable = 0x8437c4u;
    static constexpr std::uint32_t kCallbackVtable = 0x843804u;

    [[nodiscard]] static PalTestWindowState& Construct(
        PalTestWindowState& state, std::uint32_t name,
        IPalTestWindowRuntime& runtime);
    static void Destroy(PalTestWindowState& state,
                        IPalTestWindowRuntime& runtime);
    static void SetVisible(PalTestWindowState& state,
                           std::uint32_t object_address,
                           std::uint8_t showing, std::uint32_t parent,
                           IPalTestWindowRuntime& runtime);
};

}  // namespace pal4
