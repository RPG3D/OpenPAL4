#include "pal4/effect_script_vm.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEffectScriptVmRuntime {
    std::vector<std::string> events;
    std::uint32_t open_result = 0x30u;
    std::uint32_t load_result = 0u;
    std::uint32_t error_result = 0u;
    std::uint32_t next_vm = 0x90u;
    std::array<std::uint32_t, 7> load_arguments{};
    std::string path;
    std::string mode;
    std::string shown_text;
    std::string shown_caption;
    std::uint32_t shown_flags = 0xffffffffu;
    std::uint32_t last_slot = 0u;

    std::uint32_t CurrentDevice() override {
        events.push_back("device");
        return 0x20u;
    }
    std::uint32_t OpenFile(const std::string_view source,
                           const std::string_view open_mode) override {
        events.push_back("open");
        path = source;
        mode = open_mode;
        return open_result;
    }
    std::uint32_t ScriptSize(const std::uint32_t file) override {
        Check(file == 0x30u, "script size receives opened file");
        events.push_back("size");
        return 43u;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        Check(bytes == 43u, "allocation uses file script size");
        events.push_back("allocate");
        return 0x40u;
    }
    void ReadFile(const std::uint32_t buffer, const std::uint32_t bytes,
                  const std::uint32_t count,
                  const std::uint32_t file) override {
        Check(buffer == 0x40u && bytes == 43u && count == 1u &&
                  file == 0x30u, "read uses exact IDA arguments");
        events.push_back("read");
    }
    void CloseFile(const std::uint32_t file) override {
        Check(file == 0x30u, "close receives opened file");
        events.push_back("close");
    }
    std::uint32_t LoadAndExecute(
        const std::uint32_t device, const std::uint32_t buffer,
        const std::uint32_t bytes, const std::uint32_t compile_flags,
        const std::uint32_t compile_options,
        const std::uint32_t configuration,
        const std::uint32_t execution_flags, std::uint32_t& output_vm,
        std::uint32_t& error) override {
        events.push_back("execute");
        load_arguments = {device, buffer, bytes, compile_flags,
                          compile_options, configuration, execution_flags};
        output_vm = next_vm;
        error = error_result;
        return load_result;
    }
    std::string_view ErrorText(const std::uint32_t error) override {
        Check(error == error_result, "error text receives returned error");
        events.push_back("error-text");
        return "bad effect";
    }
    void ShowError(const std::string_view text,
                   const std::string_view caption,
                   const std::uint32_t flags) override {
        events.push_back("show-error");
        shown_text = text;
        shown_caption = caption;
        shown_flags = flags;
    }
    void Free(const std::uint32_t buffer) override {
        Check(buffer == 0x40u, "free receives script allocation");
        events.push_back("free");
    }
    void ReleaseVm(const std::uint32_t vm) override {
        events.push_back("release:" + std::to_string(vm));
    }
    std::uint32_t InvokeVmSlot(const std::uint32_t vm,
                               const std::uint32_t byte_offset) override {
        Check(vm == next_vm, "VM getter uses output instance");
        events.push_back("slot");
        last_slot = byte_offset;
        return vm + byte_offset;
    }
};

void OpenFailure() {
    Runtime runtime;
    runtime.open_result = 0u;
    pal4::EffectScriptVm vm;
    Check(vm.ExecuteFile("effects/missing.fx", runtime) == 0u &&
              vm.Vm() == 0u &&
              runtime.events ==
                  std::vector<std::string>{"device", "open"} &&
              runtime.path == "effects/missing.fx" && runtime.mode == "r",
          "open failure stops after release, device and read-mode open");
    Check(vm.Release(runtime) == 1u && runtime.events.size() == 2u,
          "release is a no-op without a VM");
    Check(vm.GetValue284(runtime) == 0u && vm.GetValue288(runtime) == 0u &&
              runtime.events.size() == 2u,
          "VM getters return zero without dispatch");
}

void SuccessAndReplacement() {
    Runtime runtime;
    pal4::EffectScriptVm vm;
    Check(vm.ExecuteFile("effects/water.fx", runtime) == 1u &&
              vm.Vm() == 0x90u &&
              runtime.load_arguments ==
                  (std::array<std::uint32_t, 7>{0x20u, 0x40u, 43u, 0u,
                                                0u, 260u, 0u}) &&
              runtime.events ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "free"}),
          "successful load preserves file and VM call order and flags");
    Check(vm.GetValue284(runtime) == 0x90u + 284u &&
              runtime.last_slot == 284u &&
              vm.GetValue288(runtime) == 0x90u + 288u &&
              runtime.last_slot == 288u,
          "VM getters dispatch distinct virtual slots");
    runtime.events.clear();
    runtime.open_result = 0u;
    Check(vm.ExecuteFile("effects/absent.fx", runtime) == 0u &&
              vm.Vm() == 0u &&
              runtime.events ==
                  (std::vector<std::string>{"release:144", "device", "open"}),
          "replacement releases old VM before opening new file");
}

void LoadFailures() {
    Runtime runtime;
    runtime.load_result = 1u;
    runtime.error_result = 0x60u;
    runtime.next_vm = 0u;
    pal4::EffectScriptVm vm;
    Check(vm.ExecuteFile("effects/bad.fx", runtime) == 0u &&
              runtime.shown_text == "bad effect" &&
              runtime.shown_caption == "error" &&
              runtime.shown_flags == 0u &&
              runtime.events ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "error-text",
                                            "show-error", "free"}),
          "execution error reports text and frees buffer");
    runtime.events.clear();
    runtime.error_result = 0u;
    Check(vm.ExecuteFile("effects/no-detail.fx", runtime) == 0u &&
              runtime.events ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "free"}),
          "failure without error object omits message box");
    pal4::EffectScriptVmRuntimeStub stub;
    Check(vm.Release(stub) == 1u && vm.ExecuteFile("", stub) == 0u,
          "native service stub is linkable");
}

}  // namespace

int main() {
    OpenFailure();
    SuccessAndReplacement();
    LoadFailures();
}
