#include "pal4/combat_log_stream.h"
#include "pal4/combat_log_buffer.h"

#include <ostream>

namespace pal4 {

std::ostream& CombatLogStream::WriteEntry(
    std::ostream& stream, std::uint8_t character) {
    stream.put(static_cast<char>(character));
    return stream;
}

std::ostream& CombatLogStream::Flush(std::ostream& stream) {
    stream.flush();
    return stream;
}

std::ostream& CombatLogStream::WriteFormatted(
    std::ostream& stream, std::uint8_t character) {
    stream << static_cast<char>(character);
    return stream;
}

std::uint32_t CombatLogStream::GetString(
    std::uint32_t stream, std::uint32_t result,
    ICombatLogTextRuntime& runtime) {
    return CombatLogBuffer::GetString(stream + 4u, result, runtime);
}

std::uint32_t CombatLogStream::InitializeIos(
    std::uint32_t ios, std::uint32_t buffer,
    std::uint8_t standard_stream, ICombatLogIosRuntime& runtime) {
    runtime.WriteWord(ios + 40u, buffer);
    runtime.WriteWord(ios + 44u, 0u);
    runtime.WriteByte(ios + 48u, 32u);
    runtime.InitializeBase(ios);
    const auto connected = runtime.ReadWord(ios + 40u);
    if (connected == 0u)
        runtime.Clear(ios, runtime.ReadWord(ios + 4u) | 4u, false);
    const auto result = (connected & ~0xffu) | standard_stream;
    if (standard_stream != 0u) runtime.AddStandardStream(ios);
    return result;
}

}  // namespace pal4
