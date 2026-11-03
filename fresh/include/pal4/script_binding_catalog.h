#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pal4 {

enum class ScriptBindingNameKind {
    Literal,
    EnumBuffer,
    EnumLabel,
    EnumItem,
};

struct ScriptBindingName {
    ScriptBindingNameKind kind = ScriptBindingNameKind::Literal;
    std::uint32_t source = 0;
    std::uint32_t index = 0;
    std::uint32_t stride = 0;
    std::uint32_t count = 0;
    const char* literal = nullptr;
    const char* suffix = nullptr;
};

struct ScriptBindingSpec {
    std::string name;
    std::uint32_t object_size = 0;
    std::uint32_t object_vtable = 0;
    std::uint32_t object_constructor = 0;
    std::uint32_t constructor_source = 0;
    std::uint32_t constructor_count = 0;
    std::uint32_t getter_constructor = 0;
    std::uint32_t getter_callback = 0;
    std::uint32_t setter_constructor = 0;
    std::uint32_t setter_callback = 0;
    std::uint32_t callback_index = 0;
};

class IScriptBindingCatalogRuntime {
public:
    virtual ~IScriptBindingCatalogRuntime() = default;
    virtual void InitializeStorage(std::uint32_t address) = 0;
    virtual void RegisterCleanup(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::string ResolveName(
        const ScriptBindingName& name) = 0;
};

class ScriptBindingCatalog final {
public:
    static constexpr std::uint32_t kCollectionAddress = 0x8faa1cu;

    // IDA 0x5835b0.
    [[nodiscard]] const std::vector<ScriptBindingSpec>& Initialize(
        IScriptBindingCatalogRuntime& runtime);
    // IDA 0x585340.
    [[nodiscard]] const std::vector<ScriptBindingSpec>& InitializeForwarded(
        IScriptBindingCatalogRuntime& runtime);
    // IDA 0x5852b0, the CRT cleanup registered by Initialize.
    void Cleanup();
    [[nodiscard]] bool IsStorageInitialized() const { return storage_initialized_; }
    [[nodiscard]] bool IsPopulated() const { return populated_; }

private:
    bool storage_initialized_ = false;
    bool populated_ = false;
    std::vector<ScriptBindingSpec> entries_;
};

}  // namespace pal4
