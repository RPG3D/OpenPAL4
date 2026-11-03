#include "pal4/dialog_manager_settings.h"
#include "pal4/dialog_manager_flags.h"

#include <bit>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IDialogManagerSettingsWriteRuntime {
    std::vector<std::string> calls;
    std::vector<std::uint32_t> words;
    std::vector<std::uint8_t> bytes;
    std::unordered_map<std::size_t, std::uint32_t> first_nodes;
    std::unordered_map<std::uint32_t, std::uint32_t> next_nodes;
    std::unordered_map<std::uint32_t, std::uint32_t> node_values;
    std::unordered_map<std::uint32_t, std::uint32_t> array_words;
    std::unordered_map<std::string, std::uint32_t> existing;
    std::unordered_map<std::string, std::uint32_t> attributes;
    std::string fail_add;
    std::uint32_t manager_calls = 0;

    void ClearData(pal4::DialogManagerState&) override {
        calls.emplace_back("clear data");
    }
    std::uint32_t OpenFile(const char* path, const char* mode) override {
        Check(std::string(path) == "pal4.set" && std::string(mode) == "wb+",
              "settings filename and mode");
        calls.emplace_back("open");
        return 0x1000;
    }
    void InitializeFileStream(std::uint32_t file) override {
        Check(file == 0x1000, "stream file handle");
        calls.emplace_back("stream");
    }
    std::uint32_t CreateOutputArchive(std::uint32_t file) override {
        Check(file == 0x1000, "archive file handle");
        calls.emplace_back("archive");
        return 0xabcd;
    }
    void WriteBytes(std::uint32_t archive, const void* data,
                    std::size_t length) override {
        Check(archive == 0xabcd && (length == 1 || length == 4),
              "settings archive write size");
        if (length == 4) {
            std::uint32_t word = 0;
            std::memcpy(&word, data, 4);
            words.push_back(word);
        } else {
            bytes.push_back(*static_cast<const std::uint8_t*>(data));
        }
        calls.emplace_back("write");
    }
    std::uint32_t AudioManager() override {
        ++manager_calls;
        calls.emplace_back("audio manager");
        return 0x3000;
    }
    float AudioLevel(std::uint32_t manager, std::uint32_t index) override {
        Check(manager == 0x3000 && index < 2, "audio level operands");
        calls.emplace_back("audio level");
        return index == 0 ? 0.25f : 0.75f;
    }
    void WriteFlagString(std::uint32_t archive,
                         const std::string& flags) override {
        Check(archive == 0xabcd && flags.size() == 256 &&
                  flags.find_first_not_of('0') == std::string::npos,
              "256-bit flag string write");
        calls.emplace_back("flag string");
    }
    std::uint32_t FirstNode(const pal4::DialogManagerState& state,
                            std::size_t offset) override {
        calls.emplace_back("first:" + std::to_string(offset));
        return first_nodes.count(offset) ? first_nodes.at(offset)
                                         : state.Word(offset + 4);
    }
    std::uint32_t NextNode(std::uint32_t node, bool primary) override {
        calls.emplace_back(primary ? "next primary" : "next standard");
        return next_nodes.at(node);
    }
    std::uint32_t NodeValue(std::uint32_t node) override {
        return node_values.at(node);
    }
    std::uint32_t ReadArrayWord(std::uint32_t address) override {
        calls.emplace_back("story value");
        return array_words.at(address);
    }
    std::uint32_t CreateXml(const char* root) override {
        Check(std::string(root) == "GlobalData", "XML root name");
        calls.emplace_back("create xml");
        return 0x4000;
    }
    std::uint32_t FindAttribute(std::uint32_t xml,
                                const std::string& name) override {
        Check(xml == 0x4000, "XML lookup handle");
        calls.emplace_back("find:" + name);
        return existing.count(name) ? existing.at(name) : 0;
    }
    void SetAttribute(std::uint32_t attribute,
                      std::uint32_t value) override {
        Check(attribute == 0x5000 || attribute == 0x5001,
              "existing XML attribute handle");
        attributes[attribute == 0x5000 ? "GlobalExtInt1" : "Story0"] = value;
        calls.emplace_back("set");
    }
    void AddAttribute(std::uint32_t xml, const std::string& name,
                      std::uint32_t value) override {
        Check(xml == 0x4000, "XML add handle");
        calls.emplace_back("add:" + name);
        if (name == fail_add) throw std::runtime_error("XML add failed");
        attributes[name] = value;
    }
    void WriteConfig(std::uint32_t xml,
                     std::uint32_t archive) override {
        Check(xml == 0x4000 && archive == 0xabcd, "XML config operands");
        calls.emplace_back("write config");
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == 0x1000, "close file handle");
        calls.emplace_back("close");
    }
    void DestroyXml(std::uint32_t xml) override {
        Check(xml == 0x4000, "destroy XML handle");
        calls.emplace_back("destroy xml");
    }
    void DestroyArchive(std::uint32_t archive) override {
        Check(archive == 0xabcd, "destroy archive handle");
        calls.emplace_back("destroy archive");
    }
};

struct ReadRuntime final : pal4::IDialogManagerSettingsReadRuntime {
    std::vector<std::uint32_t> words{
        5, 29, 33, 37, 41, 45, 25, 1, 49,
        1, 2, 3, 0, 5, 0x3e800000, 0x3f400000,
        2, 21, 22, 1, 31, 0, 1, 41};
    std::vector<std::string> calls;
    std::vector<std::pair<std::size_t, std::uint32_t>> inserted;
    std::unordered_map<std::string, std::uint32_t> xml_values{
        {"GlobalExtInt0", 101}, {"GlobalExtInt7", 108},
        {"Story0", 301}};
    std::unordered_map<std::uint32_t, std::uint32_t> story_values;
    std::size_t word_cursor = 0;
    std::size_t flag_reads = 0;
    bool file_exists = true;
    bool valid_xml = true;
    std::string flags = "1001";
    float atomic_value = 0.0f;
    std::uint8_t setting_byte = 0x5a;

    void Reset(pal4::DialogManagerState& state) override {
        calls.emplace_back("reset");
        for (std::size_t offset = 52; offset <= 80; offset += 4)
            state.SetWord(offset, 0);
    }
    std::uint32_t OpenFile(const char* path, const char* mode) override {
        Check(std::string(path) == "pal4.set" && std::string(mode) == "rb",
              "load filename and mode");
        calls.emplace_back("open");
        return file_exists ? 0x1000 : 0;
    }
    void InitializeFileStream(std::uint32_t file) override {
        Check(file == 0x1000, "load stream handle");
        calls.emplace_back("stream");
    }
    std::uint32_t CreateInputArchive(std::uint32_t file) override {
        Check(file == 0x1000, "load archive file");
        calls.emplace_back("archive");
        return 0xabcd;
    }
    void ReadBytes(std::uint32_t archive, void* data,
                   std::size_t length) override {
        Check(archive == 0xabcd, "load archive handle");
        if (length == 4) {
            Check(word_cursor < words.size(), "load word bounds");
            const auto value = words[word_cursor++];
            std::memcpy(data, &value, 4);
        } else {
            Check(length == 1, "load byte length");
            std::memcpy(data, &setting_byte, 1);
        }
        calls.emplace_back("read");
    }
    void WriteBytes(std::uint32_t, const void*, std::size_t) override {
        throw std::runtime_error("load must not write archive");
    }
    std::uint32_t AudioManager() override {
        calls.emplace_back("audio manager");
        return 0x3000;
    }
    float AudioLevel(std::uint32_t, std::uint32_t) override { return 0; }
    void SetAudioLevel(std::uint32_t manager, std::uint32_t index,
                       float value) override {
        Check(manager == 0x3000 && index < 3 && value >= 0,
              "loaded audio level");
        calls.emplace_back("set audio");
    }
    std::size_t AudioStreamCount(
        std::uint32_t,
        pal4::GlobalDataAudioCollection) override { return 0; }
    std::uint32_t AudioStreamAt(
        std::uint32_t, pal4::GlobalDataAudioCollection,
        std::size_t) override { return 0; }
    std::uint32_t AudioStreamKind(std::uint32_t) override { return 0; }
    void SetStreamVolume(std::uint32_t, float) override {}
    void RefreshStream(std::uint32_t) override {}
    void SetAtomicPluginFloatData(float value) override {
        atomic_value = value;
        calls.emplace_back("atomic");
    }
    bool HasEffectSystem() override { return true; }
    std::int32_t SetEffectSystemSquaredValue(float value) override {
        Check(value == atomic_value, "effect scale follows Atomic scale");
        return 7;
    }
    std::string ReadFlagString(std::uint32_t archive) override {
        Check(archive == 0xabcd, "flag string archive");
        ++flag_reads;
        calls.emplace_back("flag string");
        return flags;
    }
    void ClearTree(pal4::DialogManagerState&, std::size_t offset) override {
        calls.emplace_back("clear:" + std::to_string(offset));
    }
    void InsertTreeValue(pal4::DialogManagerState&, std::size_t offset,
                         std::uint32_t value) override {
        inserted.emplace_back(offset, value);
        calls.emplace_back("insert");
    }
    void WriteArrayWord(std::uint32_t address,
                        std::uint32_t value) override {
        story_values[address] = value;
        calls.emplace_back("story");
    }
    std::uint32_t CreateXml(const char* root) override {
        Check(std::string(root) == "GlobalData", "load XML root");
        calls.emplace_back("create xml");
        return 0x4000;
    }
    bool XmlVersionMatches(std::uint32_t xml,
                           std::uint32_t archive) override {
        Check(xml == 0x4000 && archive == 0xabcd, "XML version operands");
        calls.emplace_back("check xml");
        return valid_xml;
    }
    std::optional<std::uint32_t> XmlInteger(
        std::uint32_t xml, const std::string& name) override {
        Check(xml == 0x4000, "XML integer handle");
        calls.emplace_back("xml:" + name);
        if (xml_values.count(name)) return xml_values.at(name);
        return std::nullopt;
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == 0x1000, "load close file");
        calls.emplace_back("close");
    }
    void DestroyXml(std::uint32_t xml) override {
        Check(xml == 0x4000, "load destroy XML");
        calls.emplace_back("destroy xml");
    }
    void DestroyArchive(std::uint32_t archive) override {
        Check(archive == 0xabcd, "load destroy archive");
        calls.emplace_back("destroy archive");
    }
};

struct FlagRuntime final : pal4::IDialogManagerFlagRuntime {
    std::array<std::uint32_t, 4> words{
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    std::vector<std::string> calls;
    std::uint32_t instance_calls = 0;

    std::uint32_t GameInstance() override {
        ++instance_calls;
        calls.emplace_back("instance");
        return 0x4000;
    }
    std::uint32_t GameWord(std::uint32_t address) override {
        Check(address >= 0x4164 && address <= 0x4170 &&
                  (address - 0x4164) % 4 == 0,
              "dialog flag word address");
        calls.emplace_back("read");
        return words[(address - 0x4164) / 4];
    }
    void SetGameWord(std::uint32_t address,
                     std::uint32_t value) override {
        Check(address >= 0x4164 && address <= 0x4170 &&
                  (address - 0x4164) % 4 == 0,
              "dialog flag write address");
        words[(address - 0x4164) / 4] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
};

void Seed(pal4::DialogManagerState& state, Runtime& runtime) {
    for (std::size_t offset = 0; offset <= 48; offset += 4)
        state.SetWord(offset, static_cast<std::uint32_t>(offset + 1));
    state.SetByte(20, 0x7f);
    for (std::size_t offset = 52; offset <= 80; offset += 4)
        state.SetWord(offset, static_cast<std::uint32_t>(100 + offset));
    state.SetWord(152, 0x1400);
    state.SetWord(160, 2);
    state.SetWord(168, 0x1800);
    state.SetWord(176, 1);
    state.SetWord(136, 0x1300);
    state.SetWord(144, 0);
    state.SetWord(120, 0x1000);
    state.SetWord(128, 1);
    state.SetWord(184, 0x6000);
    state.SetWord(188, 0x6008);
    runtime.first_nodes = {{148, 0x1500}, {164, 0x1700}, {116, 0x1100}};
    runtime.next_nodes = {{0x1500, 0x1600}, {0x1600, 0x1400},
                          {0x1700, 0x1800}, {0x1100, 0x1000}};
    runtime.node_values = {{0x1500, 21}, {0x1600, 22},
                           {0x1700, 31}, {0x1100, 41}};
    runtime.array_words = {{0x6000, 301}, {0x6004, 302}};
    runtime.existing = {{"GlobalExtInt1", 0x5000}, {"Story0", 0x5001}};
}

void TestSaveSettings() {
    pal4::DialogManagerState state;
    Runtime runtime;
    Seed(state, runtime);
    pal4::DialogManagerSettings::Save(state, runtime);
    Check(runtime.words == std::vector<std::uint32_t>{
              5, 29, 33, 37, 41, 45, 25, 1, 49,
              1, 5, 9, 13, 17, 0x3e800000, 0x3f400000,
              2, 21, 22, 1, 31, 0, 1, 41},
          "binary field, global-data, tree-count and node-value order");
    Check(runtime.bytes == std::vector<std::uint8_t>{0x7f} &&
              runtime.manager_calls == 2,
          "global-data byte and two separate audio manager reads");
    Check(runtime.attributes.size() == 10 &&
              runtime.attributes.at("GlobalExtInt0") == 152 &&
              runtime.attributes.at("GlobalExtInt1") == 156 &&
              runtime.attributes.at("GlobalExtInt7") == 180 &&
              runtime.attributes.at("Story0") == 301 &&
              runtime.attributes.at("Story1") == 302,
          "eight global integers and two story attributes");
    Check(runtime.calls.front() == "clear data" &&
              runtime.calls[1] == "open" &&
              runtime.calls[2] == "stream" &&
              runtime.calls[3] == "archive" &&
              runtime.calls[runtime.calls.size() - 4] == "write config" &&
              runtime.calls[runtime.calls.size() - 3] == "close" &&
              runtime.calls[runtime.calls.size() - 2] == "destroy xml" &&
              runtime.calls.back() == "destroy archive",
          "file, archive and XML lifecycle order");
}

void TestXmlFailureCleanup() {
    pal4::DialogManagerState state;
    Runtime runtime;
    Seed(state, runtime);
    runtime.fail_add = "GlobalExtInt2";
    bool threw = false;
    try {
        pal4::DialogManagerSettings::Save(state, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls[runtime.calls.size() - 3] == "close" &&
              runtime.calls[runtime.calls.size() - 2] == "destroy xml" &&
              runtime.calls.back() == "destroy archive",
          "XML failure closes the file and unwinds owned objects");
}

void TestLoadMissingFile() {
    pal4::DialogManagerState state;
    ReadRuntime runtime;
    runtime.file_exists = false;
    pal4::DialogManagerSettings::Load(state, runtime);
    Check(runtime.calls == std::vector<std::string>{"reset", "open"},
          "missing settings file returns after reset");
}

void TestLoadSettings() {
    pal4::DialogManagerState state;
    state.SetWord(184, 0x6000);
    state.SetWord(188, 0x6008);
    ReadRuntime runtime;
    pal4::DialogManagerSettings::Load(state, runtime);
    Check(runtime.word_cursor == runtime.words.size() &&
              runtime.flag_reads == 2,
          "load consumes complete binary layout and both flag strings");
    for (std::size_t offset = 28; offset <= 44; offset += 4)
        Check(state.Word(offset) == offset + 1,
              "load restores five header words");
    Check(state.Word(24) == 25 && state.Word(48) == 49 &&
              state.Word(84) == 9 && state.Word(88) == 0,
          "load restores versions and right-to-left bitset");
    Check(state.Word(0) == 1 && state.Word(4) == 2 &&
              state.Word(8) == 3 && state.Word(12) == 0 &&
              state.Word(16) == 5 && state.Byte(20) == 0x5a &&
              runtime.atomic_value == 1800.0f,
          "load composes global-data settings and scale update");
    Check(runtime.inserted ==
              std::vector<std::pair<std::size_t, std::uint32_t>>{
                  {148, 21}, {148, 22}, {164, 31}, {116, 41}},
          "four trees restore counts and node values in save order");
    Check(state.Word(52) == 101 && state.Word(56) == 0 &&
              state.Word(80) == 108 &&
              runtime.story_values.at(0x6000) == 301 &&
              runtime.story_values.at(0x6004) == 0,
          "missing XML attributes become zero");
    Check(runtime.calls[runtime.calls.size() - 3] == "close" &&
              runtime.calls[runtime.calls.size() - 2] == "destroy xml" &&
              runtime.calls.back() == "destroy archive",
          "load resource teardown order");
}

void TestXmlVersionMismatchAndInvalidFlags() {
    pal4::DialogManagerState state;
    state.SetWord(184, 0x6000);
    state.SetWord(188, 0x6008);
    ReadRuntime runtime;
    runtime.valid_xml = false;
    pal4::DialogManagerSettings::Load(state, runtime);
    Check(state.Word(52) == 0 && state.Word(80) == 0 &&
              runtime.story_values.empty(),
          "XML version mismatch leaves reset integer state alone");

    ReadRuntime invalid;
    invalid.flags = "10x1";
    bool threw = false;
    try {
        pal4::DialogManagerSettings::Load(state, invalid);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Check(threw && invalid.calls[invalid.calls.size() - 2] == "close" &&
              invalid.calls.back() == "destroy archive",
          "invalid bitset rejects non-binary digits and closes resources");
}

void TestFlagStringConversion() {
    pal4::DialogManagerState state;
    state.SetWord(84, 1);
    state.SetWord(112, 0x80000000);
    const auto flags = pal4::DialogManagerSettings::FlagsToString(state);
    Check(flags.size() == 256 && flags.front() == '1' &&
              flags.back() == '1' &&
              flags.substr(1, 254) == std::string(254, '0'),
          "flag conversion emits bit 255 before bit zero");
    state.SetWord(84, 0);
    state.SetWord(112, 0);
    Check(pal4::DialogManagerSettings::FlagsToString(state) ==
              std::string(256, '0'),
          "zero flags retain fixed-width serialization");
}

void TestDialogFlags() {
    pal4::DialogManagerState state;
    Runtime writer;
    Seed(state, writer);
    FlagRuntime game;
    pal4::DialogManagerFlags::ClearAndSave(state, game, writer);
    Check(game.words == std::array<std::uint32_t, 4>{0, 0, 0, 0} &&
              game.instance_calls == 1 &&
              game.calls == std::vector<std::string>{
                  "instance", "write:16752", "write:16748",
                  "write:16744", "write:16740"} &&
              writer.calls.back() == "destroy archive",
          "clear writes four game words in reverse order then saves");

    writer = Runtime{};
    Seed(state, writer);
    game.calls.clear();
    game.instance_calls = 0;
    pal4::DialogManagerFlags::SetAndSave(state, 127, game, writer);
    Check(game.words[3] == 0x80000000 && game.instance_calls == 2 &&
              game.calls == std::vector<std::string>{
                  "instance", "instance", "read", "write:16752"} &&
              writer.calls.back() == "destroy archive",
          "highest valid bit saves after two singleton lookups");
    game.calls.clear();
    game.instance_calls = 0;
    Check(pal4::DialogManagerFlags::IsSet(127, game) &&
              game.instance_calls == 2 &&
              game.calls == std::vector<std::string>{
                  "instance", "instance", "read"},
          "query returns highest bit");
    game.calls.clear();
    game.instance_calls = 0;
    Check(!pal4::DialogManagerFlags::IsSet(31, game) &&
              game.instance_calls == 2,
          "query returns false for clear bit");

    writer = Runtime{};
    game.calls.clear();
    game.instance_calls = 0;
    pal4::DialogManagerFlags::SetAndSave(state, 128, game, writer);
    Check(writer.calls.empty() && game.instance_calls == 1 &&
              game.calls == std::vector<std::string>{"instance"},
          "out-of-range set has no mutation or save");
    game.calls.clear();
    game.instance_calls = 0;
    Check(!pal4::DialogManagerFlags::IsSet(128, game) &&
              game.instance_calls == 1 &&
              game.calls == std::vector<std::string>{"instance"},
          "out-of-range query still fetches singleton once");
}

}  // namespace

int main() {
    try {
        TestSaveSettings();
        TestXmlFailureCleanup();
        TestLoadMissingFile();
        TestLoadSettings();
        TestXmlVersionMismatchAndInvalidFlags();
        TestFlagStringConversion();
        TestDialogFlags();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
