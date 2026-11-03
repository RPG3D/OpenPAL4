#include "pal4/xml_attribute_record.h"
#include "pal4/string_buffer.h"

#include <bit>
#include <new>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4 {

namespace {

std::uint32_t BeginNamedRecord(const std::uint32_t record,
                               const std::uint8_t marker,
                               const std::uint32_t type,
                               const std::uint32_t name,
                               IXmlAttributeRecordRuntime& runtime) {
    runtime.WriteByte(record, marker);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    if (name == 0u) throw std::runtime_error("XmlAttribute's name is null!");

    runtime.WriteWord(record + 16u, type);
    std::uint32_t length = 0u;
    while (runtime.ReadByte(name + length) != 0u) ++length;
    return length;
}

void CopyAllocatedName(const std::uint32_t record,
                       const std::uint32_t name,
                       const std::uint32_t length,
                       IXmlAttributeRecordRuntime& runtime) {
    if (!runtime.AllocateText(record, length, true)) return;
    const auto destination = runtime.ReadWord(record + 4u);
    for (std::uint32_t index = 0u; index < length; ++index)
        runtime.WriteByte(destination + index, runtime.ReadByte(name + index));
    runtime.WriteWord(record + 8u, length);
    runtime.WriteByte(destination + length, 0u);
}

std::uint32_t InitializePointerValue(
    const std::uint32_t record, const std::uint32_t name,
    const std::uint32_t value_address, const std::uint32_t type,
    IXmlAttributeRecordRuntime& runtime) {
    runtime.WriteByte(record, static_cast<std::uint8_t>(name));
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    if (name == 0u) throw std::runtime_error("XmlAttribute's name is null!");

    runtime.WriteWord(record + 16u, type);
    std::uint32_t length = 0u;
    while (runtime.ReadByte(name + length) != 0u) {
        if (length == std::numeric_limits<std::uint32_t>::max() - 2u)
            throw std::length_error("XmlAttribute name too long");
        ++length;
    }
    runtime.SetErrorFlag(record, 1u);
    if (length != 0u) {
        runtime.Reallocate(record, length);
        const auto destination = runtime.ReadWord(record + 4u);
        for (std::uint32_t index = 0u; index < length; ++index)
            runtime.WriteByte(destination + index,
                              runtime.ReadByte(name + index));
        runtime.WriteWord(record + 8u, length);
        runtime.WriteByte(destination + length, 0u);
    }
    runtime.WriteWord(record + 20u, 0u);
    runtime.WriteWord(record + 24u, runtime.ReadWord(value_address));
    return record;
}

}  // namespace

std::uint32_t XmlAttributeRecord::Initialize(
    const std::uint32_t record, IXmlAttributeRecordRuntime& runtime) {
    constexpr std::string_view kUnknownName = "ATTR_NAME_UNKNOW";
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    runtime.WriteWord(record + 16u, 4u);
    runtime.SetErrorFlag(record, 1u);
    runtime.Reallocate(record, static_cast<std::uint32_t>(kUnknownName.size()));
    const auto data = runtime.ReadWord(record + 4u);
    if (data == 0u) throw std::bad_alloc{};
    for (std::size_t index = 0u; index < kUnknownName.size(); ++index)
        runtime.WriteByte(data + static_cast<std::uint32_t>(index),
                          static_cast<std::uint8_t>(kUnknownName[index]));
    runtime.WriteWord(record + 8u,
                      static_cast<std::uint32_t>(kUnknownName.size()));
    runtime.WriteByte(data + static_cast<std::uint32_t>(kUnknownName.size()),
                      0u);
    runtime.WriteWord(record + 20u, 0u);
    runtime.WriteWord(record + 24u, 0u);
    return record;
}

std::uint32_t XmlAttributeRecord::InitializeWithValue(
    const std::uint32_t record, const std::uint32_t type,
    const std::uint32_t name, IXmlAttributeRecordRuntime& runtime) {
    const auto length = BeginNamedRecord(
        record, static_cast<std::uint8_t>(type), type, name, runtime);
    CopyAllocatedName(record, name, length, runtime);

    runtime.WriteWord(record + 20u, 0u);
    runtime.WriteWord(record + 24u, 0u);
    if (type == 3u) {
        const auto child = runtime.AllocateMemory(16u);
        if (child != 0u) {
            runtime.WriteWord(child + 4u, 0u);
            runtime.WriteByte(child, static_cast<std::uint8_t>(type));
            runtime.WriteWord(child + 8u, 0u);
            runtime.WriteWord(child + 12u, 0u);
        }
        runtime.WriteWord(record + 24u, child);
    }
    return record;
}

std::uint32_t XmlAttributeRecord::InitializeWithPointerValue(
    const std::uint32_t record, const std::uint32_t name,
    const std::uint32_t value_address, IXmlAttributeRecordRuntime& runtime) {
    return InitializePointerValue(record, name, value_address, 1u, runtime);
}

std::uint32_t XmlAttributeRecord::InitializeWithPointerValue2(
    const std::uint32_t record, const std::uint32_t name,
    const std::uint32_t value_address, IXmlAttributeRecordRuntime& runtime) {
    return InitializePointerValue(record, name, value_address, 2u, runtime);
}

std::uint32_t XmlAttributeRecord::InitializeWithValueAndType(
    const std::uint32_t record, const std::uint32_t name,
    const std::uint32_t value, IXmlAttributeRecordRuntime& runtime) {
    const auto marker = static_cast<std::uint8_t>(name);
    const auto name_length = BeginNamedRecord(record, marker, 3u, name, runtime);
    CopyAllocatedName(record, name, name_length, runtime);

    runtime.WriteWord(record + 20u, 0u);
    const auto child = runtime.AllocateMemory(16u);
    if (child == 0u) throw std::bad_alloc{};
    runtime.WriteWord(child + 4u, 0u);
    runtime.WriteWord(child + 8u, 0u);
    runtime.WriteWord(child + 12u, 0u);
    runtime.WriteByte(child, marker);
    runtime.WriteWord(record + 24u, child);

    std::uint32_t value_length = 0u;
    if (value != 0u) {
        while (runtime.ReadByte(value + value_length) != 0u) {
            if (value_length == std::numeric_limits<std::uint32_t>::max() - 2u)
                throw std::length_error("XmlAttribute value too long");
            ++value_length;
        }
    }
    runtime.SetErrorFlag(child, 1u);
    if (value_length != 0u) {
        runtime.Reallocate(child, value_length);
        const auto destination = runtime.ReadWord(child + 4u);
        for (std::uint32_t index = 0u; index < value_length; ++index)
            runtime.WriteByte(destination + index,
                              runtime.ReadByte(value + index));
        runtime.WriteWord(child + 8u, value_length);
        runtime.WriteByte(destination + value_length, 0u);
    }
    return record;
}

namespace {

std::uint32_t CopyValue(
    const std::uint32_t destination, const std::uint32_t source,
    const std::uint32_t type,
    IXmlAttributeRecordRuntime& runtime) {
    switch (type) {
    case 0u:
        runtime.WriteByte(destination + 24u, runtime.ReadByte(source + 24u));
        return destination;
    case 1u:
    case 2u:
        runtime.WriteWord(destination + 24u, runtime.ReadWord(source + 24u));
        return destination;
    case 3u: {
        const auto child = runtime.AllocateMemory(16u);
        if (child == 0u) throw std::bad_alloc{};
        runtime.WriteWord(child + 4u, 0u);
        runtime.WriteByte(child, static_cast<std::uint8_t>(source));
        runtime.WriteWord(child + 8u, 0u);
        runtime.WriteWord(child + 12u, 0u);
        runtime.WriteWord(destination + 24u, child);

        const auto source_child = runtime.ReadWord(source + 24u);
        const auto source_text = runtime.ReadWord(source_child + 4u);
        std::uint32_t length = 0u;
        if (source_text != 0u) {
            while (runtime.ReadByte(source_text + length) != 0u) {
                if (length == std::numeric_limits<std::uint32_t>::max() - 2u)
                    throw std::length_error("XmlAttribute child text too long");
                ++length;
            }
        }
        runtime.SetErrorFlag(child, 1u);
        if (length != 0u) {
            runtime.Reallocate(child, length);
            const auto target_text = runtime.ReadWord(child + 4u);
            for (std::uint32_t index = 0u; index < length; ++index)
                runtime.WriteByte(target_text + index,
                                  runtime.ReadByte(source_text + index));
            runtime.WriteWord(child + 8u, length);
            runtime.WriteByte(target_text + length, 0u);
        }
        return destination;
    }
    default: {
        const auto raw_size = runtime.ReadWord(source + 20u);
        runtime.WriteWord(destination + 20u, raw_size);
        if (static_cast<std::int32_t>(raw_size) > 0) {
            const auto raw = runtime.AllocateRaw(raw_size);
            if (raw == 0u) throw std::bad_alloc{};
            runtime.WriteWord(destination + 24u, raw);
            const auto source_raw = runtime.ReadWord(source + 24u);
            for (std::uint32_t index = 0u; index < raw_size; ++index)
                runtime.WriteByte(raw + index,
                                  runtime.ReadByte(source_raw + index));
        }
        return destination;
    }
    }
}

}  // namespace

std::uint32_t XmlAttributeRecord::CopyConstruct(
    const std::uint32_t destination, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    runtime.WriteByte(destination, static_cast<std::uint8_t>(source));
    runtime.WriteWord(destination + 4u, 0u);
    runtime.WriteWord(destination + 8u, 0u);
    runtime.WriteWord(destination + 12u, 0u);
    const auto type = runtime.ReadWord(source + 16u);
    runtime.WriteWord(destination + 16u, type);
    runtime.AssignName(destination, source, 0u, 0xffffffffu);
    return CopyValue(destination, source, type, runtime);
}

std::uint32_t XmlAttributeRecord::FreeValue(
    const std::uint32_t record, IXmlAttributeRecordRuntime& runtime) {
    const auto type = runtime.ReadWord(record + 16u);
    if (type == 3u) {
        const auto child = runtime.ReadWord(record + 24u);
        std::uint32_t result = type;
        if (child != 0u) {
            const auto text = runtime.ReadWord(child + 4u);
            if (text != 0u) {
                const auto header = text - 1u;
                const auto references = runtime.ReadByte(header);
                if (references == 0u || references == 0xffu)
                    (void)runtime.FreeBlock(header);
                else
                    runtime.WriteByte(header,
                                      static_cast<std::uint8_t>(references - 1u));
            }
            runtime.WriteWord(child + 4u, 0u);
            runtime.WriteWord(child + 8u, 0u);
            runtime.WriteWord(child + 12u, 0u);
            result = runtime.FreeBlock(child);
        }
        runtime.WriteWord(record + 24u, 0u);
        return result;
    }
    if (type == 4u) {
        const auto size = runtime.ReadWord(record + 20u);
        if (static_cast<std::int32_t>(size) <= 0) {
            runtime.WriteWord(record + 24u, 0u);
            return size;
        }
        const auto result = runtime.FreeRaw(runtime.ReadWord(record + 24u));
        runtime.WriteWord(record + 24u, 0u);
        runtime.WriteWord(record + 20u, 0u);
        return result;
    }
    runtime.WriteWord(record + 24u, 0u);
    return type;
}

std::uint32_t XmlAttributeRecord::Destroy(
    const std::uint32_t record, IXmlAttributeRecordRuntime& runtime) {
    (void)FreeValue(record, runtime);
    const auto text = runtime.ReadWord(record + 4u);
    std::uint32_t result = 0u;
    if (text != 0u) {
        const auto header = text - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu) {
            result = runtime.FreeBlock(header);
        } else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
    return result;
}

std::uint32_t XmlAttributeRecord::ValueSize(
    const std::uint32_t record, IXmlAttributeRecordRuntime& runtime) {
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        return 1u;
    case 1u:
    case 2u:
        return 4u;
    case 3u:
        return runtime.ReadWord(runtime.ReadWord(record + 24u) + 8u);
    default:
        return runtime.ReadWord(record + 20u);
    }
}

std::uint32_t XmlAttributeRecord::Assign(
    const std::uint32_t destination, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    (void)FreeValue(destination, runtime);
    const auto type = runtime.ReadWord(source + 16u);
    runtime.WriteWord(destination + 16u, type);
    runtime.AssignName(destination, source, 0u, 0xffffffffu);
    return CopyValue(destination, source, type, runtime);
}

namespace {

std::uint32_t RawOrFallbackBits(const std::uint32_t record,
                                std::uint32_t fallback,
                                IXmlAttributeRecordRuntime& runtime) {
    const auto size = runtime.ReadWord(record + 20u);
    if (static_cast<std::int32_t>(size) <= 0) return fallback;
    const auto raw = runtime.ReadWord(record + 24u);
    if (size >= 4u) return runtime.ReadWord(raw);
    for (std::uint32_t index = 0u; index < size; ++index) {
        const auto shift = index * 8u;
        fallback = (fallback & ~(0xffu << shift)) |
                   (std::uint32_t{runtime.ReadByte(raw + index)} << shift);
    }
    return fallback;
}

std::uint32_t ChildText(const std::uint32_t record,
                        IXmlAttributeRecordRuntime& runtime) {
    return runtime.ReadWord(runtime.ReadWord(record + 24u) + 4u);
}

std::string ReadCString(const std::uint32_t address,
                        IXmlAttributeRecordRuntime& runtime) {
    std::string text;
    if (address == 0u) return text;
    for (std::uint32_t index = 0u;; ++index) {
        const auto character = runtime.ReadByte(address + index);
        if (character == 0u) break;
        text.push_back(static_cast<char>(character));
    }
    return text;
}

}  // namespace

std::uint8_t XmlAttributeRecord::ToChar(
    const std::uint32_t record, const std::uint32_t fallback_address,
    IXmlAttributeRecordRuntime& runtime) {
    const auto fallback = runtime.ReadByte(fallback_address);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
    case 1u:
        return runtime.ReadByte(record + 24u);
    case 2u:
        return static_cast<std::uint8_t>(
            runtime.FloatToInt(runtime.ReadWord(record + 24u)));
    case 3u:
        return runtime.ScanChar(ChildText(record, runtime), fallback);
    default:
        if (static_cast<std::int32_t>(runtime.ReadWord(record + 20u)) > 0)
            return runtime.ReadByte(runtime.ReadWord(record + 24u));
        return fallback;
    }
}

std::int32_t XmlAttributeRecord::ToInt(
    const std::uint32_t record, const std::uint32_t fallback_address,
    IXmlAttributeRecordRuntime& runtime) {
    const auto fallback = runtime.ReadWord(fallback_address);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        return static_cast<std::int8_t>(runtime.ReadByte(record + 24u));
    case 1u:
        return static_cast<std::int32_t>(runtime.ReadWord(record + 24u));
    case 2u:
        return runtime.FloatToInt(runtime.ReadWord(record + 24u));
    case 3u:
        return runtime.ScanInt(ChildText(record, runtime),
                               static_cast<std::int32_t>(fallback));
    default:
        return static_cast<std::int32_t>(
            RawOrFallbackBits(record, fallback, runtime));
    }
}

double XmlAttributeRecord::ToFloat(
    const std::uint32_t record, const std::uint32_t fallback_address,
    IXmlAttributeRecordRuntime& runtime) {
    const auto fallback = runtime.ReadWord(fallback_address);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        return static_cast<double>(
            static_cast<std::int8_t>(runtime.ReadByte(record + 24u)));
    case 1u:
        return static_cast<double>(
            static_cast<std::int32_t>(runtime.ReadWord(record + 24u)));
    case 2u:
        return static_cast<double>(
            std::bit_cast<float>(runtime.ReadWord(record + 24u)));
    case 3u:
        return static_cast<double>(std::bit_cast<float>(
            runtime.ScanFloatBits(ChildText(record, runtime), fallback)));
    default:
        return static_cast<double>(std::bit_cast<float>(
            RawOrFallbackBits(record, fallback, runtime)));
    }
}

std::uint32_t XmlAttributeRecord::ToString(
    const std::uint32_t record, const std::uint32_t output,
    const std::uint32_t fallback_address,
    IXmlAttributeRecordRuntime& runtime) {
    if (fallback_address == 0u)
        throw std::invalid_argument("XmlAttribute string fallback is null");
    std::string text = ReadCString(fallback_address, runtime);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u: {
        const auto value = runtime.ReadByte(record + 24u);
        text = value == 0u ? std::string{} :
                             std::string(1u, static_cast<char>(value));
        break;
    }
    case 1u:
        text = std::to_string(
            static_cast<std::int32_t>(runtime.ReadWord(record + 24u)));
        break;
    case 2u:
        text = runtime.FormatFloat(runtime.ReadWord(record + 24u));
        break;
    case 3u:
        text = ReadCString(ChildText(record, runtime), runtime);
        break;
    default: {
        const auto size = runtime.ReadWord(record + 20u);
        if (static_cast<std::int32_t>(size) > 0) {
            const auto allocated = runtime.AllocateMemory(size);
            if (allocated == 0u) throw std::bad_alloc{};
            if (size > 4u)
                throw std::length_error(
                    "XmlAttribute raw string overwrites the stack pointer slot");
            auto pointer = allocated;
            const auto raw = runtime.ReadWord(record + 24u);
            for (std::uint32_t index = 0u; index < size; ++index) {
                const auto shift = index * 8u;
                pointer = (pointer & ~(0xffu << shift)) |
                          (std::uint32_t{runtime.ReadByte(raw + index)} << shift);
            }
            text = ReadCString(pointer, runtime);
            (void)runtime.FreeBlock(pointer);
        }
        break;
    }
    }

    runtime.WriteByte(output, static_cast<std::uint8_t>(fallback_address));
    runtime.WriteWord(output + 4u, 0u);
    runtime.WriteWord(output + 8u, 0u);
    runtime.WriteWord(output + 12u, 0u);
    const auto length = static_cast<std::uint32_t>(text.size());
    if (runtime.AllocateText(output, length, true)) {
        const auto destination = runtime.ReadWord(output + 4u);
        for (std::uint32_t index = 0u; index < length; ++index)
            runtime.WriteByte(destination + index,
                              static_cast<std::uint8_t>(text[index]));
        runtime.WriteWord(output + 8u, length);
        runtime.WriteByte(destination + length, 0u);
    }
    return output;
}

std::uint32_t XmlAttributeRecord::GetValue(
    const std::uint32_t record, const std::uint32_t output,
    const std::uint32_t capacity_address,
    IXmlAttributeRecordRuntime& runtime) {
    const auto type = runtime.ReadWord(record + 16u);
    if (type == 0u) {
        runtime.WriteByte(output, runtime.ReadByte(record + 24u));
        return 1u;
    }
    if (type == 1u || type == 2u) {
        runtime.WriteWord(output, runtime.ReadWord(record + 24u));
        return 4u;
    }
    if (type == 3u) {
        const auto child = runtime.ReadWord(record + 24u);
        const auto size = runtime.ReadWord(child + 8u);
        auto source = runtime.ReadWord(child + 4u);
        if (source == 0u) source = StringBuffer::EmptyLogMessageAddress();
        for (std::uint32_t index = 0u; index < size; ++index)
            runtime.WriteByte(output + index,
                              runtime.ReadByte(source + index));
        return size;
    }

    const auto size = runtime.ReadWord(record + 20u);
    if (static_cast<std::int32_t>(size) <= 0) return 0u;
    const auto capacity = runtime.ReadWord(capacity_address);
    if (static_cast<std::int32_t>(capacity) <= 0) return 0u;
    const auto copied = size < capacity ? size : capacity;
    const auto source = runtime.ReadWord(record + 24u);
    for (std::uint32_t index = 0u; index < copied; ++index)
        runtime.WriteByte(output + index,
                          runtime.ReadByte(source + index));
    return copied;
}

namespace {

void SetChildText(const std::uint32_t record, const std::string_view text,
                  IXmlAttributeRecordRuntime& runtime) {
    const auto child = runtime.ReadWord(record + 24u);
    if (child == 0u) throw std::runtime_error("XmlAttribute child is null");
    const auto length = static_cast<std::uint32_t>(text.size());
    const auto data = runtime.ReadWord(child + 4u);
    const auto reference = data == 0u ? 0u : runtime.ReadByte(data - 1u);
    const bool shared = data != 0u && reference != 0u && reference != 0xffu;
    if (shared && length == 0u) {
        runtime.WriteByte(data - 1u,
                          static_cast<std::uint8_t>(reference - 1u));
        runtime.WriteWord(child + 4u, 0u);
        runtime.WriteWord(child + 8u, 0u);
        runtime.WriteWord(child + 12u, 0u);
        return;
    }
    if (!shared && length == 0u) {
        runtime.SetErrorFlag(child, 1u);
        return;
    }
    if (shared) {
        runtime.Reallocate(child, length);
    } else {
        const auto capacity = runtime.ReadWord(child + 12u);
        if (capacity > 0x1fu || capacity < length) {
            runtime.SetErrorFlag(child, 1u);
            runtime.Reallocate(child, length);
        }
    }
    const auto destination = runtime.ReadWord(child + 4u);
    for (std::uint32_t index = 0u; index < length; ++index)
        runtime.WriteByte(destination + index,
                          static_cast<std::uint8_t>(text[index]));
    runtime.WriteWord(child + 8u, length);
    runtime.WriteByte(destination + length, 0u);
}

std::uint8_t SetRawValue(const std::uint32_t record,
                         const std::uint32_t source,
                         const std::uint32_t size,
                         IXmlAttributeRecordRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadWord(record + 20u)) > 0) {
        (void)runtime.FreeRaw(runtime.ReadWord(record + 24u));
        runtime.WriteWord(record + 20u, 0u);
        runtime.WriteWord(record + 24u, 0u);
    }
    runtime.WriteWord(record + 20u, size);
    const auto destination = runtime.AllocateRaw(size);
    if (destination == 0u) throw std::bad_alloc{};
    runtime.WriteWord(record + 24u, destination);
    for (std::uint32_t index = 0u; index < size; ++index)
        runtime.WriteByte(destination + index,
                          runtime.ReadByte(source + index));
    return 1u;
}

}  // namespace

std::uint8_t XmlAttributeRecord::SetChar(
    const std::uint32_t record, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    const auto value = runtime.ReadByte(source);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        runtime.WriteByte(record + 24u, value);
        return 1u;
    case 1u:
        runtime.WriteWord(record + 24u,
                          static_cast<std::uint32_t>(
                              static_cast<std::int32_t>(
                                  static_cast<std::int8_t>(value))));
        return 1u;
    case 2u:
        runtime.WriteWord(record + 24u, std::bit_cast<std::uint32_t>(
            static_cast<float>(static_cast<std::int8_t>(value))));
        return 1u;
    case 3u:
        SetChildText(record, value == 0u ? std::string_view{} :
                           std::string_view{reinterpret_cast<const char*>(&value), 1u},
                     runtime);
        return 1u;
    default:
        return SetRawValue(record, source, 1u, runtime);
    }
}

std::uint8_t XmlAttributeRecord::SetInt(
    const std::uint32_t record, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    const auto value = runtime.ReadWord(source);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        runtime.WriteByte(record + 24u, static_cast<std::uint8_t>(value));
        return 1u;
    case 1u:
        runtime.WriteWord(record + 24u, value);
        return 1u;
    case 2u:
        runtime.WriteWord(record + 24u, std::bit_cast<std::uint32_t>(
            static_cast<float>(static_cast<std::int32_t>(value))));
        return 1u;
    case 3u:
        SetChildText(record,
                     std::to_string(static_cast<std::int32_t>(value)), runtime);
        return 1u;
    default:
        return SetRawValue(record, source, 4u, runtime);
    }
}

std::uint8_t XmlAttributeRecord::SetFloat(
    const std::uint32_t record, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    const auto bits = runtime.ReadWord(source);
    switch (runtime.ReadWord(record + 16u)) {
    case 0u:
        runtime.WriteByte(record + 24u,
                          static_cast<std::uint8_t>(runtime.FloatToInt(bits)));
        return 1u;
    case 1u:
        runtime.WriteWord(record + 24u,
                          static_cast<std::uint32_t>(runtime.FloatToInt(bits)));
        return 1u;
    case 2u:
        runtime.WriteWord(record + 24u, bits);
        return 1u;
    case 3u:
        SetChildText(record, runtime.FormatFloat(bits), runtime);
        return 1u;
    default:
        return SetRawValue(record, source, 4u, runtime);
    }
}

std::uint8_t XmlAttributeRecord::SetString(
    const std::uint32_t record, const std::uint32_t source,
    IXmlAttributeRecordRuntime& runtime) {
    const auto type = runtime.ReadWord(record + 16u);
    if (type <= 3u && source == 0u)
        throw std::invalid_argument("XmlAttribute source string is null");
    switch (type) {
    case 0u:
        runtime.WriteByte(record + 24u, runtime.ReadByte(source));
        return 1u;
    case 1u:
        runtime.WriteWord(record + 24u,
                          static_cast<std::uint32_t>(runtime.ParseInt(source)));
        return 1u;
    case 2u:
        runtime.WriteWord(record + 24u, runtime.ParseFloatBits(source));
        return 1u;
    case 3u:
        SetChildText(record, ReadCString(source, runtime), runtime);
        return 1u;
    default:
        if (static_cast<std::int32_t>(runtime.ReadWord(record + 20u)) > 0) {
            (void)runtime.FreeRaw(runtime.ReadWord(record + 24u));
            runtime.WriteWord(record + 20u, 0u);
            runtime.WriteWord(record + 24u, 0u);
        }
        if (source != 0u) {
            const auto text = ReadCString(source, runtime);
            const auto length = static_cast<std::uint32_t>(text.size());
            runtime.WriteWord(record + 20u, length);
            const auto destination = runtime.AllocateRaw(length);
            if (destination == 0u && length != 0u) throw std::bad_alloc{};
            runtime.WriteWord(record + 24u, destination);
            for (std::uint32_t index = 0u; index < length; ++index)
                runtime.WriteByte(destination + index,
                                  static_cast<std::uint8_t>(text[index]));
        }
        return 1u;
    }
}

std::uint8_t XmlAttributeRecord::SetValue(
    const std::uint32_t record, const std::uint32_t source,
    const std::uint32_t size_address,
    IXmlAttributeRecordRuntime& runtime) {
    if (size_address == 0u)
        throw std::invalid_argument("XmlAttribute size pointer is null");
    const auto size = runtime.ReadWord(size_address);
    const auto type = runtime.ReadWord(record + 16u);
    if (type <= 3u) {
        if (static_cast<std::int32_t>(size) <= 0) return 0u;
        if (source == 0u)
            throw std::invalid_argument("XmlAttribute value pointer is null");
        if (type == 0u) {
            runtime.WriteByte(record + 24u, runtime.ReadByte(source));
        } else if (type == 1u || type == 2u) {
            runtime.WriteWord(record + 24u, runtime.ReadWord(source));
        } else {
            SetChildText(record, ReadCString(source, runtime), runtime);
        }
        return 1u;
    }

    if (static_cast<std::int32_t>(size) > 0) {
        if (source == 0u)
            throw std::invalid_argument("XmlAttribute value pointer is null");
        (void)SetRawValue(record, source, size, runtime);
    } else if (static_cast<std::int32_t>(runtime.ReadWord(record + 20u)) > 0) {
        (void)runtime.FreeRaw(runtime.ReadWord(record + 24u));
        runtime.WriteWord(record + 20u, 0u);
        runtime.WriteWord(record + 24u, 0u);
    }
    return 0u;
}

std::uint32_t XmlAttributeRecord::GetTypeName(
    const std::uint32_t record, IXmlAttributeRecordRuntime& runtime) {
    constexpr std::string_view names[]{
        "XML_CHAR", "XML_INT", "XML_FLOAT", "XML_STRING", "XML_BINARY"};
    const auto text = runtime.ReadWord(record + 4u);
    const auto length = runtime.ReadWord(record + 8u);
    for (std::uint32_t type = 0u; type < 5u; ++type) {
        const auto name = names[type];
        if (length != name.size()) continue;
        bool equal = true;
        for (std::uint32_t index = 0u; index < length; ++index) {
            if (runtime.ReadByte(text + index) !=
                static_cast<std::uint8_t>(name[index])) {
                equal = false;
                break;
            }
        }
        if (equal) return type;
    }
    return text;
}

}  // namespace pal4
