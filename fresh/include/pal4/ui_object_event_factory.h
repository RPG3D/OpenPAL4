#pragma once

#include <cstdint>

namespace pal4 {

class IUiObjectEventFactoryRuntime {
public:
    virtual ~IUiObjectEventFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CallDeletingDestructor(
        std::uint32_t address, std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool ForwardToFrameManager(
        std::uint32_t callback_member, std::uint32_t event_address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeStoredHandler(
        std::uint32_t handler, std::uint32_t event) = 0;
};

class UiObjectEventFactoryRuntimeStub : public IUiObjectEventFactoryRuntime {
public:
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void Free(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CallDeletingDestructor(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] bool ForwardToFrameManager(
        std::uint32_t, std::uint32_t) override { return false; }
    [[nodiscard]] std::uint32_t InvokeStoredHandler(
        std::uint32_t, std::uint32_t) override { return 0u; }
};

class UiObjectEventFactory final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x843500u;
    static constexpr std::uint32_t kDerivedVtable = 0x843494u;

    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source_address, IUiObjectEventFactoryRuntime& runtime);
};

class UiObjectEventCallback final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x843500u;
    static constexpr std::uint32_t kCallbackVtable = 0x8434f8u;
    static constexpr std::uint32_t kAlternateBaseVtable = 0x84350cu;
    static constexpr std::uint32_t kIntroductionVtable = 0x84356cu;
    static constexpr std::uint32_t kLoadBaseVtable = 0x8435d0u;
    static constexpr std::uint32_t kLoadCallbackVtable = 0x8435c4u;
    static constexpr std::uint32_t kFrameCallbackVtable = 0x8435dcu;
    static constexpr std::uint32_t kMainCallbackVtable = 0x843650u;

    [[nodiscard]] static std::uint32_t DeleteBase(
        std::uint32_t callback, std::uint32_t flags,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static bool ForwardEvent(
        std::uint32_t callback, std::uint32_t event,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t callback, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructIntroduction(
        std::uint32_t callback, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        IUiObjectEventFactoryRuntime& runtime);
    static void Destroy(std::uint32_t callback,
                        IUiObjectEventFactoryRuntime& runtime);
    static void ConstructAlternateBase(
        std::uint32_t callback, IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteAlternateBase(
        std::uint32_t callback, std::uint32_t flags,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReleaseOwned(
        std::uint32_t owner, IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneLoad(
        std::uint32_t source, IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteLoadBase(
        std::uint32_t callback, std::uint32_t flags,
        IUiObjectEventFactoryRuntime& runtime);
    static void DestroyLoad(std::uint32_t callback,
                            IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneFrame(
        std::uint32_t source, IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t CallStoredHandler(
        std::uint32_t callback, std::uint32_t event,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteFrame(
        std::uint32_t callback, std::uint32_t flags,
        IUiObjectEventFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructMain(
        std::uint32_t callback, std::uint32_t handler,
        std::uint32_t unused, std::uint32_t owner,
        IUiObjectEventFactoryRuntime& runtime);
};

}  // namespace pal4
