#include "pal4/combat_event_logger.h"
#include "pal4/combat_event_log_entry.h"
#include "pal4/combat_log_buffer.h"

#include <array>
#include <cstdarg>
#include <stdexcept>

namespace pal4 {

std::uint32_t CombatEventLogger::Construct(
    std::uint32_t logger, ICombatEventLoggerRuntime& runtime) {
    constexpr std::uint32_t kStdIosVtable = 0x8682d0u;
    constexpr std::uint32_t kStdOstreamVtable = 0x8682c8u;
    constexpr std::array<std::uint32_t, 7> kMessages{
        0x8b8f40u, 0x8b8ef0u, 0x8b8ea0u, 0x8b8e50u,
        0x8b8e00u, 0x8b8da8u, 0x8b8d88u};

    runtime.WriteWord(logger, 1u);
    runtime.WriteWord(logger + 4u, 0x8480d0u);
    runtime.WriteWord(logger + 128u, 0u);
    runtime.WriteWord(logger + 92u, kStdIosVtable);
    runtime.WriteWord(logger + 92u, kStdOstreamVtable);
    runtime.InitializeIos(logger + 92u, logger + 8u, 0u);
    runtime.ConstructFileBuffer(logger + 8u, 0u);
    runtime.WriteWord(logger + 92u, 0x8480ccu);

    runtime.WriteByte(logger + 144u, runtime.OpaqueStackByte());
    runtime.WriteWord(logger + 148u, 0u);
    runtime.WriteWord(logger + 152u, 0u);
    runtime.WriteWord(logger + 156u, 0u);
    runtime.WriteWord(logger + 160u, 0x848180u);
    runtime.WriteWord(logger + 276u, 0u);
    runtime.WriteWord(logger + 240u, kStdIosVtable);
    runtime.WriteWord(logger + 240u, kStdOstreamVtable);
    runtime.InitializeIos(logger + 240u, logger + 164u, 0u);
    runtime.InitializeStreamBuffer(logger + 164u);
    runtime.WriteWord(logger + 164u, 0x84814cu);
    const auto flags = CombatLogBuffer::ConvertFlags(2u);
    runtime.InitializeExtendedBuffer(logger + 164u, 0u, 0u, flags);
    runtime.WriteWord(logger + 240u, 0x848148u);
    runtime.WriteByte(logger + 292u, 1u);

    for (const auto message : kMessages)
        runtime.LogEvent(logger, 1u, message, 0u);
    return logger;
}

void CombatEventLogger::Cleanup(
    std::uint32_t logger, ICombatEventLoggerRuntime& runtime) {
    constexpr std::uint32_t kStdIosVtable = 0x8682d0u;
    constexpr std::uint32_t kStdOstreamVtable = 0x8682c8u;
    constexpr std::uint32_t kStdFilebufVtable = 0x8682d8u;

    const auto handle = runtime.ReadWord(logger + 88u);
    if (handle != 0u) {
        if (runtime.CloseFile(handle) == 0) {
            runtime.InitializeFileBuffer(logger + 8u, 0u, 2u);
        } else {
            const auto offset = runtime.ReadWord(
                runtime.ReadWord(logger + 4u) + 4u);
            const auto ios = logger + 4u + offset;
            auto state = runtime.ReadWord(ios + 4u) | 2u;
            if (runtime.ReadWord(ios + 40u) == 0u) state |= 4u;
            runtime.ClearIos(ios, state, false);
        }
    }

    const auto secondary_offset = runtime.ReadWord(
        runtime.ReadWord(logger + 160u) + 4u);
    runtime.WriteWord(logger + 160u + secondary_offset, 0x848148u);
    runtime.WriteWord(logger + 164u, 0x84814cu);
    runtime.CleanupBuffer(logger + 164u);
    runtime.DestroyStreamBuffer(logger + 164u);
    const auto secondary_base = logger + 160u;
    runtime.WriteWord(secondary_base + runtime.ReadWord(
        runtime.ReadWord(secondary_base) + 4u), kStdOstreamVtable);
    runtime.WriteWord(logger + 240u, kStdIosVtable);
    runtime.DestroyIos(logger + 240u);

    const auto end = runtime.ReadWord(logger + 152u);
    const auto begin = runtime.ReadWord(logger + 148u);
    for (auto item = begin; item != end; item += 20u)
        runtime.SetErrorFlag(item, 1u);
    runtime.Free(runtime.ReadWord(logger + 148u));
    runtime.WriteWord(logger + 148u, 0u);
    runtime.WriteWord(logger + 152u, 0u);
    runtime.WriteWord(logger + 156u, 0u);

    const auto primary_offset = runtime.ReadWord(
        runtime.ReadWord(logger + 4u) + 4u);
    runtime.WriteWord(logger + 4u + primary_offset, 0x8480ccu);
    runtime.WriteWord(logger + 8u, kStdFilebufVtable);
    if (runtime.ReadByte(logger + 80u) != 0u)
        runtime.CloseFileBuffer(logger + 8u);
    const auto text = runtime.ReadWord(logger + 76u);
    if (text != 0u) runtime.DestroyTextRecord(text, 1u);
    runtime.DestroyLocale(logger + 84u);
    runtime.DestroyStreamBuffer(logger + 8u);
    runtime.WriteWord(logger + 4u + runtime.ReadWord(
        runtime.ReadWord(logger + 4u) + 4u), kStdOstreamVtable);
    runtime.WriteWord(logger + 92u, kStdIosVtable);
    runtime.DestroyIos(logger + 92u);
}

std::uint32_t CombatEventLogger::Log(
    std::uint32_t logger, std::uint32_t level, std::uint32_t message,
    ICombatEventLogRuntime& runtime, const char* format, ...) {
    const auto threshold = runtime.ReadWord(logger);
    if (threshold < level) return threshold;

    if (format != nullptr) {
        std::va_list arguments;
        va_start(arguments, format);
        runtime.FormatMessage(logger + 293u, format, arguments);
        va_end(arguments);
    }
    const auto local_time = runtime.LocalTime();
    if (!local_time) return 0u;

    runtime.ResetBuffer(logger + 164u);
    const auto message_text=runtime.ReadCString(message);
    const auto detail=format ? std::optional<std::string>{runtime.ReadCString(logger+293u)} : std::nullopt;
    const auto text=FormatCombatLogEntry(*local_time,level,message_text,
        detail ? std::optional<std::string_view>{*detail} : std::nullopt);
    runtime.WriteBuffer(logger + 164u, text);
    runtime.FlushBuffer(logger + 164u);
    if (runtime.ReadByte(logger + 292u) != 0u)
        return runtime.RecordEvent(logger, runtime.ReadWord(logger + 152u),
                                   level, text);
    runtime.WritePrimary(logger + 4u, text);
    return runtime.FlushPrimary(logger + 4u);
}

std::uint32_t CombatEventLogger::SetLogFile(
    std::uint32_t logger, std::string_view filename,
    std::uint32_t mode, ICombatLogFileRuntime& runtime) {
    const auto ios_offset = [&] {
        return runtime.ReadWord(runtime.ReadWord(logger + 4u) + 4u);
    };
    const auto handle = runtime.ReadWord(logger + 88u);
    if (handle != 0u) {
        if (runtime.CloseFile(handle) == 0)
            runtime.InitializeFileBuffer(logger + 8u, 0u, 2u);
        else
            runtime.SetIoStreamState(logger + 4u + ios_offset(), 2u, false);
    }

    if (runtime.ReadWord(logger + 88u) != 0u) {
        runtime.SetIoStreamState(logger + 4u + ios_offset(), 2u, false);
    } else {
        const auto open_mode = ((mode & 0xffu) == 0u ? 16u : 8u) | 2u;
        const auto opened = runtime.OpenFile(filename, open_mode);
        if (opened == 0u) {
            runtime.SetIoStreamState(logger + 4u + ios_offset(), 2u, false);
        } else {
            runtime.InitializeFileBuffer(logger + 8u, opened, 1u);
            runtime.GetCodecvtFacet(logger + 8u);
        }
    }

    const auto ios_minus_four = logger + ios_offset();
    if ((runtime.ReadByte(ios_minus_four + 8u) & 6u) != 0u)
        throw std::runtime_error(
            "skLogger::setLogFilename - Failed to open file.");
    runtime.WriteWord(ios_minus_four + 24u, 2u);
    const auto buffering = runtime.ReadByte(logger + 292u);
    if (buffering == 0u)
        return (ios_minus_four & ~0xffu) | buffering;

    const auto begin = runtime.ReadWord(logger + 148u);
    auto end = runtime.ReadWord(logger + 152u);
    runtime.WriteByte(logger + 292u, 0u);
    for (auto record = begin; record != end; record += 20u) {
        if (runtime.ReadWord(logger) >= runtime.ReadWord(record + 16u)) {
            auto text = runtime.ReadWord(record + 4u);
            if (text == 0u) text = 0x841964u;
            runtime.InsertPrimary(logger + 4u, text);
            runtime.FlushPrimary(logger + 4u);
        }
        end = runtime.ReadWord(logger + 152u);
    }
    const auto release_begin = runtime.ReadWord(logger + 148u);
    const auto release_end = runtime.ReadWord(logger + 152u);
    for (auto record = release_begin; record != release_end; record += 20u)
        runtime.ReleaseText(record);
    runtime.WriteWord(logger + 152u, release_begin);
    return end;
}

}  // namespace pal4
