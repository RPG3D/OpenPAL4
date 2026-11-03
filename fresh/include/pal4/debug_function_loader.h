#pragma once

#include <cstdint>

namespace pal4 {

struct DebugFunctionState {
    std::uint32_t initialized = 0u;
    std::uint32_t enum_process_modules = 0u;
    std::uint32_t get_module_base_name = 0u;
    std::uint32_t get_module_file_name = 0u;
};

class IDebugFunctionRuntime {
public:
    virtual ~IDebugFunctionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t LoadLibrary(const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t GetProcAddress(
        std::uint32_t module, const char* name) = 0;
};

class DebugFunctionLoader final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e0894u;
    static constexpr std::uint32_t kEnumAddress = 0x8e0898u;
    static constexpr std::uint32_t kBaseNameAddress = 0x8e089cu;
    static constexpr std::uint32_t kFileNameAddress = 0x8e08a0u;

    explicit DebugFunctionLoader(DebugFunctionState& state) : state_(state) {}
    [[nodiscard]] std::uint32_t Load(IDebugFunctionRuntime& runtime);

private:
    DebugFunctionState& state_;
};

}  // namespace pal4
