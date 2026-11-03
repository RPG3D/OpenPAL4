#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IEffectScriptVmRuntime {
public:
    virtual ~IEffectScriptVmRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CurrentDevice() = 0;
    [[nodiscard]] virtual std::uint32_t OpenFile(std::string_view path,
                                                  std::string_view mode) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptSize(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void ReadFile(std::uint32_t buffer, std::uint32_t bytes,
                          std::uint32_t count, std::uint32_t file) = 0;
    virtual void CloseFile(std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t LoadAndExecute(
        std::uint32_t device, std::uint32_t buffer, std::uint32_t bytes,
        std::uint32_t compile_flags, std::uint32_t compile_options,
        std::uint32_t configuration, std::uint32_t execution_flags,
        std::uint32_t& output_vm, std::uint32_t& error) = 0;
    [[nodiscard]] virtual std::string_view ErrorText(std::uint32_t error) = 0;
    virtual void ShowError(std::string_view text, std::string_view caption,
                           std::uint32_t flags) = 0;
    virtual void Free(std::uint32_t buffer) = 0;
    virtual void ReleaseVm(std::uint32_t vm) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVmSlot(
        std::uint32_t vm, std::uint32_t byte_offset) = 0;
};

class EffectScriptVmRuntimeStub final : public IEffectScriptVmRuntime {
public:
    [[nodiscard]] std::uint32_t CurrentDevice() override { return 0u; }
    [[nodiscard]] std::uint32_t OpenFile(std::string_view,
                                          std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t ScriptSize(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    void ReadFile(std::uint32_t, std::uint32_t, std::uint32_t,
                  std::uint32_t) override {}
    void CloseFile(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t LoadAndExecute(
        std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t&,
        std::uint32_t&) override { return 1u; }
    [[nodiscard]] std::string_view ErrorText(std::uint32_t) override {
        return {};
    }
    void ShowError(std::string_view, std::string_view,
                   std::uint32_t) override {}
    void Free(std::uint32_t) override {}
    void ReleaseVm(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t InvokeVmSlot(std::uint32_t,
                                              std::uint32_t) override {
        return 0u;
    }
};

class EffectScriptVm final {
public:
    // IDA 0x51d510, outputVMInstance is represented by vm_.
    [[nodiscard]] std::uint32_t ExecuteFile(
        std::string_view path, IEffectScriptVmRuntime& runtime);
    // IDA 0x51d5d0.
    [[nodiscard]] std::uint32_t Release(IEffectScriptVmRuntime& runtime);
    // IDA 0x51d5f0 and 0x51d610.
    [[nodiscard]] std::uint32_t GetValue284(
        IEffectScriptVmRuntime& runtime) const;
    [[nodiscard]] std::uint32_t GetValue288(
        IEffectScriptVmRuntime& runtime) const;
    [[nodiscard]] std::uint32_t Vm() const { return vm_; }

private:
    std::uint32_t vm_{};
};

}  // namespace pal4
