#include "pal4/binary_xml_config.h"
#include "pal4/native_dialog_settings_file_writer.h"
#include "pal4/native_dialog_settings_file_reader.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/native_movie_flag_runtime.h"
#include "pal4/script_global_callbacks.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <bit>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class StateSource final : public pal4::INativeDialogSettingsStateSource {
public:
    void SynchronizeStoryData(pal4::DialogManagerState&) override {
        ++synchronizations;
    }
    float AudioLevel(std::uint32_t index) override {
        return index == 0u ? 0.5f : 0.75f;
    }
    std::uint32_t FirstNode(const pal4::DialogManagerState& state,
                            std::size_t offset) override {
        if (populated && offset == 148u) return 0x200u;
        return state.Word(offset + 4u);
    }
    std::uint32_t NextNode(std::uint32_t node, bool) override {
        Check(populated && node == 0x200u, "dialog node traversal");
        return 0x100u;
    }
    std::uint32_t NodeValue(std::uint32_t node) override {
        Check(populated && node == 0x200u, "dialog node value");
        return 42u;
    }
    std::uint32_t StoryWord(std::uint32_t address) override {
        if (fail_story)
            throw std::runtime_error("story synchronization failed");
        Check(address >= 0x1000u && address < 0x102cu &&
                  (address - 0x1000u) % 4u == 0u,
              "story word address");
        return 77u + (address - 0x1000u) / 4u;
    }
    int synchronizations{};
    bool populated{};
    bool fail_story{};
};

class LoadedState final : public pal4::INativeDialogSettingsStateSink,
                          public pal4::INativeDialogSettingsStateSource {
public:
    void Reset(pal4::DialogManagerState& state) override {
        trees.clear();
        stories.clear();
        std::fill(std::begin(audio), std::end(audio), 0.0f);
        scale = 0.0f;
        state = {};
        state.SetWord(184u, 0x5000u);
        state.SetWord(188u, 0x5000u + 38u * 4u);
    }
    void ClearTree(pal4::DialogManagerState& state,
                   std::size_t offset) override {
        trees[offset].clear();
        state.SetWord(offset + 4u, static_cast<std::uint32_t>(
                                     0x80000000u + offset));
        state.SetWord(offset + 12u, 0u);
    }
    void InsertTreeValue(pal4::DialogManagerState& state,
                         std::size_t offset, std::uint32_t value) override {
        trees[offset].push_back(value);
        state.SetWord(offset + 12u,
                      static_cast<std::uint32_t>(trees[offset].size()));
    }
    void WriteStoryWord(std::uint32_t address,
                        std::uint32_t value) override {
        stories[address] = value;
    }
    void SetAudioLevel(std::uint32_t index, float value) override {
        Check(index < 3u, "loaded PAL4 audio index");
        audio[index] = value;
    }
    std::size_t AudioStreamCount(
        pal4::GlobalDataAudioCollection) override { return 0u; }
    std::uint32_t AudioStreamAt(
        pal4::GlobalDataAudioCollection, std::size_t) override {
        throw std::logic_error("unexpected audio stream");
    }
    std::uint32_t AudioStreamKind(std::uint32_t) override {
        throw std::logic_error("unexpected audio stream kind");
    }
    void SetStreamVolume(std::uint32_t, float) override {}
    void RefreshStream(std::uint32_t) override {}
    void SetAtomicPluginFloatData(float value) override {
        scale = value;
    }
    bool HasEffectSystem() override { return true; }
    std::int32_t SetEffectSystemSquaredValue(float) override { return 1; }

    void SynchronizeStoryData(pal4::DialogManagerState&) override {}
    float AudioLevel(std::uint32_t index) override {
        Check(index < 2u, "saved PAL4 audio index");
        return audio[index];
    }
    std::uint32_t FirstNode(const pal4::DialogManagerState& state,
                            std::size_t offset) override {
        return trees[offset].empty() ? state.Word(offset + 4u)
                                     : NodeId(offset, 0u);
    }
    std::uint32_t NextNode(std::uint32_t node, bool) override {
        const auto offset = static_cast<std::size_t>(node / 65536u);
        const auto index = static_cast<std::size_t>(node % 65536u - 1u);
        Check(trees.contains(offset) && index < trees.at(offset).size(),
              "loaded dialog traversal");
        return index + 1u == trees.at(offset).size()
                   ? static_cast<std::uint32_t>(0x80000000u + offset)
                   : NodeId(offset, index + 1u);
    }
    std::uint32_t NodeValue(std::uint32_t node) override {
        const auto offset = static_cast<std::size_t>(node / 65536u);
        const auto index = static_cast<std::size_t>(node % 65536u - 1u);
        return trees.at(offset).at(index);
    }
    std::uint32_t StoryWord(std::uint32_t address) override {
        return stories.at(address);
    }

    std::unordered_map<std::size_t, std::vector<std::uint32_t>> trees;
    std::unordered_map<std::uint32_t, std::uint32_t> stories;
    float audio[3]{};
    float scale{};

private:
    static std::uint32_t NodeId(std::size_t offset, std::size_t index) {
        return static_cast<std::uint32_t>(offset * 65536u + index + 1u);
    }
};

std::vector<std::uint8_t> ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    Check(static_cast<bool>(file), "read saved pal4.set");
    return {std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()};
}

std::uint32_t WordAt(const std::vector<std::uint8_t>& bytes,
                     std::size_t offset) {
    Check(offset + 4u <= bytes.size(), "PAL4 settings word bounds");
    std::uint32_t value{};
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

}  // namespace

int main(int argc, char** argv) {
    const auto folder = std::filesystem::temp_directory_path() /
        ("pal4-settings-test-" + std::to_string(GetCurrentProcessId()) +
         "-" + std::to_string(GetTickCount64()));
    try {
        Check(std::filesystem::create_directory(folder),
              "create isolated settings directory");
        pal4::DialogManagerState state;
        state.SetWord(24u, 0x400u);
        state.SetWord(52u, 1234u);
        StateSource source;
        pal4::NativeDialogSettingsFileWriter writer(folder, source);
        pal4::NativeMovieFlagRuntime flag(state, writer);

        pal4::ScriptGlobalCallbacks::OpenMovieFlag(0u, flag);
        const auto saved = ReadFile(folder / "pal4.set");
        Check(source.synchronizations == 1 && state.Word(24u) == 0x401u &&
                  WordAt(saved, 0u) == 5u && WordAt(saved, 24u) == 0x401u,
              "original movie callback writes live settings file");
        constexpr std::size_t xml_offset = 601u;
        Check(saved.size() > xml_offset, "settings contain binary XML");
        pal4::BinaryXmlConfig xml;
        Check(xml.Parse({saved.data() + xml_offset,
                         saved.size() - xml_offset}) &&
                  xml.Root().name == "GlobalData" &&
                  xml.Root().FindAttribute("GlobalExtInt0") &&
                  xml.Root().FindAttribute("GlobalExtInt0")->ToInt(0u) ==
                      1234u,
              "saved PAL4 GlobalData XML round trips");

        state.SetWord(152u, 0x100u);
        state.SetWord(160u, 1u);
        state.SetWord(184u, 0x1000u);
        state.SetWord(188u, 0x102cu);
        source.populated = true;
        pal4::ScriptGlobalCallbacks::OpenMovieFlag(1u, flag);
        const auto populated = ReadFile(folder / "pal4.set");
        Check(WordAt(populated, 24u) == 0x403u &&
                  WordAt(populated, 585u) == 1u &&
                  WordAt(populated, 589u) == 42u,
              "save nonempty dialog tree and movie flags");
        pal4::BinaryXmlConfig populated_xml;
        constexpr std::size_t populated_xml_offset = 605u;
        Check(populated.size() > populated_xml_offset &&
                  populated_xml.Parse({
                  populated.data() + populated_xml_offset,
                  populated.size() - populated_xml_offset}) &&
                  populated_xml.Root().FindAttribute("Story0") &&
                  populated_xml.Root().FindAttribute("Story0")->ToInt(0u) ==
                      77u &&
                  populated_xml.Root().FindAttribute("Story10") &&
                  populated_xml.Root().FindAttribute("Story10")->ToInt(0u) ==
                      87u,
              "save story progress in GlobalData XML");
        const auto& attributes = populated_xml.Root().attributes;
        const auto story10 = std::find_if(
            attributes.begin(), attributes.end(),
            [](const auto& attribute) { return attribute.name == "Story10"; });
        const auto story2 = std::find_if(
            attributes.begin(), attributes.end(),
            [](const auto& attribute) { return attribute.name == "Story2"; });
        Check(story10 < story2,
              "binary XML attributes follow original tree name order");

        source.fail_story = true;
        bool failed = false;
        try {
            pal4::ScriptGlobalCallbacks::OpenMovieFlag(2u, flag);
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Check(failed && ReadFile(folder / "pal4.set") == populated,
              "failed settings save preserves last complete file");
        const auto read_folder = folder / "read";
        Check(std::filesystem::create_directory(read_folder),
              "create isolated settings input directory");
        LoadedState loaded;
        pal4::DialogManagerState loaded_state;
        loaded_state.SetWord(24u, 0xffffffffu);
        pal4::NativeDialogSettingsFileReader reader(read_folder, loaded);
        pal4::DialogManagerSettings::Load(loaded_state, reader);
        Check(loaded_state.Word(24u) == 0u && loaded.stories.empty(),
              "missing settings file leaves reset state");
        const std::uint32_t defaults[]{0u, 2u, 1u, 1u, 1u};
        for (std::size_t i = 0; i < std::size(defaults); ++i)
            Check(loaded_state.Word(i * 4u) == defaults[i],
                  "missing settings file retains original global defaults");
        Check(loaded_state.Byte(20u) == 1u && loaded.audio[0] == 0.3f &&
                  loaded.audio[1] == 0.8f && loaded.audio[2] == 0.8f &&
                  loaded.scale == 1200.0f,
              "missing settings file applies original audio and clip defaults");
        {
            std::ofstream truncated(read_folder / "pal4.set",
                                    std::ios::binary);
            const std::uint32_t header = 5u;
            truncated.write(reinterpret_cast<const char*>(&header),
                            sizeof(header));
        }
        failed = false;
        try {
            pal4::DialogManagerSettings::Load(loaded_state, reader);
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Check(failed, "truncated settings file fails explicitly");
        std::filesystem::copy_file(
            folder / "pal4.set", read_folder / "pal4.set",
            std::filesystem::copy_options::overwrite_existing);
        pal4::DialogManagerSettings::Load(loaded_state, reader);
        Check(loaded_state.Word(24u) == 0x403u &&
                  loaded.stories.size() == 38u &&
                  loaded.stories.at(0x5000u) == 77u &&
                  loaded.stories.at(0x5028u) == 87u &&
                  loaded.stories.at(0x502cu) == 0u,
              "settings reader reopens after truncated input");
        if (argc == 2) {
            pal4::DialogManagerState original_state;
            pal4::NativeDialogSettingsFileReader original_reader(argv[1],
                                                                  loaded);
            pal4::DialogManagerSettings::Load(original_state,
                                              original_reader);
            Check(original_state.Word(24u) == 0x3ffu &&
                      loaded.stories.size() == 38u &&
                      loaded.scale > 0.0f,
                  "load original PAL4 settings and Story values");
            pal4::NativeDialogSettingsFileWriter roundtrip(folder, loaded);
            pal4::DialogManagerSettings::Save(original_state, roundtrip);
            const auto from_game = ReadFile(
                std::filesystem::path(argv[1]) / "pal4.set");
            const auto rewritten = ReadFile(folder / "pal4.set");
            Check(from_game == rewritten,
                  "original PAL4 settings round trip byte for byte");

            pal4::NativeDialogSettingsSnapshot snapshot;
            pal4::NativeDialogSettingsFileReader snapshot_reader(argv[1],
                                                                   snapshot);
            pal4::DialogManagerSettings::Load(snapshot.State(),
                                              snapshot_reader);
            Check(snapshot.State().Word(24u) == 0x3ffu,
                  "reusable settings snapshot loads original PAL4 state");
            pal4::NativeDialogSettingsFileWriter snapshot_writer(folder,
                                                                   snapshot);
            pal4::DialogManagerSettings::Save(snapshot.State(),
                                              snapshot_writer);
            Check(ReadFile(folder / "pal4.set") == from_game,
                  "reusable settings snapshot preserves original file bytes");
        }
        {
            pal4::NativeDialogSettingsSnapshot progress;
            bool uninitialized=false;
            try{progress.RecordMonsterProgress(1);}catch(const std::logic_error&){uninitialized=true;}
            Check(uninitialized,"Monster progress requires initialized settings owner");
            uninitialized=false;
            try{progress.RecordEventKey(1u);}catch(const std::logic_error&){uninitialized=true;}
            Check(uninitialized,"opened-object event requires initialized settings owner");
            pal4::NativeDialogSettingsFileReader input(folder,progress);
            pal4::DialogManagerSettings::Load(progress.State(),input);
            progress.ClearTree(progress.State(),116u);
            for(auto key:{0x80000000u,1u,0xffffffffu,0u,0x80000000u})progress.RecordEventKey(key);
            progress.InsertTreeValue(progress.State(),116u,1u);
            const auto verify_events=[&](pal4::NativeDialogSettingsSnapshot& owner){
                std::vector<std::uint32_t> keys;
                for(auto node=owner.FirstNode(owner.State(),116u);node!=owner.State().Word(120u);node=owner.NextNode(node,true))
                    keys.push_back(owner.NodeValue(node));
                Check(keys==std::vector<std::uint32_t>{0u,1u,0x80000000u,0xffffffffu} &&
                    owner.State().Word(128u)==4u,"event CRC keys retain unsigned ordering and uniqueness");
                for(auto key:keys)Check(owner.HasEventKey(key),"event CRC lookup retains key");
                Check(!owner.HasEventKey(2u),"event CRC lookup does not invent a key");
            };
            verify_events(progress);
            progress.ClearTree(progress.State(),132u);
            for(auto key:{7,-1,2,7,(std::numeric_limits<std::int32_t>::min)(),-1})
                progress.RecordMonsterProgress(key);
            //The original settings loader also inserts through the same signed
            //unique-key boundary, rather than retaining duplicate file records.
            progress.InsertTreeValue(progress.State(),132u,0xffffffffu);
            const std::vector<std::int32_t> expected{(std::numeric_limits<std::int32_t>::min)(),-1,2,7};
            const auto verify=[&](pal4::NativeDialogSettingsSnapshot& owner){
                std::vector<std::int32_t> actual;
                for(auto node=owner.FirstNode(owner.State(),132u);node!=owner.State().Word(136u);node=owner.NextNode(node,false))
                    actual.push_back(std::bit_cast<std::int32_t>(owner.NodeValue(node)));
                Check(actual==expected && owner.State().Word(144u)==expected.size(),"Monster keys retain signed tree order and uniqueness");
                for(auto key:expected)Check(owner.HasMonsterProgress(key),"Monster progress lookup retains serialized key");
                Check(!owner.HasMonsterProgress(0),"Monster progress does not invent missing key");
            };
            verify(progress);
            for(auto offset:{148u,164u}){
                progress.ClearTree(progress.State(),offset);
                for(auto key:expected)progress.InsertTreeValue(progress.State(),offset,std::bit_cast<std::uint32_t>(key));
                progress.InsertTreeValue(progress.State(),offset,0xffffffffu);
            }
            progress.RecordEquipmentProgress(7);
            const auto verify_discovery=[&](pal4::NativeDialogSettingsSnapshot& owner){
                for(auto offset:{148u,164u}){
                    std::vector<std::int32_t> actual;
                    for(auto node=owner.FirstNode(owner.State(),offset);node!=owner.State().Word(offset+4u);node=owner.NextNode(node,false))
                        actual.push_back(std::bit_cast<std::int32_t>(owner.NodeValue(node)));
                    Check(actual==expected && owner.State().Word(offset+12u)==expected.size(),"item/equipment discovery has signed unique keys in original settings storage");
                }
                for(auto key:expected)Check(owner.HasEquipmentProgress(key),"equipment discovery lookup retains saved key");
                Check(!owner.HasEquipmentProgress(0),"equipment discovery has no invented key");
            };
            verify_discovery(progress);
            pal4::NativeDialogSettingsFileWriter output(folder,progress);
            pal4::DialogManagerSettings::Save(progress.State(),output);
            pal4::NativeDialogSettingsSnapshot reopened;
            pal4::NativeDialogSettingsFileReader reload(folder,reopened);
            pal4::DialogManagerSettings::Load(reopened.State(),reload);
            verify(reopened);
            verify_discovery(reopened);
            verify_events(reopened);
        }
        std::filesystem::remove(read_folder / "pal4.set");
        std::filesystem::remove(read_folder);
        std::filesystem::remove(folder / "pal4.set");
        std::filesystem::remove(folder);
        std::cout << "Movie flag persisted to PAL4 settings format\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
