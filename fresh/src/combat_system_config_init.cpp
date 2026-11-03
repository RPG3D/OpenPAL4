#include "pal4/combat_system_config_init.h"

#include "pal4/string_buffer.h"

namespace pal4 {

std::uint32_t CombatSystemConfigInit::Initialize(
    const std::uint32_t object, const std::uint8_t seed,
    ICombatSystemConfigInitRuntime& runtime) {
    runtime.WriteByte(object + 20u, seed);
    runtime.WriteWord(object + 24u, 0u);
    runtime.WriteWord(object + 28u, 0u);
    runtime.WriteWord(object + 32u, 0u);
    runtime.WriteByte(object + 36u, seed);
    runtime.WriteWord(object + 40u, 0u);
    runtime.WriteWord(object + 44u, 0u);
    runtime.WriteWord(object + 48u, 0u);
    runtime.WriteWord(object + 56u, 0u);
    runtime.WriteByte(object + 52u, seed);
    runtime.WriteWord(object + 60u, 0u);
    runtime.WriteWord(object + 64u, 0u);
    static_cast<void>(InitializeStrings(object, runtime));
    return object;
}

std::uint8_t CombatSystemConfigInit::AssignErrorString(
    const std::uint32_t record, const std::uint32_t length,
    ICombatSystemConfigInitRuntime& runtime) {
    if (length > StringBuffer::MaxSize()) runtime.ThrowLength();
    const auto data = runtime.ReadWord(record + 4u);
    if (data != 0u) {
        const auto references = runtime.ReadByte(data - 1u);
        if (references != 0u && references != 0xffu) {
            if (length == 0u) {
                runtime.WriteByte(data - 1u,
                                  static_cast<std::uint8_t>(references - 1u));
                return runtime.SetErrorFlag(record, 0u);
            }
            runtime.ReallocateText(record, length);
        } else if (length == 0u) {
            return runtime.SetErrorFlag(record, 1u);
        } else {
            const auto capacity = runtime.ReadWord(record + 12u);
            if (capacity > 31u || capacity < length) {
                static_cast<void>(runtime.SetErrorFlag(record, 1u));
                runtime.ReallocateText(record, length);
            }
        }
    } else if (length == 0u) {
        return runtime.SetErrorFlag(record, 1u);
    } else {
        const auto capacity = runtime.ReadWord(record + 12u);
        if (capacity > 31u || capacity < length) {
            static_cast<void>(runtime.SetErrorFlag(record, 1u));
            runtime.ReallocateText(record, length);
        }
    }
    const auto destination = runtime.ReadWord(record + 4u);
    runtime.CopyBytes(destination, kErrorString, length);
    runtime.WriteWord(record + 8u, length);
    runtime.WriteByte(destination + length, 0u);
    return static_cast<std::uint8_t>(length);
}

std::uint8_t CombatSystemConfigInit::InitializeStrings(
    const std::uint32_t object, ICombatSystemConfigInitRuntime& runtime) {
    runtime.WriteWord(object + 12u, 0u);
    runtime.WriteWord(object, 0u);
    runtime.WriteWord(object + 4u, 0u);
    runtime.WriteWord(object + 8u, 0u);
    runtime.WriteWord(object + 16u, 1u);

    const auto first_length = runtime.StringLength(kErrorString);
    if (runtime.AllocateText(object + 20u, first_length, 1u)) {
        const auto data = runtime.ReadWord(object + 24u);
        runtime.CopyBytes(data, kErrorString, first_length);
        runtime.WriteWord(object + 28u, first_length);
        runtime.WriteByte(data + first_length, 0u);
    }
    static_cast<void>(runtime.AssignErrorText(
        object + 36u, runtime.StringLength(kErrorString)));
    const auto result = runtime.AssignErrorText(
        object + 52u, runtime.StringLength(kErrorString));
    runtime.WriteByte(object + 68u, 1u);
    runtime.WriteByte(object + 69u, 0u);
    runtime.WriteByte(object + 70u, 0u);
    return result;
}

std::uint8_t ICombatSystemConfigInitRuntime::AssignErrorText(std::uint32_t record,std::uint32_t length){
    return CombatSystemConfigInit::AssignErrorString(record,length,*this);
}

}  // namespace pal4
