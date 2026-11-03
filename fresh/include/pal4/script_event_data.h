#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ScriptEventDataState {
    std::array<std::uint32_t, 10> words{};
};
static_assert(sizeof(ScriptEventDataState) == 40u);

class IScriptEventDataRuntime {
public:
    virtual ~IScriptEventDataRuntime() = default;
    virtual void DestroyXmlNode(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class ScriptEventDataRuntimeStub final : public IScriptEventDataRuntime {
public:
    void DestroyXmlNode(std::uint32_t address) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
};

class IScriptEventStringRuntime {
public:
    virtual ~IScriptEventStringRuntime() = default;
    [[noreturn]] virtual void ThrowLength() = 0;
    [[nodiscard]] virtual std::uint8_t SetErrorFlag(
        ScriptEventDataState& state, std::uint8_t flag) = 0;
    virtual void Reallocate(ScriptEventDataState& state,
                            std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Copy(std::uint32_t destination, const char* source,
                      std::uint32_t length) = 0;
};

class ScriptEventStringRuntimeStub final : public IScriptEventStringRuntime {
public:
    [[noreturn]] void ThrowLength() override;
    std::uint8_t SetErrorFlag(ScriptEventDataState& state,
                              std::uint8_t flag) override;
    void Reallocate(ScriptEventDataState& state,
                    std::uint32_t length) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void Copy(std::uint32_t destination, const char* source,
              std::uint32_t length) override;
};

class IScriptEventDocumentRuntime {
public:
    virtual ~IScriptEventDocumentRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeDocument(
        std::uint32_t address, const char* name, std::uint32_t option) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptEventDocumentRuntimeStub final
    : public IScriptEventDocumentRuntime {
public:
    std::uint32_t Allocate(std::uint32_t size) override;
    std::uint32_t InitializeDocument(
        std::uint32_t address, const char* name,
        std::uint32_t option) override;
    void Free(std::uint32_t address) override;
};

class IScriptEventCompareRuntime {
public:
    virtual ~IScriptEventCompareRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
};

class ScriptEventCompareRuntimeStub final
    : public IScriptEventCompareRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
};

class ScriptEventData final {
public:
    static ScriptEventDataState& Initialize(ScriptEventDataState& state,
                                             std::uint32_t object_address);
    [[nodiscard]] static std::uint32_t Cleanup(
        ScriptEventDataState& state, IScriptEventDataRuntime& runtime);
    [[nodiscard]] static std::uint8_t AssignCString(
        ScriptEventDataState& state, const char* source,
        IScriptEventStringRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetOrCreateConditionDocument(
        ScriptEventDataState& state, IScriptEventDocumentRuntime& runtime);
    [[nodiscard]] static std::int32_t CompareCString(
        const ScriptEventDataState& state, const char* text,
        IScriptEventCompareRuntime& runtime);
};

}  // namespace pal4
