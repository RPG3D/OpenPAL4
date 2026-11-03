#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ISkillParameterTextRuntime {
public:
    virtual ~ISkillParameterTextRuntime() = default;
    virtual void InitializeText(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool CopyLiteral(std::uint32_t address,
                                           std::string_view text) = 0;
    [[nodiscard]] virtual int SetExceptionMessage(std::uint32_t address,
                                                  std::string_view text) = 0;
    virtual void DestroyText(std::uint32_t address) = 0;
    [[nodiscard]] virtual int RegisterCleanup() = 0;
};

class SkillParameterTextRegistry final {
public:
    static constexpr std::uint32_t kFirstAddress = 0x8ef248;
    static constexpr std::uint32_t kTextBytes = 16;
    static constexpr std::uint32_t kCount = 9;

    // IDA 0x4f2a90: initialize the nine strings, then register their cleanup.
    [[nodiscard]] static int InitializeAndRegisterCleanup(
        ISkillParameterTextRuntime& runtime);
    // IDA 0x4f2aa0: first six strings use allocation/copy; final three use
    // SetExceptionMessage and forward its final return value.
    [[nodiscard]] static int Initialize(ISkillParameterTextRuntime& runtime);
    // IDA 0x4f2d90: destroy nine 16-byte string objects in reverse order.
    static void Destroy(ISkillParameterTextRuntime& runtime);
};

}  // namespace pal4
