#include "pal4/combat_log_buffer.h"

namespace pal4 {

namespace {

std::uint32_t CurrentPointer(std::uint32_t buffer, std::uint32_t offset,
                             ICombatLogBufferStorageRuntime& runtime) {
    return runtime.ReadWord(runtime.ReadWord(buffer + offset));
}

void UpdateEnd(std::uint32_t buffer,
               ICombatLogBufferStorageRuntime& runtime) {
    const auto writer = CurrentPointer(buffer, 32u, runtime);
    if (writer != 0u && runtime.ReadWord(buffer + 60u) < writer)
        runtime.WriteWord(buffer + 60u, writer);
}

bool ValidTarget(std::int32_t target, std::uint32_t buffer,
                 ICombatLogBufferStorageRuntime& runtime) {
    if (target < 0) return false;
    const auto base = CurrentPointer(buffer, 12u, runtime);
    return target <= static_cast<std::int32_t>(
                         runtime.ReadWord(buffer + 60u) - base);
}

std::int32_t RelativeTarget(std::int32_t offset, std::uint32_t anchor,
                            std::uint32_t base) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(offset) + anchor - base);
}

void MoveWriter(std::uint32_t buffer, std::int32_t target,
                ICombatLogBufferStorageRuntime& runtime) {
    const auto base = CurrentPointer(buffer, 12u, runtime);
    const auto writer_ref = runtime.ReadWord(buffer + 32u);
    const auto writer = runtime.ReadWord(writer_ref);
    const auto delta = base + static_cast<std::uint32_t>(target) - writer;
    const auto count_ref = runtime.ReadWord(buffer + 48u);
    runtime.WriteWord(count_ref, runtime.ReadWord(count_ref) - delta);
    runtime.WriteWord(writer_ref, writer + delta);
}

void MoveReader(std::uint32_t buffer, std::int32_t target,
                bool move_writer, ICombatLogBufferStorageRuntime& runtime) {
    const auto base = CurrentPointer(buffer, 12u, runtime);
    const auto reader_ref = runtime.ReadWord(buffer + 28u);
    const auto reader = runtime.ReadWord(reader_ref);
    const auto delta = base + static_cast<std::uint32_t>(target) - reader;
    const auto count_ref = runtime.ReadWord(buffer + 44u);
    runtime.WriteWord(count_ref, runtime.ReadWord(count_ref) - delta);
    const auto new_reader = reader + delta;
    runtime.WriteWord(reader_ref, new_reader);
    if (move_writer) {
        const auto writer_ref = runtime.ReadWord(buffer + 32u);
        const auto writer = runtime.ReadWord(writer_ref);
        if (writer != 0u) {
            const auto write_count_ref = runtime.ReadWord(buffer + 48u);
            const auto write_count = runtime.ReadWord(write_count_ref);
            runtime.WriteWord(writer_ref, new_reader);
            runtime.WriteWord(write_count_ref,
                              writer + write_count - new_reader);
        }
    }
}

void WriteSeekResult(std::uint32_t result, std::int32_t position,
                     ICombatLogBufferStorageRuntime& runtime) {
    runtime.WriteWord(result, static_cast<std::uint32_t>(position));
    runtime.WriteWord(result + 8u, runtime.ReadWord(0x8683e0u));
    runtime.WriteWord(result + 12u, runtime.ReadWord(0x8683e4u));
    runtime.WriteWord(result + 16u, runtime.ReadWord(0x978b34u));
}

std::uint32_t InitializeStorageCore(
    std::uint32_t buffer, std::uint32_t source, std::uint32_t length,
    std::uint32_t flags, ICombatLogBufferStorageRuntime& runtime) {
    runtime.WriteWord(buffer + 56u, 0u);
    runtime.WriteWord(buffer + 60u, 0u);
    runtime.WriteWord(buffer + 64u, 32u);
    runtime.WriteWord(buffer + 68u, flags);

    const auto base_ref = runtime.ReadWord(buffer + 12u);
    const auto reader_ref = runtime.ReadWord(buffer + 28u);
    const auto read_count_ref = runtime.ReadWord(buffer + 44u);
    const auto output_base_ref = runtime.ReadWord(buffer + 16u);
    const auto writer_ref = runtime.ReadWord(buffer + 32u);
    const auto write_count_ref = runtime.ReadWord(buffer + 48u);
    if (length == 0u || (flags & 6u) == 6u) {
        runtime.WriteWord(base_ref, 0u);
        runtime.WriteWord(reader_ref, 0u);
        runtime.WriteWord(read_count_ref, 0u);
        runtime.WriteWord(output_base_ref, 0u);
        runtime.WriteWord(writer_ref, 0u);
        runtime.WriteWord(write_count_ref, 0u);
        return write_count_ref;
    }

    const auto allocation_size = static_cast<std::int32_t>(length) < 0
                                     ? 0u : length;
    const auto memory = runtime.Allocate(allocation_size);
    runtime.Copy(memory, source, length);
    runtime.WriteWord(buffer + 60u, memory + length);
    if ((flags & 4u) == 0u) {
        runtime.WriteWord(base_ref, memory);
        runtime.WriteWord(reader_ref, memory);
        runtime.WriteWord(read_count_ref, length);
    }
    if ((flags & 2u) == 0u) {
        runtime.WriteWord(output_base_ref, memory);
        runtime.WriteWord(writer_ref, memory);
        runtime.WriteWord(write_count_ref, length);
        if (runtime.ReadWord(reader_ref) == 0u) {
            runtime.WriteWord(base_ref, memory);
            runtime.WriteWord(reader_ref, memory);
            runtime.WriteWord(read_count_ref, 0u);
        }
    }
    runtime.WriteWord(buffer + 68u, flags | 1u);
    return flags | 1u;
}

}  // namespace

std::int32_t CombatLogBuffer::Add(
    std::uint32_t buffer, std::int32_t character,
    ICombatLogBufferStorageRuntime& runtime) {
    if (character == -1) return 0;

    const auto write_pointer_ref = runtime.ReadWord(buffer + 32u);
    const auto write_count_ref = runtime.ReadWord(buffer + 48u);
    auto write_pointer = runtime.ReadWord(write_pointer_ref);
    const auto write_count = runtime.ReadWord(write_count_ref);
    if (write_pointer != 0u &&
        write_pointer < write_pointer + write_count) {
        runtime.WriteWord(write_count_ref, write_count - 1u);
    } else {
        const auto flags = runtime.ReadWord(buffer + 68u);
        if ((flags & 2u) != 0u) return -1;

        const auto base_ref = runtime.ReadWord(buffer + 12u);
        const auto input_ref = runtime.ReadWord(buffer + 28u);
        const auto old_base = runtime.ReadWord(base_ref);
        const auto old_input = runtime.ReadWord(input_ref);
        const auto old_size = old_input != 0u
                                  ? write_pointer + write_count - old_base
                                  : 0u;
        const auto increment = runtime.ReadWord(buffer + 64u);
        const auto new_size = old_size + increment;
        const auto allocation_size = static_cast<std::int32_t>(new_size) < 0
                                         ? 0u : new_size;
        const auto memory = runtime.Allocate(allocation_size);
        if (old_size != 0u) {
            runtime.Copy(memory, old_base, old_size);
        } else if (static_cast<std::int32_t>(increment) > 512) {
            runtime.WriteWord(buffer + 64u, 512u);
        }
        if ((flags & 1u) != 0u) runtime.Free(old_base);
        runtime.WriteWord(buffer + 68u, flags | 1u);

        const auto other_pointer_ref = runtime.ReadWord(buffer + 16u);
        const auto input_count_ref = runtime.ReadWord(buffer + 44u);
        if (old_size != 0u) {
            const auto delta = memory - old_base;
            runtime.WriteWord(buffer + 60u,
                              runtime.ReadWord(buffer + 60u) + delta);
            runtime.WriteWord(other_pointer_ref,
                              runtime.ReadWord(other_pointer_ref) + delta);
            write_pointer = runtime.ReadWord(write_pointer_ref) + delta;
            runtime.WriteWord(write_pointer_ref, write_pointer);
            runtime.WriteWord(write_count_ref,
                              new_size + memory - write_pointer);
            if ((flags & 4u) != 0u) {
                runtime.WriteWord(base_ref, memory);
                runtime.WriteWord(input_ref, memory);
                runtime.WriteWord(input_count_ref, 0u);
            } else {
                const auto input_offset = old_input - old_base;
                runtime.WriteWord(base_ref, memory);
                const auto new_input = memory + input_offset;
                runtime.WriteWord(input_ref, new_input);
                runtime.WriteWord(input_count_ref,
                                  write_pointer - new_input + 1u);
            }
        } else {
            runtime.WriteWord(buffer + 60u, memory);
            runtime.WriteWord(other_pointer_ref, memory);
            runtime.WriteWord(write_pointer_ref, memory);
            runtime.WriteWord(write_count_ref, new_size);
            runtime.WriteWord(base_ref, memory);
            runtime.WriteWord(input_ref, memory);
            runtime.WriteWord(input_count_ref, 0u);
            write_pointer = memory;
        }
        runtime.WriteWord(write_count_ref,
                          runtime.ReadWord(write_count_ref) - 1u);
    }
    runtime.WriteWord(write_pointer_ref, write_pointer + 1u);
    runtime.WriteByte(write_pointer,
                      static_cast<std::uint8_t>(character));
    return character;
}

std::int32_t CombatLogBuffer::Remove(
    std::uint32_t buffer, std::int32_t character,
    ICombatLogBufferStorageRuntime& runtime) {
    const auto input_ref = runtime.ReadWord(buffer + 28u);
    const auto input_pointer = runtime.ReadWord(input_ref);
    if (input_pointer == 0u) return -1;
    const auto base_ref = runtime.ReadWord(buffer + 12u);
    if (input_pointer <= runtime.ReadWord(base_ref)) return -1;
    if (character != -1 &&
        static_cast<std::uint8_t>(character) !=
            runtime.ReadByte(input_pointer - 1u) &&
        (runtime.ReadWord(buffer + 68u) & 2u) != 0u)
        return -1;

    const auto input_count_ref = runtime.ReadWord(buffer + 44u);
    runtime.WriteWord(input_count_ref,
                      runtime.ReadWord(input_count_ref) + 1u);
    runtime.WriteWord(input_ref, input_pointer - 1u);
    if (character == -1) return 0;
    runtime.WriteByte(input_pointer - 1u,
                      static_cast<std::uint8_t>(character));
    return character;
}

std::int32_t CombatLogBuffer::Peek(
    std::uint32_t buffer, ICombatLogBufferStorageRuntime& runtime) {
    const auto input_ref = runtime.ReadWord(buffer + 28u);
    const auto input = runtime.ReadWord(input_ref);
    if (input == 0u) return -1;
    const auto count_ref = runtime.ReadWord(buffer + 44u);
    const auto count = runtime.ReadWord(count_ref);
    if (input < input + count) return runtime.ReadByte(input);
    if ((runtime.ReadByte(buffer + 68u) & 4u) != 0u) return -1;

    const auto writer_ref = runtime.ReadWord(buffer + 32u);
    const auto writer = runtime.ReadWord(writer_ref);
    if (writer == 0u) return -1;
    auto end = runtime.ReadWord(buffer + 60u);
    if (writer <= input && end <= input) return -1;
    if (end < writer) {
        end = writer;
        runtime.WriteWord(buffer + 60u, end);
    }
    runtime.WriteWord(count_ref, end - input);
    return runtime.ReadByte(input);
}

std::uint32_t CombatLogBuffer::SeekRelative(
    std::uint32_t buffer, std::uint32_t result,
    std::int32_t offset, std::uint32_t origin, std::uint8_t mode,
    ICombatLogBufferStorageRuntime& runtime) {
    UpdateEnd(buffer, runtime);
    std::int32_t target = -1;
    const auto reader = (mode & 1u) != 0u
                            ? CurrentPointer(buffer, 28u, runtime) : 0u;
    const auto writer = CurrentPointer(buffer, 32u, runtime);
    if (reader != 0u) {
        if (origin == 0u || origin == 2u ||
            (origin == 1u && (mode & 2u) == 0u)) {
            const auto base = CurrentPointer(buffer, 12u, runtime);
            const auto anchor = origin == 2u ? runtime.ReadWord(buffer + 60u)
                                : origin == 1u ? reader : base;
            target = RelativeTarget(offset, anchor, base);
            if (ValidTarget(target, buffer, runtime))
                MoveReader(buffer, target, (mode & 2u) != 0u, runtime);
            else
                target = -1;
        }
    } else if ((mode & 2u) != 0u && writer != 0u && origin <= 2u) {
        const auto base = CurrentPointer(buffer, 12u, runtime);
        const auto anchor = origin == 2u ? runtime.ReadWord(buffer + 60u)
                            : origin == 1u ? writer : base;
        target = RelativeTarget(offset, anchor, base);
        if (ValidTarget(target, buffer, runtime))
            MoveWriter(buffer, target, runtime);
        else
            target = -1;
    }
    WriteSeekResult(result, target, runtime);
    return result;
}

std::uint32_t CombatLogBuffer::SeekPosition(
    std::uint32_t buffer, std::uint32_t result,
    std::int32_t first_offset, std::int32_t second_offset,
    std::uint8_t mode, ICombatLogBufferStorageRuntime& runtime) {
    UpdateEnd(buffer, runtime);
    auto target = static_cast<std::int32_t>(
        static_cast<std::uint32_t>(first_offset) +
        static_cast<std::uint32_t>(second_offset));
    if (target != -1) {
        const auto reader = (mode & 1u) != 0u
                                ? CurrentPointer(buffer, 28u, runtime) : 0u;
        const auto writer = CurrentPointer(buffer, 32u, runtime);
        if (ValidTarget(target, buffer, runtime)) {
            if (reader != 0u)
                MoveReader(buffer, target, (mode & 2u) != 0u, runtime);
            else if ((mode & 2u) != 0u && writer != 0u)
                MoveWriter(buffer, target, runtime);
            else
                target = -1;
        } else {
            target = -1;
        }
    }
    WriteSeekResult(result, target, runtime);
    return result;
}

std::uint32_t CombatLogBuffer::GetString(
    std::uint32_t buffer, std::uint32_t result,
    ICombatLogTextRuntime& runtime) {
    const auto flags = runtime.ReadWord(buffer + 68u);
    if ((flags & 2u) == 0u) {
        const auto writer = runtime.ReadWord(runtime.ReadWord(buffer + 32u));
        if (writer != 0u) {
            const auto end = runtime.ReadWord(buffer + 60u) >= writer
                                 ? runtime.ReadWord(buffer + 60u) : writer;
            const auto source = runtime.ReadWord(
                runtime.ReadWord(buffer + 16u));
            runtime.AssignText(result, source, end - source,
                               CombatLogTextSource::Output);
            return result;
        }
    }
    if ((flags & 4u) == 0u) {
        const auto reader = runtime.ReadWord(runtime.ReadWord(buffer + 28u));
        if (reader != 0u) {
            const auto source = runtime.ReadWord(
                runtime.ReadWord(buffer + 12u));
            const auto count = runtime.ReadWord(
                runtime.ReadWord(buffer + 44u));
            runtime.AssignText(result, source, reader + count - source,
                               CombatLogTextSource::Input);
            return result;
        }
    }
    runtime.AssignText(result, 0u, 0u, CombatLogTextSource::Empty);
    return result;
}

std::uint32_t CombatLogBuffer::InitializeFromText(
    std::uint32_t buffer, std::uint32_t text,
    ICombatLogBufferStorageRuntime& runtime) {
    const auto old_flags = runtime.ReadByte(buffer + 68u);
    if ((old_flags & 1u) != 0u)
        runtime.Free(CurrentPointer(buffer, 12u, runtime));
    const auto flags = runtime.ReadWord(buffer + 68u) & ~1u;
    runtime.WriteWord(buffer + 60u, 0u);
    runtime.WriteWord(buffer + 68u, flags);

    const auto length = runtime.ReadWord(text + 8u);
    const auto text_pointer = runtime.ReadWord(text + 4u);
    const auto source = text_pointer != 0u ? text_pointer : 0x841964u;
    return InitializeStorageCore(buffer, source, length, flags, runtime);
}

std::uint32_t CombatLogBuffer::CleanupStorage(
    std::uint32_t buffer, ICombatLogBufferStorageRuntime& runtime) {
    if ((runtime.ReadByte(buffer + 68u) & 1u) != 0u)
        runtime.Free(CurrentPointer(buffer, 12u, runtime));
    const auto flags = runtime.ReadWord(buffer + 68u) & ~1u;
    runtime.WriteWord(buffer + 60u, 0u);
    runtime.WriteWord(buffer + 68u, flags);
    return flags;
}

std::uint32_t CombatLogBuffer::ConstructBase(
    std::uint32_t buffer, ICombatLogBaseRuntime& runtime) {
    runtime.WriteWord(buffer + 52u, runtime.InitializeLocale());
    const auto locale = runtime.ReadWord(0x950a08u);
    const auto references = runtime.ReadWord(locale + 4u);
    if (references != 0xffffffffu)
        runtime.WriteWord(locale + 4u, references + 1u);

    runtime.WriteWord(buffer + 12u, buffer + 4u);
    runtime.WriteWord(buffer + 28u, buffer + 20u);
    runtime.WriteWord(buffer + 32u, buffer + 24u);
    runtime.WriteWord(buffer, 0x868310u);
    runtime.WriteWord(buffer + 16u, buffer + 8u);
    runtime.WriteWord(buffer + 44u, buffer + 36u);
    runtime.WriteWord(buffer + 48u, buffer + 40u);
    runtime.WriteWord(buffer + 8u, 0u);
    runtime.WriteWord(buffer + 24u, 0u);
    runtime.WriteWord(buffer + 40u, 0u);
    runtime.WriteWord(buffer + 4u, 0u);
    runtime.WriteWord(buffer + 20u, 0u);
    runtime.WriteWord(buffer + 36u, 0u);
    return buffer;
}

std::uint8_t CombatLogBuffer::InitializeStorage(
    std::uint32_t buffer, std::uint32_t source, std::int32_t length,
    std::uint32_t flags, ICombatLogBufferStorageRuntime& runtime) {
    return static_cast<std::uint8_t>(InitializeStorageCore(
        buffer, source, static_cast<std::uint32_t>(length), flags, runtime));
}

std::uint8_t CombatLogBuffer::ConvertFlags(std::uint8_t flags) {
    std::uint8_t result = (flags & 1u) == 0u ? 4u : 0u;
    if ((flags & 2u) == 0u) result |= 2u;
    return result;
}

}  // namespace pal4
