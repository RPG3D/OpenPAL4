#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ScriptWaitHandlerState {
    std::uint32_t vtable_address{};
    std::array<std::byte, 28> base_state{};
    double remaining{};
};
static_assert(offsetof(ScriptWaitHandlerState, remaining) == 0x20u);
static_assert(sizeof(ScriptWaitHandlerState) == 0x28u);

class IScriptWaitHandlerRuntime {
public:
    virtual ~IScriptWaitHandlerRuntime() = default;
    [[nodiscard]] virtual ScriptWaitHandlerState* Allocate(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void ConstructBase(ScriptWaitHandlerState& handler,
                               std::uint32_t wait_mode,
                               std::uint32_t queue_address) = 0;
    virtual void Flush(ScriptWaitHandlerState& handler) = 0;
    virtual void AddHandler(std::uint32_t queue_address,
                            ScriptWaitHandlerState* handler) = 0;
    [[nodiscard]] virtual int SetExecutionFlag(
        std::uint32_t queue_address) = 0;
    virtual void SetClosedFlag(ScriptWaitHandlerState& handler) = 0;
};

class ScriptWaitHandlerRuntimeStub final : public IScriptWaitHandlerRuntime {
public:
    ScriptWaitHandlerState* Allocate(std::uint32_t) override;
    std::uint32_t ScriptExecutor() override;
    void ConstructBase(ScriptWaitHandlerState&, std::uint32_t,
                       std::uint32_t) override;
    void Flush(ScriptWaitHandlerState&) override;
    void AddHandler(std::uint32_t, ScriptWaitHandlerState*) override;
    int SetExecutionFlag(std::uint32_t) override;
    void SetClosedFlag(ScriptWaitHandlerState&) override;
};

class ScriptWaitHandler final {
public:
    static constexpr std::uint32_t kSize = 0x28u;
    static constexpr std::uint32_t kVtable = 0x849514u;
    static constexpr std::uint32_t kQueueOffset = 0x54u;

    explicit ScriptWaitHandler(IScriptWaitHandlerRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5dfa30, 0x5dfad0 and 0x5dfb00.
    [[nodiscard]] int Create(double duration);
    void Update(ScriptWaitHandlerState& handler, double elapsed);
    void Close(ScriptWaitHandlerState& handler);

private:
    IScriptWaitHandlerRuntime& runtime_;
};

}  // namespace pal4
