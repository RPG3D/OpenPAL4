#include "pal4/matfx_script_vm.h"
#include "pal4/world_manager_script_thunks.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IEffectScriptVmRuntime {
    std::vector<std::string> calls;
    std::uint32_t file = 0x30u;
    std::uint32_t execute_result = 0u;
    std::uint32_t error = 0u;
    std::uint32_t next_vm = 0x90u;
    std::uint32_t last_slot = 0u;
    std::array<std::uint32_t, 7> execution_args{};
    std::string opened_path;
    std::string shown_text;

    std::uint32_t CurrentDevice() override {
        calls.emplace_back("device");
        return 0x20u;
    }
    std::uint32_t OpenFile(std::string_view path,
                           std::string_view mode) override {
        Check(mode == "r", "file mode");
        opened_path = path;
        calls.emplace_back("open");
        return file;
    }
    std::uint32_t ScriptSize(std::uint32_t opened) override {
        Check(opened == file, "size file");
        calls.emplace_back("size");
        return 17u;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 17u, "allocated size");
        calls.emplace_back("allocate");
        return 0x40u;
    }
    void ReadFile(std::uint32_t buffer, std::uint32_t size,
                  std::uint32_t count, std::uint32_t opened) override {
        Check(buffer == 0x40u && size == 17u && count == 1u &&
                  opened == file, "read arguments");
        calls.emplace_back("read");
    }
    void CloseFile(std::uint32_t opened) override {
        Check(opened == file, "close file");
        calls.emplace_back("close");
    }
    std::uint32_t LoadAndExecute(
        std::uint32_t device, std::uint32_t buffer, std::uint32_t size,
        std::uint32_t compile_flags, std::uint32_t compile_options,
        std::uint32_t configuration, std::uint32_t execution_flags,
        std::uint32_t& output_vm, std::uint32_t& output_error) override {
        calls.emplace_back("execute");
        execution_args = {device, buffer, size, compile_flags,
                          compile_options, configuration, execution_flags};
        output_vm = next_vm;
        output_error = error;
        return execute_result;
    }
    std::string_view ErrorText(std::uint32_t value) override {
        Check(value == error, "error object");
        calls.emplace_back("error-text");
        return "matfx error";
    }
    void ShowError(std::string_view text, std::string_view caption,
                   std::uint32_t flags) override {
        Check(caption == "error" && flags == 0u, "error message options");
        shown_text = text;
        calls.emplace_back("message");
    }
    void Free(std::uint32_t buffer) override {
        Check(buffer == 0x40u, "freed buffer");
        calls.emplace_back("free");
    }
    void ReleaseVm(std::uint32_t vm) override {
        calls.emplace_back("release:" + std::to_string(vm));
    }
    std::uint32_t InvokeVmSlot(std::uint32_t vm,
                               std::uint32_t slot) override {
        Check(vm == next_vm, "getter VM");
        last_slot = slot;
        calls.emplace_back("slot");
        return vm + slot;
    }
};

void OpenFailureAndEmptyRelease() {
    Runtime runtime;
    runtime.file = 0u;
    pal4::MatFxScriptVm vm;
    Check(vm.ExecuteFile("missing.fx", runtime) == 0u &&
              runtime.opened_path == "missing.fx" &&
              runtime.calls == std::vector<std::string>{"device", "open"},
          "open failure stops before allocation");
    Check(vm.Release(runtime) == 1u &&
              vm.GetState(runtime) == 0u &&
              vm.GetProperty288(runtime) == 0u &&
              runtime.calls.size() == 2u,
          "empty release and getters do not dispatch");
}

void SuccessAndIndependentVmState() {
    Runtime runtime;
    pal4::MatFxScriptVm matfx;
    pal4::EffectScriptVm effect;
    Check(matfx.ExecuteFile("matfx.fx", runtime) == 1u &&
              matfx.Vm() == 0x90u && effect.Vm() == 0u &&
              runtime.execution_args ==
                  (std::array<std::uint32_t, 7>{0x20u, 0x40u, 17u, 0u,
                                                0u, 260u, 0u}) &&
              runtime.calls ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "free"}),
          "MatFx load uses its own VM and exact loader arguments");
    Check(matfx.GetState(runtime) == 0x90u + 284u &&
              runtime.last_slot == 284u &&
              matfx.GetProperty288(runtime) == 0x90u + 288u &&
              runtime.last_slot == 288u,
          "getters call distinct VM slots");
    runtime.calls.clear();
    runtime.file = 0u;
    Check(matfx.ExecuteFile("gone.fx", runtime) == 0u &&
              matfx.Vm() == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"release:144", "device", "open"}),
          "reload releases prior VM before device and open");
}

void ExecutionFailures() {
    Runtime runtime;
    runtime.execute_result = 7u;
    runtime.error = 0x70u;
    runtime.next_vm = 0u;
    pal4::MatFxScriptVm vm;
    Check(vm.ExecuteFile("bad.fx", runtime) == 0u &&
              runtime.shown_text == "matfx error" &&
              runtime.calls ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "error-text",
                                            "message", "free"}),
          "failure with error reports before buffer release");
    runtime.calls.clear();
    runtime.error = 0u;
    Check(vm.ExecuteFile("no-detail.fx", runtime) == 0u &&
              runtime.calls ==
                  (std::vector<std::string>{"device", "open", "size",
                                            "allocate", "read", "close",
                                            "execute", "free"}),
          "failure without error skips message and frees buffer");
    Check(vm.Release(runtime) == 1u, "cleanup always returns one");
}

void WorldManagerGetterThunks() {
    Runtime runtime;
    pal4::MatFxScriptVm vm;
    Check(pal4::WorldManagerScriptThunks::GetState(vm, runtime) == 0u &&
              pal4::WorldManagerScriptThunks::GetProperty288(vm, runtime) ==
                  0u && runtime.calls.empty(),
          "empty VM thunks forward the getter's zero result");
    Check(vm.ExecuteFile("world.fx", runtime) == 1u,
          "load VM for forwarding test");
    runtime.calls.clear();
    Check(pal4::WorldManagerScriptThunks::GetState(vm, runtime) ==
              0x90u + 284u && runtime.last_slot == 284u,
          "state thunk targets VM slot 284");
    Check(pal4::WorldManagerScriptThunks::GetProperty288(vm, runtime) ==
              0x90u + 288u && runtime.last_slot == 288u &&
              runtime.calls == (std::vector<std::string>{"slot", "slot"}),
          "property thunk targets distinct VM slot 288");
}

}  // namespace

int main() {
    OpenFailureAndEmptyRelease();
    SuccessAndIndependentVmState();
    ExecutionFailures();
    WorldManagerGetterThunks();
}
