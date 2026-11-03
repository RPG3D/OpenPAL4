#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>

namespace pal4 {

class ScriptOutputStream final {
public:
    // IDA 0x56f1e0-0x56f790: the native entries are MSVC ostream support.
    [[nodiscard]] static std::unique_ptr<std::ostream> Construct(
        std::streambuf* buffer);
    static void Destroy(std::unique_ptr<std::ostream>& stream);
    static void DestroyFile(std::unique_ptr<std::ofstream>& stream);
    [[nodiscard]] static bool CheckReady(std::ostream& stream);
    [[nodiscard]] static std::ostream& InsertInteger(std::ostream& stream,
                                                     std::int32_t value);
    [[nodiscard]] static std::ostream& InsertCString(std::ostream& stream,
                                                     const char* text);
};

class IScriptOutputTextRuntime {
public:
    virtual ~IScriptOutputTextRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class ScriptOutputTextRecord final {
public:
    // IDA 0x56f560: three-word text cleanup and optional deleting destructor.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint8_t flags,
        IScriptOutputTextRuntime& runtime);
};

}  // namespace pal4
