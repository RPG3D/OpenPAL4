#include "pal4/dialog_manager_settings.h"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

void WriteWord(IGlobalDataSettingsSaveRuntime& runtime,
               const std::uint32_t archive, const std::uint32_t value) {
    runtime.WriteBytes(archive, &value, sizeof(value));
}

std::uint32_t ReadWord(IGlobalDataSettingsRuntime& runtime,
                       const std::uint32_t archive) {
    std::uint32_t value = 0;
    runtime.ReadBytes(archive, &value, sizeof(value));
    return value;
}

void RestoreFlags(DialogManagerState& state, const std::string& flags) {
    for (std::size_t offset = 84; offset <= 112; offset += 4)
        state.SetWord(offset, 0);
    const auto length = std::min<std::size_t>(flags.size(), 256);
    for (std::size_t bit = 0; bit < length; ++bit) {
        const auto digit = flags[flags.size() - bit - 1];
        if (digit == '1') {
            const auto offset = 84 + (bit / 32) * 4;
            state.SetWord(offset, state.Word(offset) | (1u << (bit % 32)));
        } else if (digit != '0') {
            throw std::invalid_argument("invalid dialog flag bitset");
        }
    }
}

void SetXmlInteger(IDialogManagerSettingsWriteRuntime& runtime,
                   const std::uint32_t xml, const std::string& name,
                   const std::uint32_t value) {
    const auto attribute = runtime.FindAttribute(xml, name);
    if (attribute != 0)
        runtime.SetAttribute(attribute, value);
    else
        runtime.AddAttribute(xml, name, value);
}

}  // namespace

std::string DialogManagerSettings::FlagsToString(
    const DialogManagerState& state) {
    std::string result;
    result.reserve(256);
    for (int bit = 255; bit >= 0; --bit) {
        const auto word = state.Word(84 + (bit / 32) * 4);
        result.push_back((word & (1u << (bit % 32))) != 0 ? '1' : '0');
    }
    return result;
}

void DialogManagerSettings::Save(
    DialogManagerState& state, IDialogManagerSettingsWriteRuntime& runtime) {
    runtime.ClearData(state);
    const auto file = runtime.OpenFile("pal4.set", "wb+");
    runtime.InitializeFileStream(file);
    const auto archive = runtime.CreateOutputArchive(file);
    std::uint32_t xml = 0;
    bool xml_created = false;
    bool file_closed = false;
    try {
        WriteWord(runtime, archive, 5);
        for (std::size_t offset = 28; offset <= 44; offset += 4)
            WriteWord(runtime, archive, state.Word(offset));
        WriteWord(runtime, archive, state.Word(24));

        const auto flags = FlagsToString(state);
        runtime.WriteFlagString(archive, flags);
        runtime.WriteFlagString(archive, flags);
        WriteWord(runtime, archive, 1);
        WriteWord(runtime, archive, state.Word(48));

        GlobalDataSettingsState global_data;
        for (std::size_t index = 0; index < global_data.words.size(); ++index)
            global_data.words[index] = state.Word(index * 4);
        global_data.byte_14 = state.Byte(20);
        static_cast<void>(GlobalDataSettings::Save(global_data, archive,
                                                   runtime));

        constexpr std::array<std::size_t, 4> tree_offsets{148, 164, 132, 116};
        for (const auto offset : tree_offsets) {
            WriteWord(runtime, archive, state.Word(offset + 12));
            const auto head = state.Word(offset + 4);
            for (auto node = runtime.FirstNode(state, offset); node != head;
                 node = runtime.NextNode(node, offset == 116))
                WriteWord(runtime, archive, runtime.NodeValue(node));
        }

        xml = runtime.CreateXml("GlobalData");
        xml_created = true;
        for (std::uint32_t index = 0; index < 8; ++index)
            SetXmlInteger(runtime, xml,
                          "GlobalExtInt" + std::to_string(index),
                          state.Word(52 + 4 * index));

        const auto first = state.Word(184);
        const auto end = state.Word(188);
        if (first != 0 && end >= first) {
            const auto count = (end - first) / 4;
            for (std::uint32_t index = 0; index < count; ++index)
                SetXmlInteger(runtime, xml, "Story" + std::to_string(index),
                              runtime.ReadArrayWord(first + 4 * index));
        }
        runtime.WriteConfig(xml, archive);
        runtime.CloseFile(file);
        file_closed = true;
        runtime.DestroyXml(xml);
        xml_created = false;
        runtime.DestroyArchive(archive);
    } catch (...) {
        if (!file_closed) runtime.CloseFile(file);
        if (xml_created) runtime.DestroyXml(xml);
        runtime.DestroyArchive(archive);
        throw;
    }
}

void DialogManagerSettings::Load(
    DialogManagerState& state, IDialogManagerSettingsReadRuntime& runtime) {
    runtime.Reset(state);
    const auto file = runtime.OpenFile("pal4.set", "rb");
    if (file == 0) return;
    runtime.InitializeFileStream(file);
    const auto archive = runtime.CreateInputArchive(file);
    std::uint32_t xml = 0;
    bool xml_created = false;
    bool file_closed = false;
    try {
        const auto header_count = static_cast<std::int32_t>(
            ReadWord(runtime, archive));
        for (std::int32_t index = 0; index < header_count; ++index) {
            const auto value = ReadWord(runtime, archive);
            if (index < 5) state.SetWord(28 + 4 * index, value);
        }
        state.SetWord(24, ReadWord(runtime, archive));
        RestoreFlags(state, runtime.ReadFlagString(archive));
        static_cast<void>(runtime.ReadFlagString(archive));

        const auto extra_count = static_cast<std::int32_t>(
            ReadWord(runtime, archive));
        for (std::int32_t index = 0; index < extra_count; ++index) {
            const auto value = ReadWord(runtime, archive);
            if (index == 0) state.SetWord(48, value);
        }

        GlobalDataSettingsState global_data;
        for (std::size_t index = 0; index < global_data.words.size(); ++index)
            global_data.words[index] = state.Word(index * 4);
        global_data.byte_14 = state.Byte(20);
        static_cast<void>(GlobalDataSettings::Load(global_data, archive,
                                                   runtime));
        for (std::size_t index = 0; index < global_data.words.size(); ++index)
            state.SetWord(index * 4, global_data.words[index]);
        state.SetByte(20, global_data.byte_14);

        constexpr std::array<std::size_t, 4> tree_offsets{148, 164, 132, 116};
        for (const auto offset : tree_offsets) {
            runtime.ClearTree(state, offset);
            const auto count = static_cast<std::int32_t>(
                ReadWord(runtime, archive));
            for (std::int32_t index = 0; index < count; ++index)
                runtime.InsertTreeValue(state, offset,
                                        ReadWord(runtime, archive));
        }

        xml = runtime.CreateXml("GlobalData");
        xml_created = true;
        if (runtime.XmlVersionMatches(xml, archive)) {
            for (std::uint32_t index = 0; index < 8; ++index)
                state.SetWord(52 + 4 * index,
                              runtime.XmlInteger(
                                  xml, "GlobalExtInt" + std::to_string(index))
                                  .value_or(0));
            const auto first = state.Word(184);
            const auto end = state.Word(188);
            if (first != 0 && end >= first) {
                const auto count = (end - first) / 4;
                for (std::uint32_t index = 0; index < count; ++index)
                    runtime.WriteArrayWord(
                        first + index * 4,
                        runtime.XmlInteger(
                            xml, "Story" + std::to_string(index))
                            .value_or(0));
            }
        }
        runtime.CloseFile(file);
        file_closed = true;
        runtime.DestroyXml(xml);
        xml_created = false;
        runtime.DestroyArchive(archive);
    } catch (...) {
        if (!file_closed) runtime.CloseFile(file);
        if (xml_created) runtime.DestroyXml(xml);
        runtime.DestroyArchive(archive);
        throw;
    }
}

}  // namespace pal4
