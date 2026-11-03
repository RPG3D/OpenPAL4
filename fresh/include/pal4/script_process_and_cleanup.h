#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IScriptProcessRuntime {
public:
    virtual ~IScriptProcessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::int32_t CompareCaseInsensitive(
        std::uint32_t left, std::uint32_t right) = 0;
    virtual void CopyName(char* destination, std::uint32_t source,
                          std::uint32_t count) = 0;
    virtual void ResetObject(std::uint32_t object) = 0;
    virtual void AddAndProcessScript() = 0;
    virtual void InitializeObject(std::uint32_t object,
                                  const char* name,
                                  std::uint32_t script_name,
                                  std::uint32_t mode) = 0;
    virtual void CleanupAllScripts() = 0;
    virtual void ApplyFirst(std::uint32_t object,
                            const std::array<std::uint32_t, 3>& values,
                            std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::int32_t ApplySecond(
        std::uint32_t object,
        const std::array<std::uint32_t, 3>& values,
        std::uint32_t mode) = 0;
};

class ScriptProcessRuntimeStub : public IScriptProcessRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    std::uint32_t EmptyStringAddress() override;
    std::int32_t CompareCaseInsensitive(std::uint32_t,
                                        std::uint32_t) override;
    void CopyName(char*, std::uint32_t, std::uint32_t) override;
    void ResetObject(std::uint32_t) override;
    void AddAndProcessScript() override;
    void InitializeObject(std::uint32_t, const char*,
                          std::uint32_t, std::uint32_t) override;
    void CleanupAllScripts() override;
    void ApplyFirst(std::uint32_t,
                    const std::array<std::uint32_t, 3>&,
                    std::uint32_t) override;
    std::int32_t ApplySecond(
        std::uint32_t, const std::array<std::uint32_t, 3>&,
        std::uint32_t) override;
};

class ScriptProcessAndCleanup final {
public:
    // IDA 0x4400e0.
    [[nodiscard]] static std::int32_t Process(
        std::uint32_t object, IScriptProcessRuntime& runtime);
    // IDA 0x4401d0.
    [[nodiscard]] static bool NamesDiffer(
        std::uint32_t object, IScriptProcessRuntime& runtime);
};

}  // namespace pal4
