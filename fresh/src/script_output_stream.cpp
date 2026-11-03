#include "pal4/script_output_stream.h"

#include <fstream>
#include <ostream>

namespace pal4 {

std::unique_ptr<std::ostream> ScriptOutputStream::Construct(
    std::streambuf* buffer) {
    return std::make_unique<std::ostream>(buffer);
}

void ScriptOutputStream::Destroy(std::unique_ptr<std::ostream>& stream) {
    stream.reset();
}

void ScriptOutputStream::DestroyFile(std::unique_ptr<std::ofstream>& stream) {
    stream.reset();
}

bool ScriptOutputStream::CheckReady(std::ostream& stream) {
    const std::ostream::sentry guard(stream);
    return static_cast<bool>(guard);
}

std::ostream& ScriptOutputStream::InsertInteger(std::ostream& stream,
                                                std::int32_t value) {
    return stream << value;
}

std::ostream& ScriptOutputStream::InsertCString(std::ostream& stream,
                                                const char* text) {
    return stream << text;
}

std::uint32_t ScriptOutputTextRecord::Destroy(
    std::uint32_t object, std::uint8_t flags,
    IScriptOutputTextRuntime& runtime) {
    const auto data = runtime.ReadWord(object + 4u);
    if (data != 0u) {
        const auto header = data - 1u;
        const auto count = runtime.ReadByte(header);
        if (count == 0u || count == 0xffu) {
            runtime.Free(header);
        } else {
            runtime.WriteByte(header, static_cast<std::uint8_t>(count - 1u));
        }
    }
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 12u, 0u);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

}  // namespace pal4
