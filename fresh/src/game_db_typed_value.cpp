#include "pal4/game_db_typed_value.h"
#include "pal4/legacy_crc32.h"

#include <bit>
#include <cstring>
#include <limits>
#include <string>

namespace pal4 {

GameDbTypedValueState& GameDbTypedValue::Construct(
    GameDbTypedValueState& destination,
    const GameDbTypedValueState& source,
    IGameDbTypedValueRuntime& runtime) {
    destination = {};
    return AssignFrom(destination, source, runtime);
}

void GameDbTypedValue::Destroy(GameDbTypedValueState& state,
                               IGameDbTypedValueRuntime& runtime) {
    Clear(state, runtime);
}

void GameDbTypedValue::Clear(GameDbTypedValueState& state,
                             IGameDbTypedValueRuntime& runtime) {
    if (state.type == 2u && state.value != 0u)
        runtime.DestroyString(state.value);
    state = {};
}

GameDbTypedValueState& GameDbTypedValue::AssignFrom(
    GameDbTypedValueState& destination,
    const GameDbTypedValueState& source,
    IGameDbTypedValueRuntime& runtime) {
    Clear(destination, runtime);
    switch (source.type) {
    case 1u:
        (void)SetInteger(destination, GetInteger(source), runtime);
        break;
    case 2u: {
        const auto* text = runtime.StringData(source.value);
        (void)SetString(destination, text == nullptr ? "" : text, runtime);
        break;
    }
    case 3u:
        (void)SetFloat(destination, std::bit_cast<float>(source.value),
                       runtime);
        break;
    default:
        break;
    }
    return destination;
}

std::int32_t GameDbTypedValue::GetInteger(
    const GameDbTypedValueState& state) {
    return state.type == 1u ? std::bit_cast<std::int32_t>(state.value)
                            : std::bit_cast<std::int32_t>(0xcdcdcdcdu);
}

std::uint32_t GameDbTypedValue::GetStringObject(
    const GameDbTypedValueState& value,
    GameDbTypedValueErrorState& fallback,
    const std::uint8_t source_high_byte,
    IGameDbTypedValueErrorRuntime& runtime) {
    if (value.type == 2u)
        return value.value;
    if (!fallback.initialized) {
        fallback.initialized = true;
        fallback.flag = source_high_byte;
        fallback.data = fallback.length = fallback.capacity = 0u;
        constexpr char message[] = "dbField::asString Error!!";
        constexpr auto length = static_cast<std::uint32_t>(sizeof(message) - 1u);
        runtime.SetErrorFlag(fallback, 1u);
        fallback.data = runtime.AllocateText(length);
        if (fallback.data != 0u) {
            runtime.CopyText(fallback.data, message, length);
            fallback.length = fallback.capacity = length;
        }
        runtime.RegisterCleanup();
    }
    return 0x93ceccu;
}

void GameDbTypedValue::DeinitializeErrorString(
    GameDbTypedValueErrorState& fallback,
    IGameDbTypedValueErrorRuntime& runtime) {
    if (fallback.data != 0u)
        runtime.ReleaseText(fallback.data);
    fallback.data = fallback.length = fallback.capacity = 0u;
}

float GameDbTypedValue::GetFloat(const GameDbTypedValueState& state) {
    return state.type == 3u ? std::bit_cast<float>(state.value) : 0.0f;
}

std::int32_t GameDbTypedValue::SetInteger(
    GameDbTypedValueState& state, const std::int32_t number,
    IGameDbTypedValueRuntime& runtime) {
    Clear(state, runtime);
    state.type = 1u;
    state.value = std::bit_cast<std::uint32_t>(number);
    return number;
}

std::uint8_t GameDbTypedValue::SetString(
    GameDbTypedValueState& state, const char* text,
    IGameDbTypedValueRuntime& runtime) {
    Clear(state, runtime);
    state.type = 2u;
    const auto address = runtime.AllocateStringObject(16u);
    if (address == 0u) {
        state.value = 0u;
        return 0u;
    }
    auto& object = runtime.StringObject(address);
    object.flag = (object.flag & 0xffffff00u) |
                  static_cast<std::uint8_t>(
                      reinterpret_cast<std::uintptr_t>(text));
    object.data = object.length = object.capacity = 0u;
    const auto length = static_cast<std::uint32_t>(std::strlen(text));
    const auto result = runtime.SetErrorFlag(object, 1u);
    if (length == 0u) {
        state.value = address;
        return result;
    }
    const auto data = runtime.AllocateStringText(length);
    runtime.CopyStringText(data, text, length);
    object.data = data;
    object.length = object.capacity = length;
    state.value = address;
    return static_cast<std::uint8_t>(data);
}

std::uint32_t GameDbTypedValue::SetFloat(
    GameDbTypedValueState& state, const float number,
    IGameDbTypedValueRuntime& runtime) {
    Clear(state, runtime);
    state.type = 3u;
    state.value = std::bit_cast<std::uint32_t>(number);
    return state.value;
}

void GameDbTypedValue::ApplyBounds(
    GameDbTypedValueState& state,
    const GameDbValueBoundsState& bounds,
    IGameDbTypedValueRuntime& runtime) {
    if (bounds.minimum == 0 && bounds.maximum == 0)
        return;
    switch (state.type) {
    case 1u: {
        const auto value = GetInteger(state);
        if (value > bounds.maximum)
            (void)SetInteger(state, bounds.maximum, runtime);
        else if (value < bounds.minimum)
            (void)SetInteger(state, bounds.minimum, runtime);
        break;
    }
    case 2u:
        if (state.value != 0u && bounds.maximum > 0 &&
            runtime.StringLength(state.value) >
                static_cast<std::uint32_t>(bounds.maximum)) {
            const auto* data = runtime.StringData(state.value);
            std::string prefix = data == nullptr ? "" : data;
            prefix.resize(static_cast<std::size_t>(bounds.maximum), '\0');
            (void)SetString(state, prefix.c_str(), runtime);
        }
        break;
    case 3u: {
        const auto value = GetFloat(state);
        const auto maximum = static_cast<float>(bounds.maximum);
        const auto minimum = static_cast<float>(bounds.minimum);
        if (value > maximum)
            (void)SetFloat(state, maximum, runtime);
        else if (value < minimum)
            (void)SetFloat(state, minimum, runtime);
        break;
    }
    default:
        break;
    }
}

GameDbValueArrayState& GameDbValueArray::Construct(
    GameDbValueArrayState& state, const std::int32_t count,
    IGameDbValueArrayRuntime& runtime) {
    state.flag = (state.flag & 0xffffff00u) |
                 (static_cast<std::uint32_t>(count) & 0xffu);
    state.begin = state.end = state.capacity = 0u;
    if (count != 0) {
        const auto slots = count < 0 ? 0u
                                     : static_cast<std::uint32_t>(count);
        const auto address = runtime.Allocate(slots * 8u);
        runtime.Free(0u);
        state.begin = state.end = address;
        state.capacity = address + static_cast<std::uint32_t>(count) * 8u;
    }
    return state;
}

std::uint32_t GameDbValueArray::CleanupProperties(
    GameDbValueArrayState& state,
    IGameDbValueArrayMutationRuntime& runtime) {
    return Clear(state, runtime);
}

std::int32_t GameDbValueArray::GetIntegerAt(
    const GameDbValueArrayState& state, const std::uint32_t index,
    IGameDbValueArrayLookupRuntime& runtime) {
    if (state.begin == 0 || index >= (state.end - state.begin) / 8u)
        return -1;
    return GameDbTypedValue::GetInteger(
        runtime.ValueAt(state.begin + index * 8u));
}

std::uint32_t GameDbValueArray::GetStringObjectAt(
    const GameDbValueArrayState& state, const std::uint32_t index,
    GameDbTypedValueErrorState& row_fallback,
    GameDbTypedValueErrorState& value_fallback,
    IGameDbValueArrayLookupRuntime& runtime) {
    const auto begin = state.begin;
    if (begin != 0u && index < (state.end - begin) / 8u) {
        const auto address = begin + index * 8u;
        return GameDbTypedValue::GetStringObject(
            runtime.ValueAt(address), value_fallback,
            static_cast<std::uint8_t>(address >> 24u), runtime);
    }
    if (!row_fallback.initialized) {
        row_fallback.initialized = true;
        row_fallback.flag = static_cast<std::uint8_t>(index);
        row_fallback.data = row_fallback.length = row_fallback.capacity = 0u;
        constexpr char message[] = "dbRow_Error";
        constexpr auto length = static_cast<std::uint32_t>(sizeof(message) - 1u);
        row_fallback.data = runtime.AllocateText(length);
        if (row_fallback.data != 0u) {
            runtime.CopyText(row_fallback.data, message, length);
            row_fallback.length = row_fallback.capacity = length;
        }
        runtime.RegisterCleanup();
    }
    return 0x93cee4u;
}

void GameDbValueArray::DeinitializeRowError(
    GameDbTypedValueErrorState& row_fallback,
    IGameDbTypedValueErrorRuntime& runtime) {
    if (row_fallback.data != 0u)
        runtime.ReleaseText(row_fallback.data);
    row_fallback.data = row_fallback.length = row_fallback.capacity = 0u;
}

float GameDbValueArray::GetFloatAt(
    const GameDbValueArrayState& state, const std::uint32_t index,
    IGameDbValueArrayLookupRuntime& runtime) {
    const auto begin = state.begin;
    return begin != 0u && index < (state.end - begin) / 8u
               ? GameDbTypedValue::GetFloat(
                     runtime.ValueAt(begin + index * 8u))
               : 0.0f;
}

std::uint32_t GameDbValueArray::EntryAddressAt(
    const GameDbValueArrayState& state, const std::int32_t index) {
    return state.begin + static_cast<std::uint32_t>(index) * 8u;
}

bool GameDbValueArray::SetAt(
    GameDbValueArrayState& state, const std::uint32_t index,
    const GameDbTypedValueState& value,
    const GameDbValueBoundsState& bounds,
    IGameDbValueArrayLookupRuntime& runtime) {
    const auto count = state.begin == 0u
                           ? 0u : (state.end - state.begin) / 8u;
    if (index > count)
        return false;
    auto& destination = runtime.ValueAt(state.begin + index * 8u);
    (void)GameDbTypedValue::AssignFrom(destination, value, runtime);
    GameDbTypedValue::ApplyBounds(destination, bounds, runtime);
    return true;
}

std::int32_t GameDbValueArray::GetValueAt(
    const GameDbValueArrayState& state, const std::uint32_t index,
    IGameDbValueArrayLookupRuntime& runtime, LegacyCrc32& crc) {
    GameDbTypedValueState temporary{};
    const auto begin = state.begin;
    if (begin == 0u || index >= (state.end - begin) / 8u)
        return -1;
    (void)GameDbTypedValue::AssignFrom(
        temporary, runtime.ValueAt(begin + index * 8u), runtime);
    std::int32_t result = -1;
    switch (temporary.type) {
    case 1u:
        result = GameDbTypedValue::GetInteger(temporary);
        break;
    case 2u: {
        const char* text = runtime.StringData(temporary.value);
        if (text == nullptr)
            text = "";
        result = std::bit_cast<std::int32_t>(crc.ComputeAndStore(
            reinterpret_cast<const std::uint8_t*>(text),
            static_cast<std::uint32_t>(std::strlen(text))));
        break;
    }
    case 3u: {
        const double number = GameDbTypedValue::GetFloat(temporary);
        constexpr double lower = -2147483648.0;
        constexpr double upper = 2147483648.0;
        result = number >= lower && number < upper
                     ? static_cast<std::int32_t>(number)
                     : std::numeric_limits<std::int32_t>::min();
        break;
    }
    default:
        break;
    }
    GameDbTypedValue::Destroy(temporary, runtime);
    return result;
}

void GameDbValueArray::Add(
    GameDbValueArrayState& state, const GameDbTypedValueState& value,
    const GameDbValueBoundsState& bounds,
    IGameDbValueArrayMutationRuntime& runtime) {
    (void)Insert(state, state.end, 1u, value, runtime);
    const auto last = state.begin != 0u
                          ? state.end - 8u : 0xfffffff8u;
    GameDbTypedValue::ApplyBounds(runtime.ValueAt(last), bounds, runtime);
}

std::uint32_t GameDbValueArray::Clear(
    GameDbValueArrayState& state,
    IGameDbValueArrayMutationRuntime& runtime) {
    DestroyRange(state.begin, state.end, runtime);
    runtime.Free(state.begin);
    state.begin = state.end = state.capacity = 0u;
    return 0u;
}

std::uint32_t GameDbValueArray::Insert(
    GameDbValueArrayState& state, const std::uint32_t position,
    const std::uint32_t count, const GameDbTypedValueState& value,
    IGameDbValueArrayMutationRuntime& runtime) {
    const auto bytes = count * 8u;
    if ((state.capacity - state.end) / 8u >= count) {
        const auto tail = (state.end - position) / 8u;
        if (tail < count) {
            for (auto source = position; source != state.end; source += 8u)
                (void)GameDbTypedValue::Construct(
                    runtime.ValueAt(source + bytes), runtime.ValueAt(source),
                    runtime);
            for (auto slot = state.end;
                 slot != state.end + (count - tail) * 8u; slot += 8u)
                (void)GameDbTypedValue::Construct(
                    runtime.ValueAt(slot), value, runtime);
            for (auto slot = position; slot != state.end; slot += 8u)
                (void)GameDbTypedValue::AssignFrom(
                    runtime.ValueAt(slot), value, runtime);
        } else if (count != 0u) {
            for (auto source = state.end - bytes;
                 source != state.end; source += 8u)
                (void)GameDbTypedValue::Construct(
                    runtime.ValueAt(source + bytes), runtime.ValueAt(source),
                    runtime);
            for (auto destination = state.end - bytes;
                 destination != position;) {
                destination -= 8u;
                (void)GameDbTypedValue::AssignFrom(
                    runtime.ValueAt(destination + bytes),
                    runtime.ValueAt(destination), runtime);
            }
            for (auto destination = position;
                 destination != position + bytes; destination += 8u)
                (void)GameDbTypedValue::AssignFrom(
                    runtime.ValueAt(destination), value, runtime);
        }
        state.end += bytes;
        return bytes;
    }

    const auto old_begin = state.begin;
    const auto old_end = state.end;
    const auto old_count = old_begin == 0u ? 0u : (old_end - old_begin) / 8u;
    const auto new_count = old_count + (count < old_count ? old_count : count);
    const auto memory = runtime.Allocate(new_count * 8u);
    auto destination = memory;
    for (auto source = old_begin; source != position; source += 8u) {
        (void)GameDbTypedValue::Construct(
            runtime.ValueAt(destination), runtime.ValueAt(source), runtime);
        destination += 8u;
    }
    for (auto i = 0u; i < count; ++i) {
        (void)GameDbTypedValue::Construct(
            runtime.ValueAt(destination), value, runtime);
        destination += 8u;
    }
    for (auto source = position; source != old_end; source += 8u) {
        (void)GameDbTypedValue::Construct(
            runtime.ValueAt(destination), runtime.ValueAt(source), runtime);
        destination += 8u;
    }
    for (auto source = old_begin; source != old_end; source += 8u)
        GameDbTypedValue::Destroy(runtime.ValueAt(source), runtime);
    runtime.Free(old_begin);
    state.begin = memory;
    state.end = memory + (old_count + count) * 8u;
    state.capacity = memory + new_count * 8u;
    return state.end;
}

void GameDbValueArray::DestroyRange(
    const std::uint32_t begin, const std::uint32_t end,
    IGameDbValueArrayLookupRuntime& runtime) {
    for (auto address = begin; address != end; address += 8u)
        GameDbTypedValue::Destroy(runtime.ValueAt(address), runtime);
}

std::uint32_t GameDbValueArray::CopyRange(
    const std::uint32_t source_begin, const std::uint32_t source_end,
    std::uint32_t destination, IGameDbValueArrayLookupRuntime& runtime) {
    for (auto source = source_begin; source != source_end; source += 8u) {
        (void)ConstructAt(destination, runtime.ValueAt(source), runtime);
        destination += 8u;
    }
    return destination;
}

std::uint32_t GameDbValueArray::ConstructAt(
    const std::uint32_t destination, const GameDbTypedValueState& value,
    IGameDbValueArrayLookupRuntime& runtime) {
    if (destination != 0u)
        (void)GameDbTypedValue::Construct(
            runtime.ValueAt(destination), value, runtime);
    return destination;
}

}  // namespace pal4
