#pragma once

#include <cstdint>

namespace pal4 {

class IScriptAttributeTreeRuntime {
public:
    virtual ~IScriptAttributeTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t NilAddress() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CompareNodeKey(
        std::uint32_t node_key, std::uint32_t query) = 0;
    [[nodiscard]] virtual std::int32_t CompareBytes(
        std::uint32_t query_data, std::uint32_t node_data,
        std::uint32_t length) = 0;
};

class ScriptAttributeTreeRuntimeStub final
    : public IScriptAttributeTreeRuntime {
public:
    std::uint32_t NilAddress() override;
    std::uint32_t EmptyStringAddress() override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    std::int32_t CompareNodeKey(
        std::uint32_t node_key, std::uint32_t query) override;
    std::int32_t CompareBytes(
        std::uint32_t query_data, std::uint32_t node_data,
        std::uint32_t length) override;
};

class ScriptAttributeTreeLookup final {
public:
    static std::uint32_t& Find(
        std::uint32_t tree, std::uint32_t& output,
        std::uint32_t query, IScriptAttributeTreeRuntime& runtime);
};

}  // namespace pal4
