#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct PalScriptExecutorState final {
    std::array<std::uint32_t, 33u> words{};
};

static_assert(sizeof(PalScriptExecutorState) == 132);

struct PalScriptExecutorGlobals final {
    std::uint8_t singleton_guard = 0u;
    std::uint32_t singleton_address = 0u;
    std::uint32_t shared_record = 0u;
    std::uint32_t instance_count = 0u;
};

class IPalScriptTimerStateRuntime {
public:
    virtual ~IPalScriptTimerStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InitStringBuffer(
        PalScriptExecutorState& state, std::uint32_t offset,
        const char* text, std::uint32_t length) = 0;
};

class IPalScriptExecutorRuntime : public IPalScriptTimerStateRuntime {
public:
    ~IPalScriptExecutorRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t SingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterSingleton(
        std::uint32_t manager, const char* name, std::uint32_t id,
        std::uint32_t factory_callback,
        std::uint32_t destroy_callback) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual PalScriptExecutorState& StateAt(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void InitializeString(PalScriptExecutorState& state,
                                  std::uint32_t offset) = 0;
    virtual void InitializeContext(PalScriptExecutorState& state,
                                   std::uint32_t offset) = 0;
    virtual void DestroyContext(PalScriptExecutorState& state,
                                std::uint32_t offset) = 0;
    virtual void CleanupString(PalScriptExecutorState& state,
                               std::uint32_t offset) = 0;
    virtual void CleanupDataStructure(PalScriptExecutorState& state,
                                      std::uint32_t first,
                                      std::uint32_t last) = 0;
    virtual void TidyQueuedString(std::uint32_t address,
                              std::uint32_t value) = 0;
    [[nodiscard]] virtual bool AudioObjectExists(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t TreeWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t PreviousTreeNode(
        std::uint32_t node) = 0;
    virtual void InsertTreeNode(PalScriptExecutorState& state,
                                std::uint32_t leaf, std::uint32_t parent,
                                std::uint32_t key, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t LowerBound(
        PalScriptExecutorState& state, std::uint32_t key) = 0;
    virtual void PlayAudioObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveTreeNode(
        PalScriptExecutorState& state, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t NextTreeNode(
        std::uint32_t node) = 0;
    virtual void FreeTreeSubtree(PalScriptExecutorState& state,
                                 std::uint32_t root) = 0;

};

class PalScriptExecutorRuntimeStub final : public IPalScriptExecutorRuntime {
public:
    [[nodiscard]] std::uint32_t SingletonManager() override { return 0u; }
    [[nodiscard]] std::uint32_t RegisterSingleton(
        std::uint32_t, const char*, std::uint32_t,
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] PalScriptExecutorState& StateAt(
        std::uint32_t) override { return state_; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Free(std::uint32_t) override { return 0u; }
    void InitializeString(PalScriptExecutorState&, std::uint32_t) override {}
    void InitializeContext(PalScriptExecutorState&, std::uint32_t) override {}
    void DestroyContext(PalScriptExecutorState&, std::uint32_t) override {}
    void CleanupString(PalScriptExecutorState&, std::uint32_t) override {}
    void CleanupDataStructure(PalScriptExecutorState&, std::uint32_t,
                              std::uint32_t) override {}
    void TidyQueuedString(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] bool AudioObjectExists(std::uint32_t) override {
        return false;
    }
    [[nodiscard]] std::uint32_t TreeWord(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t PreviousTreeNode(
        std::uint32_t node) override { return node; }
    void InsertTreeNode(PalScriptExecutorState&, std::uint32_t,
                        std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    [[nodiscard]] std::uint32_t LowerBound(
        PalScriptExecutorState&, std::uint32_t) override { return 0u; }
    void PlayAudioObject(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t RemoveTreeNode(
        PalScriptExecutorState&, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t NextTreeNode(
        std::uint32_t node) override { return node; }
    void FreeTreeSubtree(PalScriptExecutorState&, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InitStringBuffer(
        PalScriptExecutorState&, std::uint32_t,
        const char*, std::uint32_t) override { return 0u; }

private:
    PalScriptExecutorState state_{};
};

class PalScriptExecutor final {
public:
    // IDA 0x600110.
    [[nodiscard]] static std::uint32_t GetInstance(
        PalScriptExecutorGlobals& globals,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x600150.
    [[nodiscard]] static std::uint32_t Create(
        PalScriptExecutorGlobals& globals, std::uint8_t native_stack_byte,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x6001b0.
    [[nodiscard]] static std::uint32_t DestroyInstance(
        std::uint32_t address, std::uint32_t undefined_null_return,
        PalScriptExecutorGlobals& globals,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x601740.
    [[nodiscard]] static std::uint32_t CleanupInstance(
        PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x6001d0.
    [[nodiscard]] static PalScriptExecutorState& Construct(
        PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
        std::uint8_t native_stack_byte,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x6002e0.
    [[nodiscard]] static std::uint32_t ResetState(
        PalScriptExecutorState& state, std::uint32_t state_address,
        PalScriptExecutorGlobals& globals,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x6008a0.
    [[nodiscard]] static std::uint32_t InsertData(
        PalScriptExecutorState& state, PalScriptExecutorGlobals& globals,
        std::uint32_t object, IPalScriptExecutorRuntime& runtime);
    // IDA 0x600970.
    [[nodiscard]] static std::uint32_t RemoveData(
        PalScriptExecutorState& state, std::uint32_t key,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x6009f0.
    [[nodiscard]] static std::uint32_t CleanupData(
        PalScriptExecutorState& state, std::uint32_t state_address,
        PalScriptExecutorGlobals& globals,
        IPalScriptExecutorRuntime& runtime);
    // IDA 0x600b10.
    [[nodiscard]] static std::uint32_t InitializeTimerState(
        PalScriptExecutorState& state, std::uint32_t delay,
        const char* callback, IPalScriptTimerStateRuntime& runtime);
};

}  // namespace pal4
