#pragma once

#include "pal4/script_builtin_registry.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace pal4 {

class ScriptWaitRegistration final {
public:
    static constexpr std::uint32_t kDescriptorAddress = 0x90f0c8u;
    static constexpr std::uint32_t kHandlerAddress = 0x5dfa30u;
    static constexpr std::uint32_t kCleanupCallback = 0x5d8d80u;

    explicit ScriptWaitRegistration(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d8c30, 0x5d8c40, 0x5d8d80.
    [[nodiscard]] int Initialize();
    [[nodiscard]] std::uint32_t Construct();
    void Cleanup();

    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor() const noexcept {
        return descriptor_;
    }

private:
    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor descriptor_{};
};

class ScriptTalkRegistration final {
public:
    static constexpr std::uint32_t kDescriptorAddress = 0x90f090u;
    static constexpr std::uint32_t kHandlerAddress = 0x5dfb10u;
    static constexpr std::uint32_t kCleanupCallback = 0x5d8ee0u;

    explicit ScriptTalkRegistration(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d8d90, 0x5d8da0, 0x5d8ee0.
    [[nodiscard]] int Initialize();
    [[nodiscard]] std::uint32_t Construct();
    void Cleanup();

    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor() const noexcept {
        return descriptor_;
    }

private:
    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor descriptor_{};
};

class ScriptRandTalkPushRegistration final {
public:
    static constexpr std::uint32_t kDescriptorAddress = 0x90f058u;
    static constexpr std::uint32_t kHandlerAddress = 0x5dfd30u;
    static constexpr std::uint32_t kCleanupCallback = 0x5d9040u;

    explicit ScriptRandTalkPushRegistration(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d8ef0, 0x5d8f00, 0x5d9040.
    [[nodiscard]] int Initialize();
    [[nodiscard]] std::uint32_t Construct();
    void Cleanup();

    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor() const noexcept {
        return descriptor_;
    }

private:
    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor descriptor_{};
};

class ScriptRandTalkRegistration final {
public:
    static constexpr std::uint32_t kDescriptorAddress = 0x90f020u;
    static constexpr std::uint32_t kHandlerAddress = 0x5dfee0u;
    static constexpr std::uint32_t kCleanupCallback = 0x5d91a0u;

    explicit ScriptRandTalkRegistration(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d9050, 0x5d9060, 0x5d91a0.
    [[nodiscard]] int Initialize();
    [[nodiscard]] std::uint32_t Construct();
    void Cleanup();

    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor() const noexcept {
        return descriptor_;
    }

private:
    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor descriptor_{};
};

class ScriptRandTalkReleaseRegistration final {
public:
    static constexpr std::uint32_t kDescriptorAddress = 0x90efe8u;
    static constexpr std::uint32_t kHandlerAddress = 0x5dffc0u;
    static constexpr std::uint32_t kCleanupCallback = 0x5d9300u;

    explicit ScriptRandTalkReleaseRegistration(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d91b0, 0x5d91c0, 0x5d9300.
    [[nodiscard]] int Initialize();
    [[nodiscard]] std::uint32_t Construct();
    void Cleanup();

    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor() const noexcept {
        return descriptor_;
    }

private:
    IScriptBuiltinRuntime& runtime_;
    ScriptBuiltinDescriptor descriptor_{};
};

struct ScriptDialogLateSpec {
    std::uint32_t registration_entry;
    std::uint32_t constructor_entry;
    std::uint32_t cleanup_entry;
    std::uint32_t descriptor_address;
    std::uint32_t handler_address;
    std::string_view description_with_nul;
    std::string_view signature_with_nul;
};

class ScriptDialogLateRegistry final {
public:
    static constexpr std::size_t kCount = 75u;

    explicit ScriptDialogLateRegistry(IScriptBuiltinRuntime& runtime)
        : runtime_(runtime) {}

    // IDA 0x5d9310..0x5dfa20: 75 consecutive registration triples.
    [[nodiscard]] static const ScriptDialogLateSpec& Describe(std::size_t index);
    [[nodiscard]] int Initialize(std::size_t index);
    [[nodiscard]] std::uint32_t Construct(std::size_t index);
    void Cleanup(std::size_t index);
    [[nodiscard]] const ScriptBuiltinDescriptor& Descriptor(
        std::size_t index) const;

private:
    IScriptBuiltinRuntime& runtime_;
    std::array<ScriptBuiltinDescriptor, kCount> descriptors_{};
};

}  // namespace pal4
