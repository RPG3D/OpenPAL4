#pragma once

#include <cstdint>

namespace pal4 {

struct ScriptFileAdapterState {
    std::uint32_t vtable{};
    std::uint32_t handle{};
};
static_assert(sizeof(ScriptFileAdapterState) == 8u);

class IScriptFileAdapterRuntime {
public:
    virtual ~IScriptFileAdapterRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetPackedScriptData() = 0;
    [[nodiscard]] virtual std::uint32_t GetPackedScriptLength(
        std::uint32_t script_data, std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t GetPosition(std::uint32_t handle) = 0;
    [[nodiscard]] virtual int Seek(std::uint32_t handle,
                                   std::int32_t offset,
                                   std::uint32_t origin) = 0;
    [[nodiscard]] virtual std::uint32_t Read(
        std::uint32_t buffer, std::uint32_t size,
        std::uint32_t count, std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t Write(
        std::uint32_t buffer, std::uint32_t size,
        std::uint32_t count, std::uint32_t handle) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class ScriptFileAdapterRuntimeStub final : public IScriptFileAdapterRuntime {
public:
    [[nodiscard]] std::uint32_t GetPackedScriptData() override { return 0u; }
    [[nodiscard]] std::uint32_t GetPackedScriptLength(
        std::uint32_t, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t GetPosition(std::uint32_t) override { return 0u; }
    [[nodiscard]] int Seek(std::uint32_t, std::int32_t,
                           std::uint32_t) override { return 0; }
    [[nodiscard]] std::uint32_t Read(std::uint32_t, std::uint32_t,
                                     std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] std::uint32_t Write(std::uint32_t, std::uint32_t,
                                      std::uint32_t, std::uint32_t) override {
        return 0u;
    }
    void Free(std::uint32_t) override {}
};

class ScriptFileAdapter final {
public:
    static constexpr std::uint32_t kVtable = 0x849f78u;
    static constexpr std::uint32_t kBaseVtable = 0x8426acu;

    explicit ScriptFileAdapter(IScriptFileAdapterRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5f7160.
    [[nodiscard]] static ScriptFileAdapterState& Construct(
        ScriptFileAdapterState& state, std::uint32_t handle);
    // IDA 0x5f7180.
    [[nodiscard]] std::uint32_t Destroy(ScriptFileAdapterState& state,
                                         std::uint32_t object,
                                         std::uint32_t flags);
    // IDA 0x5f71a0.
    static void ResetBase(ScriptFileAdapterState& state);
    // IDA 0x5f71b0.
    [[nodiscard]] std::uint32_t GetSize(const ScriptFileAdapterState& state);
    // IDA 0x5f7210.
    [[nodiscard]] std::uint32_t GetPosition(
        const ScriptFileAdapterState& state);
    // IDA 0x5f7220.
    [[nodiscard]] bool Read(const ScriptFileAdapterState& state,
                            std::uint32_t buffer, std::uint32_t bytes);
    // IDA 0x5f7250.
    [[nodiscard]] bool Write(const ScriptFileAdapterState& state,
                             std::uint32_t buffer, std::uint32_t bytes);

private:
    IScriptFileAdapterRuntime& runtime_;
};

}  // namespace pal4
