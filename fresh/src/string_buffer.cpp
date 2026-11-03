#include "pal4/string_buffer.h"

#include <cstdint>
#include <stdexcept>

namespace pal4 {

std::uint32_t StringBuffer::CharacterAddressAt(
    const std::uint32_t record, const std::int32_t index,
    IStringBufferRuntime& runtime) {
    const auto data = runtime.ReadDword(record + 4u);
    return data == 0u ? EmptyLogMessageAddress() :
                        data + static_cast<std::uint32_t>(index);
}
namespace {

constexpr std::uint32_t Add32(const std::uint32_t left,
                              const std::uint32_t right) noexcept {
    return left + right;
}

std::int32_t CompareData(std::uint32_t left, std::uint32_t right,
                         IStringBufferRuntime& runtime) {
    for (;;) {
        const auto left_byte = runtime.ReadByte(left++);
        const auto right_byte = runtime.ReadByte(right++);
        if (left_byte != right_byte)
            return left_byte < right_byte ? -1 : 1;
        if (left_byte == 0u) return 0;
    }
}

}  // namespace

std::uint8_t StringUniqueRuntimeStub::ReadByte(
    const std::uint32_t address) { return bytes_.at(address); }
void StringUniqueRuntimeStub::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    bytes_[address] = value;
}
void StringUniqueRuntimeStub::ThrowLength() {
    throw std::length_error("string buffer length");
}
std::uint32_t StringUniqueRuntimeStub::SetSize(
    StringBufferState& state, const std::uint32_t length) {
    state.length = length;
    if (state.data != 0u) {
        WriteByte(state.data + length, 0u);
    }
    return length;
}
std::uint32_t StringUniqueRuntimeStub::Reallocate(
    StringBufferState& state, const std::uint32_t length) {
    state.capacity = length;
    return state.data;
}

StringBuffer StringBuffer::Create(const std::uint32_t source,
                                  const std::uint32_t length,
                                  IStringBufferRuntime& runtime) {
    StringBuffer result;
    result.state_.capacity = Add32(length, 8U);
    result.state_.data = runtime.Allocate(result.state_.capacity);
    result.state_.length = length;
    runtime.Copy(result.state_.data, source, length);
    runtime.WriteByte(Add32(result.state_.data, length), 0);
    return result;
}

StringBuffer StringBuffer::CreatePrefix(
    const StringBufferState& source, const std::int32_t requested_length,
    IStringBufferRuntime& runtime) {
    const auto selected = requested_length >=
            static_cast<std::int32_t>(source.length)
        ? source.length : static_cast<std::uint32_t>(requested_length);
    return Create(source.data, selected, runtime);
}

std::uint32_t StringBuffer::ExtractTailAt(
    const std::uint32_t source_record,
    const std::uint32_t destination_record,
    const std::int32_t requested_length,
    IStringBufferRuntime& runtime) {
    const auto source_length = runtime.ReadDword(source_record + 4u);
    const auto length = requested_length >=
            static_cast<std::int32_t>(source_length) ?
        source_length : static_cast<std::uint32_t>(requested_length);
    const auto source = runtime.ReadDword(source_record) +
                        source_length - length;
    const auto result = Create(source, length, runtime);
    runtime.WriteDword(destination_record, result.State().data);
    runtime.WriteDword(destination_record + 4u,
                       result.State().length);
    runtime.WriteDword(destination_record + 8u,
                       result.State().capacity);
    return destination_record;
}

StringBuffer& StringBuffer::CopyConstruct(
    const StringBuffer& source, IStringBufferRuntime& runtime) {
    state_.capacity = Add32(source.state_.length, 8U);
    state_.data = runtime.Allocate(state_.capacity);
    state_.length = source.state_.length;
    runtime.Copy(state_.data, source.state_.data, state_.length);
    runtime.WriteByte(Add32(state_.data, state_.length), 0);
    return *this;
}

StringBuffer StringBuffer::Concatenate(
    const std::uint32_t first, const std::uint32_t first_length,
    const std::uint32_t second, const std::uint32_t second_length,
    IStringBufferRuntime& runtime) {
    StringBuffer result;
    result.state_.length = Add32(first_length, second_length);
    result.state_.capacity = Add32(result.state_.length, 8U);
    result.state_.data = runtime.Allocate(result.state_.capacity);
    runtime.Copy(result.state_.data, first, first_length);
    runtime.Copy(Add32(result.state_.data, first_length), second,
                 second_length);
    runtime.WriteByte(Add32(result.state_.data, result.state_.length), 0);
    return result;
}

StringBuffer& StringBuffer::ConcatenateRecords(
    const StringBufferState& first, const StringBufferState& second,
    IStringBufferRuntime& runtime) {
    *this = Concatenate(first.data, first.length, second.data,
                        second.length, runtime);
    return *this;
}

StringBuffer& StringBuffer::ConcatenateRecordAndCString(
    const StringBufferState& first,
    const std::uint32_t c_string,
    IStringBufferRuntime& runtime) {
    std::uint32_t length = 0u;
    while (runtime.ReadByte(c_string + length) != 0u) {
        ++length;
    }
    *this = Concatenate(first.data, first.length,
                        c_string, length, runtime);
    return *this;
}

StringBuffer& StringBuffer::InitializeEmpty(IStringBufferRuntime& runtime) {
    state_.data = runtime.Allocate(0x20U);
    state_.length = 0;
    state_.capacity = 0x20U;
    runtime.WriteByte(state_.data, 0);
    return *this;
}

std::uint32_t StringBuffer::InitializeEmptyAt(
    const std::uint32_t self_address, IStringBufferStorageRuntime& runtime) {
    const auto data = runtime.Allocate(0x20);
    runtime.WriteDword(self_address, data);
    runtime.WriteDword(self_address + 4, 0);
    runtime.WriteDword(self_address + 8, 0x20);
    runtime.WriteByte(data, 0);
    return self_address;
}

StringBuffer& StringBuffer::InitializeFromCString(
    const std::uint32_t source, IStringBufferRuntime& runtime) {
    if (source == 0) {
        state_.data = 0;
        state_.length = 0;
        state_.capacity = 8;
        return *this;
    }

    std::uint32_t length = 0;
    while (runtime.ReadByte(Add32(source, length)) != 0) ++length;
    state_.length = length;
    state_.capacity = Add32(length, 8u);
    state_.data = runtime.Allocate(state_.capacity);
    runtime.Copy(state_.data, source, length);
    runtime.WriteByte(Add32(state_.data, length), 0);
    return *this;
}

std::uint32_t StringBuffer::Cleanup(IStringBufferRuntime& runtime) {
    if (state_.data == 0) return 0;
    const auto result = runtime.Free(state_.data);
    state_.data = 0;
    return result;
}

void StringBuffer::ReallocateAndInitialize(
    const std::uint32_t requested_length, IStringBufferRuntime& runtime) {
    static_cast<void>(runtime.Free(state_.data));
    const auto capacity = Add32(requested_length, 8U);
    const auto allocated = runtime.Allocate(capacity);
    state_.data = allocated;
    runtime.WriteByte(allocated, 0);
    state_.capacity = capacity;
    state_.length = 0;
}

void StringBuffer::Reallocate(const std::uint32_t requested_length,
                              const bool copy_existing,
                              IStringBufferRuntime& runtime) {
    if (static_cast<std::int32_t>(Add32(requested_length, 1U)) <=
        static_cast<std::int32_t>(state_.capacity)) return;

    const auto new_capacity = Add32(requested_length, 8U);
    const auto new_data = runtime.Allocate(new_capacity);
    if (copy_existing) {
        runtime.Copy(new_data, state_.data, state_.length);
    }
    static_cast<void>(runtime.Free(state_.data));
    state_.capacity = new_capacity;
    state_.data = new_data;
}

StringBuffer& StringBuffer::FormatVariadic(
    const std::uint32_t format, const std::uint32_t arguments,
    IStringBufferRuntime& runtime, IStringBufferFormatRuntime& formatter) {
    Reallocate(1024u, false, runtime);
    state_.length = static_cast<std::uint32_t>(
        formatter.Vsprintf(state_.data, format, arguments));
    return *this;
}

std::int32_t StringBuffer::FindSubstring(
    const std::uint32_t substring, const std::uint32_t start_offset,
    IStringBufferRuntime& runtime) const {
    const auto found = runtime.FindSubstring(Add32(state_.data, start_offset),
                                             substring);
    return found == 0 ? -1 : static_cast<std::int32_t>(found - state_.data);
}

std::int32_t StringBuffer::FindSubstringOffset(
    const std::uint32_t substring_pointer, const std::uint32_t start_offset,
    IStringBufferRuntime& runtime) const {
    const auto found = runtime.FindSubstring(Add32(state_.data, start_offset),
                                             runtime.ReadDword(substring_pointer));
    if (found == 0) return -1;
    return static_cast<std::int32_t>(found - state_.data);
}

std::int32_t StringBuffer::FindLastCharacter(
    const std::int32_t character, IStringBufferRuntime& runtime) const {
    const auto found = runtime.FindLastCharacter(
        state_.data, static_cast<std::int8_t>(character));
    if (found == 0) return -1;
    return static_cast<std::int32_t>(found - state_.data);
}

std::int32_t StringBuffer::Compare(const std::uint32_t left_record,
                                   const std::uint32_t right_record,
                                   IStringBufferRuntime& runtime) {
    return CompareData(runtime.ReadDword(left_record),
                       runtime.ReadDword(right_record), runtime);
}

std::int32_t StringBuffer::Compare(const StringBuffer& left,
                                   const StringBuffer& right,
                                   IStringBufferRuntime& runtime) {
    return CompareData(left.state_.data, right.state_.data, runtime);
}

std::int32_t StringBuffer::CompareCaseInsensitive(
    const std::uint32_t other_record, IStringBufferRuntime& runtime) const {
    return runtime.CompareCaseInsensitive(
        state_.data, runtime.ReadDword(other_record));
}

std::int32_t StringBuffer::CompareCaseInsensitive(
    const StringBuffer& other, IStringBufferRuntime& runtime) const {
    return runtime.CompareCaseInsensitive(state_.data, other.state_.data);
}

StringBuffer& StringBuffer::ToUpper(IStringBufferRuntime& runtime) {
    runtime.ConvertUppercase(state_.data);
    return *this;
}

StringBuffer& StringBuffer::ToLower(IStringBufferRuntime& runtime) {
    runtime.ConvertLowercase(state_.data);
    return *this;
}

StringBuffer& StringBuffer::TrimLeft(const std::uint32_t characters,
                                     IStringBufferRuntime& runtime) {
    std::uint32_t index = 0;
    auto remaining = state_.length;
    while (remaining != 0) {
        const auto value = static_cast<std::int8_t>(
            runtime.ReadByte(Add32(state_.data, index)));
        const auto current = runtime.FindCharacter(characters, value);
        if (current == 0) break;
        ++index;
        --remaining;
    }

    state_.length = remaining;
    runtime.Copy(state_.data, Add32(state_.data, index),
                 Add32(remaining, 1U));
    return *this;
}

StringBuffer& StringBuffer::TrimRight(const std::uint32_t characters,
                                      IStringBufferRuntime& runtime) {
    while (state_.length != 0) {
        const auto value = static_cast<std::int8_t>(runtime.ReadByte(
            Add32(state_.data, state_.length - 1U)));
        const auto character = runtime.FindCharacter(characters, value);
        if (character == 0) break;
        --state_.length;
    }
    runtime.WriteByte(Add32(state_.data, state_.length), 0);
    return *this;
}

StringBuffer& StringBuffer::Trim(const std::uint32_t characters,
                                 IStringBufferRuntime& runtime) {
    static_cast<void>(TrimRight(characters, runtime));
    static_cast<void>(TrimLeft(characters, runtime));
    return *this;
}

StringBuffer& StringBuffer::Initialize(const std::uint32_t source,
                                      const std::uint32_t length,
                                      IStringBufferRuntime& runtime) {
    if (state_.data == 0) {
        state_.length = length;
        state_.capacity = Add32(length, 8U);
        state_.data = runtime.Allocate(state_.capacity);
    }
    Reallocate(length, false, runtime);
    state_.length = length;
    runtime.Copy(state_.data, source, length);
    runtime.WriteByte(Add32(state_.data, length), 0);
    return *this;
}

std::uint32_t StringBuffer::InitializeAt(
    const std::uint32_t self_address, const std::uint32_t source,
    const std::uint32_t length, IStringBufferRuntime& runtime) {
    auto data = runtime.ReadDword(self_address);
    if (data == 0) {
        runtime.WriteDword(self_address + 4, length);
        const auto initial_capacity = Add32(length, 8U);
        runtime.WriteDword(self_address + 8, initial_capacity);
        data = runtime.Allocate(initial_capacity);
        runtime.WriteDword(self_address, data);
    }

    const auto capacity = runtime.ReadDword(self_address + 8);
    if (static_cast<std::int32_t>(Add32(length, 1U)) >
        static_cast<std::int32_t>(capacity)) {
        const auto new_capacity = length + 8U;
        const auto new_data = runtime.Allocate(new_capacity);
        static_cast<void>(runtime.Free(data));
        runtime.WriteDword(self_address + 8, new_capacity);
        runtime.WriteDword(self_address, new_data);
        data = new_data;
    }
    runtime.WriteDword(self_address + 4, length);
    runtime.Copy(data, source, length);
    runtime.WriteByte(Add32(data, length), 0);
    return self_address;
}

std::uint32_t StringBuffer::CleanupAt(
    const std::uint32_t self_address, IStringBufferStorageRuntime& runtime) {
    const auto data = runtime.ReadDword(self_address);
    if (data == 0) return 0;
    const auto result = runtime.Free(data);
    runtime.WriteDword(self_address, 0);
    return result;
}

StringBuffer& StringBuffer::Append(const std::uint32_t source,
                                  const std::uint32_t length,
                                  IStringBufferRuntime& runtime) {
    const auto old_length = state_.length;
    const auto new_length = Add32(old_length, length);
    Reallocate(new_length, true, runtime);
    runtime.Copy(Add32(state_.data, old_length), source, length);
    state_.length = new_length;
    runtime.WriteByte(Add32(state_.data, new_length), 0);
    return *this;
}

StringBuffer& StringBuffer::AppendRange(
    const StringBuffer& source, const std::uint32_t start,
    const std::uint32_t count, IStringAssignRuntime& runtime) {
    if (source.state_.length < start) {
        runtime.ThrowRange();
    }
    const std::uint32_t available = source.state_.length - start;
    const std::uint32_t selected = count < available ? count : available;
    if (0xffffffffu - state_.length <= selected) {
        runtime.ThrowLength();
    }
    if (selected == 0u) {
        return *this;
    }

    const std::uint32_t new_length = state_.length + selected;
    if (new_length > MaxSize()) {
        runtime.ThrowLength();
    }
    if (state_.data != 0u) {
        const std::uint8_t reference = runtime.ReadByte(state_.data - 1u);
        if (reference != 0u && reference != 0xffu) {
            runtime.Reallocate(state_, new_length);
        } else if (state_.capacity < new_length) {
            runtime.Reallocate(state_, new_length);
        }
    } else if (state_.capacity < new_length) {
        runtime.Reallocate(state_, new_length);
    }

    const std::uint32_t source_data = source.state_.data != 0u
        ? source.state_.data : 0x841964u;
    runtime.Copy(state_.data + state_.length, source_data + start,
                 selected);
    state_.length = new_length;
    runtime.WriteByte(state_.data + new_length, 0u);
    return *this;
}

StringBuffer& StringBuffer::AppendRaw(
    const std::uint32_t source, const std::uint32_t length,
    IStringAssignRuntime& runtime) {
    if (0xffffffffu - state_.length <= length) {
        runtime.ThrowLength();
    }
    if (length == 0u) {
        return *this;
    }
    const std::uint32_t new_length = state_.length + length;
    if (new_length > MaxSize()) {
        runtime.ThrowLength();
    }
    if (state_.data != 0u) {
        const std::uint8_t reference = runtime.ReadByte(state_.data - 1u);
        if ((reference != 0u && reference != 0xffu) ||
            state_.capacity < new_length) {
            runtime.Reallocate(state_, new_length);
        }
    } else if (state_.capacity < new_length) {
        runtime.Reallocate(state_, new_length);
    }
    runtime.Copy(state_.data + state_.length, source, length);
    state_.length = new_length;
    runtime.WriteByte(state_.data + new_length, 0u);
    return *this;
}

StringBuffer& StringBuffer::AppendCString(
    const std::uint32_t source, IStringAssignRuntime& runtime) {
    std::uint32_t length = 0;
    while (runtime.ReadByte(source + length) != 0u) {
        if (length == MaxSize()) runtime.ThrowLength();
        ++length;
    }
    return AppendRaw(source, length, runtime);
}

StringBuffer& StringBuffer::Assign(const StringBuffer& source,
                                   const std::uint32_t start,
                                   const std::uint32_t count,
                                   IStringAssignRuntime& runtime) {
    if (source.state_.length < start) {
        runtime.ThrowRange();
    }
    std::uint32_t selected = source.state_.length - start;
    if (count < selected) {
        selected = count;
    }

    if (this == &source) {
        const std::uint32_t selected_end = start + selected;
        if (state_.length < selected_end) {
            runtime.ThrowRange();
        }
        runtime.DetachAndReset(state_);
        const std::uint32_t remaining = state_.length - selected_end;
        std::uint32_t removed = 0xffffffffu;
        if (remaining < 0xffffffffu) {
            removed = remaining;
        }
        if (remaining == 0xffffffffu || removed != 0u) {
            runtime.Copy(state_.data + selected_end,
                         state_.data + selected_end + removed,
                         remaining - removed);
            const std::uint32_t new_length = state_.length - removed;
            if (runtime.AllocateText(state_, new_length, 0u)) {
                state_.length = new_length;
                runtime.WriteByte(state_.data + new_length, 0u);
            }
        }
        runtime.DetachAndReset(state_);
        const std::uint32_t prefix =
            start < state_.length ? start : state_.length;
        if (prefix != 0u) {
            runtime.Copy(state_.data, state_.data + prefix,
                         state_.length - prefix);
            const std::uint32_t new_length = state_.length - prefix;
            if (runtime.AllocateText(state_, new_length, 0u)) {
                runtime.SetTextSize(state_, new_length);
            }
        }
        return *this;
    }

    const std::uint32_t source_data = source.state_.data != 0u
        ? source.state_.data : 0x841964u;
    if (selected != 0u && selected == source.state_.length &&
        runtime.ReadByte(source_data - 1u) < 0xfeu) {
        if (state_.data != 0u) {
            const std::uint8_t reference = runtime.ReadByte(state_.data - 1u);
            if (reference == 0u || reference == 0xffu) {
                runtime.Free(state_.data - 1u);
            } else {
                runtime.WriteByte(state_.data - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
            }
        }
        state_ = {source_data, source.state_.length,
                  source.state_.capacity};
        runtime.WriteByte(source_data - 1u,
            static_cast<std::uint8_t>(
                runtime.ReadByte(source_data - 1u) + 1u));
        return *this;
    }

    if (MaxSize() < selected) {
        runtime.ThrowLength();
    }
    const std::uint32_t destination = state_.data;
    if (destination != 0u) {
        const std::uint8_t reference = runtime.ReadByte(destination - 1u);
        if (reference != 0u && reference != 0xffu) {
            if (selected == 0u) {
                runtime.WriteByte(destination - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
                runtime.SetErrorFlag(state_, 0u);
                return *this;
            }
            runtime.Reallocate(state_, selected);
        } else if (selected == 0u) {
            runtime.SetErrorFlag(state_, 1u);
            return *this;
        } else if (state_.capacity > 0x1fu ||
                   state_.capacity < selected) {
            runtime.SetErrorFlag(state_, 1u);
            runtime.Reallocate(state_, selected);
        }
    } else if (selected == 0u) {
        runtime.SetErrorFlag(state_, 1u);
        return *this;
    } else if (state_.capacity > 0x1fu ||
               state_.capacity < selected) {
        runtime.SetErrorFlag(state_, 1u);
        runtime.Reallocate(state_, selected);
    }

    runtime.Copy(state_.data, source_data + start, selected);
    state_.length = selected;
    runtime.WriteByte(state_.data + selected, 0u);
    return *this;
}

std::uint8_t StringBuffer::SetErrorFlag(const std::uint8_t flag,
                                        IStringResetRuntime& runtime) {
    std::uint32_t result = flag;
    if (flag != 0u) {
        result = state_.data;
        if (result != 0u) {
            --result;
            const std::uint8_t reference = runtime.ReadByte(result);
            if (reference == 0u || reference == 0xffu) {
                result = runtime.Free(result);
            } else {
                runtime.WriteByte(result,
                    static_cast<std::uint8_t>(reference - 1u));
            }
        }
    }
    state_ = {};
    return static_cast<std::uint8_t>(result);
}

std::uint32_t StringBuffer::SetSize(const std::uint32_t size,
                                    IStringResetRuntime& runtime) {
    state_.length = size;
    runtime.WriteByte(state_.data + size, 0u);
    return size;
}

bool StringBuffer::AllocateText(const std::uint32_t requested_length,
                                const std::uint8_t mode,
                                IStringAssignRuntime& runtime) {
    if (requested_length > MaxSize()) {
        runtime.ThrowLength();
    }
    const std::uint32_t data = state_.data;
    if (data != 0u) {
        const std::uint8_t reference = runtime.ReadByte(data - 1u);
        if (reference != 0u && reference != 0xffu) {
            if (requested_length != 0u) {
                runtime.Reallocate(state_, requested_length);
                return true;
            }
            runtime.WriteByte(data - 1u,
                static_cast<std::uint8_t>(reference - 1u));
            state_ = {};
            return false;
        }
    }

    if (requested_length == 0u) {
        if (mode != 0u) {
            if (data != 0u) {
                runtime.Free(data - 1u);
            }
            state_ = {};
        } else if (data != 0u) {
            state_.length = 0u;
            runtime.WriteByte(data, 0u);
        }
        return false;
    }

    if (mode == 0u) {
        if (state_.capacity < requested_length) {
            runtime.Reallocate(state_, requested_length);
        }
        return true;
    }

    if (state_.capacity <= 0x1fu &&
        state_.capacity >= requested_length) {
        return true;
    }

    if (data != 0u) {
        const std::uint8_t reference = runtime.ReadByte(data - 1u);
        if (reference != 0u && reference != 0xffu) {
            runtime.WriteByte(data - 1u,
                static_cast<std::uint8_t>(reference - 1u));
            state_ = {};
        } else {
            runtime.Free(data - 1u);
            state_ = {};
        }
    } else {
        state_.capacity = 0u;
    }
    runtime.Reallocate(state_, requested_length);
    return true;
}

std::uint32_t StringBuffer::ReallocateWithReferenceHeader(
    const std::uint32_t requested_length,
    IStringReallocateRuntime& runtime) {
    std::uint32_t capacity = requested_length | 0x1fu;
    if (capacity > MaxSize()) {
        capacity = requested_length;
    }
    const std::uint32_t allocation_size = capacity + 2u;
    const std::uint32_t base = runtime.Allocate(
        static_cast<std::int32_t>(allocation_size) < 0
            ? 0u : allocation_size);

    const std::uint32_t new_length =
        state_.length < capacity ? state_.length : capacity;
    if (state_.length != 0u) {
        runtime.Copy(base + 1u, state_.data, new_length);
    }

    if (state_.data != 0u) {
        const std::uint32_t old_header = state_.data - 1u;
        const std::uint8_t reference = runtime.ReadByte(old_header);
        if (reference != 0u && reference != 0xffu) {
            runtime.WriteByte(old_header,
                static_cast<std::uint8_t>(reference - 1u));
        } else {
            runtime.Free(old_header);
        }
    }

    state_.data = base + 1u;
    state_.capacity = capacity;
    runtime.WriteByte(base, 0u);
    state_.length = new_length;
    runtime.WriteByte(state_.data + state_.length, 0u);
    return state_.data;
}

std::uint32_t StringBuffer::DetachAndReset(
    IStringDetachRuntime& runtime) {
    const std::uint32_t old_data = state_.data;
    if (old_data == 0u) {
        return 0u;
    }
    const std::uint8_t reference = runtime.ReadByte(old_data - 1u);
    if (reference == 0u || reference == 0xffu) {
        return old_data;
    }

    runtime.WriteByte(old_data - 1u,
        static_cast<std::uint8_t>(reference - 1u));
    state_ = {};
    std::uint32_t length = 0u;
    while (runtime.ReadByte(old_data + length) != 0u) {
        ++length;
    }
    if (length > MaxSize()) {
        runtime.ThrowLength();
    }
    runtime.SetErrorFlag(state_, 1u);
    if (length == 0u) {
        return 0u;
    }

    (void)ReallocateWithReferenceHeader(length, runtime);
    runtime.Copy(state_.data, old_data, length);
    state_.length = length;
    runtime.WriteByte(state_.data + length, 0u);
    return length;
}

std::uint8_t StringBuffer::EnsureExclusive(
    StringBufferState& state, IStringUniqueRuntime& runtime) {
    std::uint32_t result = state.data;
    if (result != 0u) {
        const auto first_reference = runtime.ReadByte(result - 1u);
        result = (result & 0xffffff00u) | first_reference;
        if (first_reference != 0u && first_reference != 0xffu) {
            const auto length = state.length;
            if (length > MaxSize()) {
                runtime.ThrowLength();
            }
            result = state.data;
            bool shared = false;
            std::uint8_t second_reference = 0u;
            if (result != 0u) {
                second_reference = runtime.ReadByte(result - 1u);
                shared = second_reference != 0u &&
                         second_reference != 0xffu;
            }
            if (shared && length == 0u) {
                runtime.WriteByte(result - 1u,
                                  static_cast<std::uint8_t>(
                                      second_reference - 1u));
                state = {};
            } else if (shared ||
                       (length != 0u && state.capacity < length)) {
                result = runtime.Reallocate(state, state.length);
            } else if (length == 0u && result != 0u) {
                result = runtime.SetSize(state, state.length);
            }
        }
    }
    if (state.data != 0u) {
        runtime.WriteByte(state.data - 1u, 0xffu);
    }
    return static_cast<std::uint8_t>(result);
}

std::int32_t StringBuffer::CompareRange(
    const std::uint32_t start, const std::uint32_t count,
    const std::uint32_t other, const std::uint32_t other_length,
    IStringCompareRangeRuntime& runtime) const {
    if (state_.length < start) {
        runtime.ThrowRange();
    }
    const std::uint32_t remaining = state_.length - start;
    const std::uint32_t selected = remaining < count ? remaining : count;
    const std::uint32_t compared =
        selected < other_length ? selected : other_length;
    const std::uint32_t source =
        state_.data == 0u ? 0u : state_.data + start;
    for (std::uint32_t index = 0u; index < compared; ++index) {
        const std::uint8_t left = runtime.ReadByte(source + index);
        const std::uint8_t right = runtime.ReadByte(other + index);
        if (left != right) {
            return left < right ? -1 : 1;
        }
    }
    if (selected < other_length) {
        return -1;
    }
    return selected == other_length ? 0 : 1;
}

StringBuffer& StringBuffer::Erase(const std::uint32_t start,
                                  const std::uint32_t count,
                                  IStringAssignRuntime& runtime) {
    if (state_.length < start) {
        runtime.ThrowRange();
    }
    const std::uint32_t old_data = state_.data;
    if (old_data != 0u) {
        const std::uint8_t reference = runtime.ReadByte(old_data - 1u);
        if (reference != 0u && reference != 0xffu) {
            runtime.WriteByte(old_data - 1u,
                static_cast<std::uint8_t>(reference - 1u));
            state_ = {};
            std::uint32_t old_length = 0u;
            while (runtime.ReadByte(old_data + old_length) != 0u) {
                ++old_length;
            }
            if (runtime.AllocateText(state_, old_length, 1u)) {
                runtime.Copy(state_.data, old_data, old_length);
                runtime.SetTextSize(state_, old_length);
            }
        }
    }

    const std::uint32_t available = state_.length - start;
    const std::uint32_t removed = count < available ? count : available;
    if (removed == 0u) {
        return *this;
    }
    runtime.Copy(state_.data + start,
                 state_.data + start + removed,
                 available - removed);
    const std::uint32_t new_length = state_.length - removed;
    if (new_length > MaxSize()) {
        runtime.ThrowLength();
    }

    if (state_.data != 0u) {
        const std::uint8_t reference =
            runtime.ReadByte(state_.data - 1u);
        if (reference != 0u && reference != 0xffu) {
            if (new_length == 0u) {
                runtime.WriteByte(state_.data - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
                runtime.SetErrorFlag(state_, 0u);
                return *this;
            }
            runtime.Reallocate(state_, new_length);
            state_.length = new_length;
            runtime.WriteByte(state_.data + new_length, 0u);
            return *this;
        }
    }
    if (new_length == 0u) {
        if (state_.data != 0u) {
            state_.length = 0u;
            runtime.WriteByte(state_.data, 0u);
        }
        return *this;
    }
    if (state_.capacity < new_length) {
        runtime.Reallocate(state_, new_length);
    }
    state_.length = new_length;
    runtime.WriteByte(state_.data + new_length, 0u);
    return *this;
}

const StringBufferState& StringBuffer::State() const noexcept {
    return state_;
}

}  // namespace pal4
