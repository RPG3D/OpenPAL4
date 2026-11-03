#include "pal4/sound_pool.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct SingletonRuntime final : pal4::ISoundPoolSingletonRuntime {
    std::vector<std::string> events;
    pal4::SoundPoolFactory factory = nullptr;
    pal4::SoundPoolDeleter deleter = nullptr;
    std::uint32_t result = 0x9000u;
    void GetRegistryInstance() override { events.push_back("registry"); }
    std::uint32_t Register(const char* name, std::uint32_t identifier,
                           pal4::SoundPoolFactory creator,
                           pal4::SoundPoolDeleter destroyer) override {
        events.push_back(std::string(name) + ":" +
                         std::to_string(identifier));
        factory = creator;
        deleter = destroyer;
        return result;
    }
};

struct ObjectRuntime final : pal4::ISoundPoolObjectRuntime {
    pal4::SoundPoolTreeGlobals tree{};
    std::vector<std::uint32_t> allocations;
    std::size_t allocation_index = 0u;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    std::uint32_t free_result = 0x55u;
    pal4::SoundPoolTreeGlobals& TreeGlobals() override { return tree; }
    std::uint32_t Allocate(std::uint32_t size) override {
        const auto result = allocations.at(allocation_index++);
        events.push_back("allocate:" + std::to_string(size) + ":" +
                         std::to_string(result));
        return result;
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        events.push_back("read:" + std::to_string(address) + ":" +
                         std::to_string(offset));
        return words.at({address, offset});
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(value));
        words[{address, offset}] = value;
    }
    void WriteByte(std::uint32_t address, std::uint32_t offset,
                   std::uint8_t value) override {
        events.push_back("byte:" + std::to_string(address) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(value));
        bytes[{address, offset}] = value;
    }
    void FreeNodesRecursive(std::uint32_t node) override {
        events.push_back("recursive:" + std::to_string(node));
    }
    void CleanupNodeValue(std::uint32_t address) override {
        events.push_back("cleanup-value:" + std::to_string(address));
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void RemoveNode(std::uint32_t pool,
                    std::uint32_t node) override {
        events.push_back("remove:" + std::to_string(pool) + ":" +
                         std::to_string(node));
    }
    void FirstElement(std::uint32_t pool) override {
        events.push_back("first:" + std::to_string(pool));
    }
};

struct AddRuntime final : pal4::ISoundPoolAddRuntime {
    std::map<std::uint32_t, pal4::AudioFileState> files;
    std::vector<std::string> events;
    std::uint32_t found_node = 0x6000u;
    std::uint32_t file_address = 0x3000u;
    std::uint32_t read_result = 0x4000u;
    bool control_enabled = false;
    std::uint32_t Root(std::uint32_t pool) override {
        events.push_back("root:" + std::to_string(pool));
        return 0x6000u;
    }
    std::uint32_t FindNode(std::uint32_t pool,
                            std::uint32_t crc) override {
        events.push_back("find:" + std::to_string(pool) + ":" +
                         std::to_string(crc));
        return found_node;
    }
    std::uint32_t NodeFile(std::uint32_t node) override {
        events.push_back("node-file:" + std::to_string(node));
        return file_address;
    }
    pal4::AudioFileState& FileAt(std::uint32_t address) override {
        events.push_back("file-at:" + std::to_string(address));
        return files.at(address);
    }
    std::uint32_t AllocateFile(std::uint32_t bytes) override {
        events.push_back("allocate-file:" + std::to_string(bytes));
        files[file_address] = {};
        return file_address;
    }
    std::uint32_t FreeFile(std::uint32_t address) override {
        events.push_back("free-file:" + std::to_string(address));
        return 0x155u;
    }
    bool ControlEnabled(std::uint32_t pool) override {
        events.push_back("control:" + std::to_string(pool));
        return control_enabled;
    }
    void InsertSoundObject(std::uint32_t pool, std::uint32_t crc,
                           std::uint32_t file) override {
        events.push_back("insert:" + std::to_string(pool) + ":" +
                         std::to_string(crc) + ":" +
                         std::to_string(file));
    }
    void DestroyTemporary(std::uint32_t file,
                           std::uint32_t flags) override {
        events.push_back("destroy-temp:" + std::to_string(file) + ":" +
                         std::to_string(flags));
    }
    void IncrementReference(std::uint32_t file) override {
        events.push_back("increment:" + std::to_string(file));
        ++files.at(file).reference_count;
    }
    void InitializePath(pal4::StringBufferState& path) override {
        events.push_back("path-init");
        path = {};
    }
    std::uint32_t CleanupPath(pal4::StringBufferState&) override {
        events.push_back("path-clean");
        return 0u;
    }
    void AssignPath(pal4::StringBufferState& path, const char* text,
                    std::uint32_t length) override {
        events.push_back("path-assign:" + std::string(text));
        path.length = length;
    }
    std::uint32_t FileSize(const char* path) override {
        events.push_back("size:" + std::string(path));
        return 0x100u;
    }
    std::uint32_t FileRead(const char* path,
                           std::uint32_t flags) override {
        events.push_back("read:" + std::string(path) + ":" +
                         std::to_string(flags));
        return read_result;
    }
    std::uint32_t FileType(std::uint32_t data,
                           std::uint32_t bytes) override {
        events.push_back("type:" + std::to_string(data) + ":" +
                         std::to_string(bytes));
        return 1u;
    }
    void WaveInfo(std::uint32_t, pal4::AudioWaveInfo&) override {}
    bool DecompressAdpcm(const pal4::AudioWaveInfo&, std::uint32_t&,
                          std::uint32_t&) override { return false; }
    bool DecompressAsi(std::uint32_t, std::uint32_t, const char*,
                        std::uint32_t&, std::uint32_t&,
                        std::uint32_t) override { return false; }
    void FreeLocked(std::uint32_t data) override {
        events.push_back("free-locked:" + std::to_string(data));
    }
    void Log(std::uint32_t message_id) override {
        events.push_back("log:" + std::to_string(message_id));
    }
    std::uint32_t ComputeCrc(const char* path,
                             std::uint32_t length) override {
        events.push_back("crc:" + std::string(path) + ":" +
                         std::to_string(length));
        return 0x1234u;
    }
};

struct ReleaseRuntime final : pal4::ISoundPoolReleaseRuntime {
    std::map<std::uint32_t, pal4::AudioFileState> files;
    std::map<std::uint32_t, std::uint32_t> node_crcs;
    std::map<std::uint32_t, std::uint32_t> node_files;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    std::uint32_t first = 0x6000u;
    std::uint32_t candidate = 0x6000u;
    std::uint32_t count = 0u;
    std::uint32_t Root(std::uint32_t) override { return 0x6000u; }
    std::uint32_t FirstNode(std::uint32_t) override { return first; }
    std::uint32_t Count(std::uint32_t) override {
        events.push_back("count");
        return count;
    }
    std::uint32_t FindNode(std::uint32_t,
                            std::uint32_t crc) override {
        events.push_back("find:" + std::to_string(crc));
        return candidate;
    }
    std::uint32_t NodeCrc(std::uint32_t node) override {
        events.push_back("crc:" + std::to_string(node));
        return node_crcs.at(node);
    }
    std::uint32_t NodeFile(std::uint32_t node) override {
        events.push_back("node-file:" + std::to_string(node));
        return node_files.at(node);
    }
    pal4::AudioFileState& FileAt(std::uint32_t address) override {
        events.push_back("file:" + std::to_string(address));
        return files.at(address);
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    std::uint32_t RemoveNode(std::uint32_t pool,
                              std::uint32_t node) override {
        events.push_back("remove:" + std::to_string(pool) + ":" +
                         std::to_string(node));
        return 0xabcdu;
    }
};

void SingletonAndFactory() {
    SingletonRuntime registry;
    pal4::SoundPoolSingletonState state{0x80u, 0u};
    Check(pal4::SoundPool::GetInstance(state, registry) == 0x9000u &&
              state.guard == 0x81u && state.instance == 0x9000u &&
              registry.events == std::vector<std::string>{
                  "registry", "auSoundPool:1005"} &&
              registry.factory == &pal4::SoundPool::Factory &&
              registry.deleter == &pal4::SoundPool::DestroyInstance,
          "first lookup registers exact name, id and executable callbacks");
    registry.events.clear();
    registry.result = 0x9999u;
    Check(pal4::SoundPool::GetInstance(state, registry) == 0x9000u &&
              registry.events.empty(),
          "cached lookup skips registry even when its result changes");
    state = {};
    registry.result = 0u;
    Check(pal4::SoundPool::GetInstance(state, registry) == 0u &&
              state.guard == 1u &&
              pal4::SoundPool::GetInstance(state, registry) == 0u,
          "failed registration latches singleton guard");

    ObjectRuntime object;
    object.allocations = {0u};
    Check(registry.factory(object) == 0u &&
              object.events == std::vector<std::string>{"allocate:20:0"},
          "factory allocation failure exits before tree initialization");

    object = {};
    object.allocations = {0x1000u, 0x5000u, 0x6000u,
                          0x2000u, 0x7000u, 0x8000u};
    Check(registry.factory(object) == 0x1000u &&
              object.tree.sentinel == 0x5000u && object.tree.users == 1u &&
              object.words.at({0x5000u, 4u}) == 0u &&
              object.words.at({0x5000u, 24u}) == 1u &&
              object.words.at({0x5000u, 0u}) == 0u &&
              object.words.at({0x5000u, 8u}) == 0u &&
              object.words.at({0x6000u, 4u}) == 0x5000u &&
              object.words.at({0x6000u, 24u}) == 0u &&
              object.words.at({0x6000u, 0u}) == 0x6000u &&
              object.words.at({0x6000u, 8u}) == 0x6000u &&
              object.words.at({0x1000u, 4u}) == 0x6000u &&
              object.words.at({0x1000u, 12u}) == 0u &&
              object.bytes.at({0x1000u, 8u}) == 0u &&
              object.bytes.at({0x1000u, 16u}) == 1u &&
              object.bytes.find({0x1000u, 0u}) == object.bytes.end(),
          "first factory creates shared sentinel and initialized root");
    Check(object.events == std::vector<std::string>{
              "allocate:20:4096", "byte:4096:8:0", "allocate:28:20480",
              "write:20480:4:0", "write:20480:24:1",
              "write:20480:0:0", "write:20480:8:0",
              "allocate:28:24576", "write:24576:4:20480",
              "write:24576:24:0", "write:4096:4:24576",
              "write:4096:12:0", "write:24576:0:24576",
              "write:24576:8:24576", "byte:4096:16:1"},
          "factory preserves native allocation and field-write order");

    object.events.clear();
    Check(registry.factory(object) == 0x2000u &&
              object.tree.sentinel == 0x5000u && object.tree.users == 2u &&
              object.words.at({0x8000u, 4u}) == 0x5000u &&
              object.events == std::vector<std::string>{
                  "allocate:20:8192", "byte:8192:8:0",
                  "allocate:28:28672", "write:28672:4:0",
                  "write:28672:24:1", "free:28672",
                  "allocate:28:32768", "write:32768:4:20480",
                  "write:32768:24:0", "write:8192:4:32768",
                  "write:8192:12:0", "write:32768:0:32768",
                  "write:32768:8:32768", "byte:8192:16:1"},
          "later factory reuses shared sentinel and frees candidate");

    object.events.clear();
    Check(registry.deleter(0u, object) == 0u && object.events.empty(),
          "null singleton deleter has no side effects");
    object.events.clear();
    Check(registry.deleter(0x1000u, object) == 0x55u &&
              object.tree.users == 1u && object.tree.sentinel == 0x5000u &&
              object.events.back() == "free:4096",
          "singleton deleter destroys tree before freeing object");
}

void DestroyBranches() {
    ObjectRuntime runtime;
    runtime.tree = {0x5000u, 2u};
    runtime.words = {{{0x1000u, 4u}, 0x6000u},
                     {{0x1000u, 12u}, 2u},
                     {{0x6000u, 0u}, 0x6100u},
                     {{0x6000u, 4u}, 0x6200u}};
    Check(pal4::SoundPool::Destroy(0x1000u, runtime) == 1u &&
              runtime.tree.users == 1u &&
              runtime.tree.sentinel == 0x5000u &&
              runtime.words.at({0x6000u, 0u}) == 0x6000u &&
              runtime.words.at({0x6000u, 4u}) == 0x5000u &&
              runtime.words.at({0x6000u, 8u}) == 0x6000u &&
              runtime.words.at({0x1000u, 4u}) == 0u &&
              runtime.words.at({0x1000u, 12u}) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:4096:4", "read:4096:12", "read:24576:0",
                  "read:24576:4", "recursive:25088",
                  "write:24576:4:20480", "write:4096:12:0",
                  "write:24576:0:24576", "write:24576:8:24576",
                  "first:4096", "free:24576", "write:4096:4:0",
                  "write:4096:12:0"},
          "populated tree frees nodes and retains shared sentinel");

    runtime.events.clear();
    runtime.words[{0x2000u, 4u}] = 0x7000u;
    runtime.words[{0x2000u, 12u}] = 0u;
    runtime.words[{0x7000u, 0u}] = 0x7100u;
    runtime.successors = {{0x7100u, 0x7200u},
                          {0x7200u, 0x7000u}};
    Check(pal4::SoundPool::Destroy(0x2000u, runtime) == 0x55u &&
              runtime.tree.users == 0u &&
              runtime.tree.sentinel == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:8192:4", "read:8192:12", "read:28672:0",
                  "next:28928", "remove:8192:28928",
                  "next:29184", "remove:8192:29184",
                  "free:28672", "write:8192:4:0",
                  "write:8192:12:0", "free:20480"},
          "empty-count tree removes linked nodes and last sentinel");
}

void CleanupTreeAlias() {
    ObjectRuntime runtime;
    runtime.tree = {0x5000u, 1u};
    runtime.words = {{{0x1000u, 4u}, 0x6000u},
                     {{0x1000u, 12u}, 1u},
                     {{0x6000u, 0u}, 0x6100u},
                     {{0x6000u, 4u}, 0x6200u}};
    Check(pal4::SoundPool::CleanupTree(0x1000u, runtime) == 0x55u &&
              runtime.tree.users == 0u &&
              runtime.tree.sentinel == 0u &&
              runtime.words.at({0x1000u, 4u}) == 0u &&
              runtime.words.at({0x1000u, 12u}) == 0u &&
              runtime.events.back() == "free:20480",
          "second cleanup entry follows tree teardown and releases sentinel");
}

void ClearElementsBranches() {
    ObjectRuntime runtime;
    runtime.tree = {0x5000u, 1u};
    runtime.words = {{{0x1000u, 4u}, 0x6000u},
                     {{0x1000u, 12u}, 2u},
                     {{0x6000u, 0u}, 0x6100u},
                     {{0x6000u, 4u}, 0x6100u},
                     {{0x6100u, 0u}, 0x6200u},
                     {{0x6100u, 8u}, 0x6300u},
                     {{0x6200u, 0u}, 0x5000u},
                     {{0x6200u, 8u}, 0x5000u}};
    Check(pal4::SoundPool::ClearElements(0x1000u, runtime) == 0x6000u &&
              runtime.words.at({0x1000u, 4u}) == 0x6000u &&
              runtime.words.at({0x1000u, 12u}) == 0u &&
              runtime.words.at({0x6000u, 0u}) == 0x6000u &&
              runtime.words.at({0x6000u, 4u}) == 0x5000u &&
              runtime.words.at({0x6000u, 8u}) == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "read:4096:4", "read:4096:12", "read:24576:0",
                  "read:24576:4", "read:24832:8", "recursive:25344",
                  "read:24832:0", "cleanup-value:24844", "free:24832",
                  "read:25088:8", "recursive:20480", "read:25088:0",
                  "cleanup-value:25100", "free:25088",
                  "write:24576:4:20480", "write:4096:12:0",
                  "write:24576:0:24576", "write:24576:8:24576"},
          "populated clear cleans node values and retains empty root");

    runtime.events.clear();
    Check(pal4::SoundPool::ClearElements(0x1000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "read:4096:4", "read:4096:12", "read:24576:0"},
          "already empty tree returns root without releasing storage");

    runtime.events.clear();
    runtime.words[{0x6000u, 0u}] = 0x6100u;
    runtime.successors = {{0x6100u, 0x6200u},
                          {0x6200u, 0x6000u}};
    Check(pal4::SoundPool::ClearElements(0x1000u, runtime) == 0x6000u &&
              runtime.words.at({0x1000u, 4u}) == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "read:4096:4", "read:4096:12", "read:24576:0",
                  "next:24832", "remove:4096:24832",
                  "next:25088", "remove:4096:25088"},
          "zero-count linked nodes are removed one at a time");
}

void AddAudioFileBranches() {
    AddRuntime runtime;
    runtime.found_node = 0x7000u;
    runtime.files[0x3000u].reference_count = 4u;
    Check(pal4::SoundPool::AddAudioFile(
              0x1000u, "sound.wav", runtime) == 0x3000u &&
              runtime.files.at(0x3000u).reference_count == 5u &&
              runtime.events == std::vector<std::string>{
                  "crc:sound.wav:9", "find:4096:4660", "root:4096",
                  "node-file:28672", "file-at:12288"},
          "cache hit increments a positive reference without loading");

    runtime.events.clear();
    runtime.files[0x3000u].reference_count = 0xffffffffu;
    Check(pal4::SoundPool::AddAudioFile(
              0x1000u, "sound.wav", runtime) == 0x3000u &&
              runtime.files.at(0x3000u).reference_count == 1u,
          "cache hit resets signed nonpositive reference before increment");

    runtime.events.clear();
    runtime.found_node = 0x6000u;
    runtime.read_result = 0u;
    Check(pal4::SoundPool::AddAudioFile(
              0x1000u, "missing.wav", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "crc:missing.wav:11", "find:4096:4660", "root:4096",
                  "allocate-file:28", "file-at:12288", "path-init",
                  "size:missing.wav", "read:missing.wav:0",
                  "log:97", "log:100", "path-clean", "free-file:12288"},
          "load failure logs outer error then cleans and frees file");

    runtime.events.clear();
    runtime.read_result = 0x4000u;
    runtime.control_enabled = false;
    Check(pal4::SoundPool::AddAudioFile(
              0x1000u, "sound.wav", runtime) == 0x3000u &&
              runtime.files.at(0x3000u).data == 0x4000u &&
              runtime.files.at(0x3000u).reference_count == 0u &&
              runtime.events == std::vector<std::string>{
                  "crc:sound.wav:9", "find:4096:4660", "root:4096",
                  "allocate-file:28", "file-at:12288", "path-init",
                  "size:sound.wav", "read:sound.wav:0",
                  "type:16384:256", "crc:sound.wav:9",
                  "path-assign:sound.wav", "control:4096"},
          "disabled pool loads a file without inserting or incrementing it");

    runtime.events.clear();
    runtime.control_enabled = true;
    Check(pal4::SoundPool::AddAudioFile(
              0x1000u, "sound.wav", runtime) == 0x3000u &&
              runtime.files.at(0x3000u).reference_count == 1u &&
              runtime.events == std::vector<std::string>{
                  "crc:sound.wav:9", "find:4096:4660", "root:4096",
                  "allocate-file:28", "file-at:12288", "path-init",
                  "size:sound.wav", "read:sound.wav:0",
                  "type:16384:256", "crc:sound.wav:9",
                  "path-assign:sound.wav", "control:4096",
                  "insert:4096:4660:12288", "destroy-temp:12288:1",
                  "increment:12288"},
          "enabled pool inserts, destroys temporary owner and increments ref");
}

void CleanupEntryBranches() {
    AddRuntime runtime;
    pal4::SoundPoolEntryState entry{};
    entry.crc = 0x1234u;
    entry.file = 0x3000u;
    runtime.files[0x3000u].data = 0x4000u;
    Check(pal4::SoundPoolEntry::Cleanup(entry, runtime) == 0u &&
              runtime.events.empty(),
          "unowned entry does not clean a non-null file");
    entry.owns_file = 0x80u;
    entry.file = 0u;
    Check(pal4::SoundPoolEntry::Cleanup(entry, runtime) == 0x80u &&
              runtime.events.empty(),
          "owned entry with null file returns original ownership byte");
    entry.file = 0x3000u;
    Check(pal4::SoundPoolEntry::Cleanup(entry, runtime) == 0x55u &&
              runtime.events == std::vector<std::string>{
                  "file-at:12288", "free-locked:16384",
                  "path-clean", "free-file:12288"},
          "owned entry cleans locked data and path before file free");
}

void ReleaseAudioFileBranches() {
    ReleaseRuntime runtime;
    Check(pal4::SoundPool::ReleaseAudioFile(
              0x1000u, 0x1234u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>{"find:4660"},
          "release miss returns root without reading a node value");
    runtime.events.clear();
    runtime.candidate = 0x7000u;
    runtime.node_crcs[0x7000u] = 0x2000u;
    Check(pal4::SoundPool::ReleaseAudioFile(
              0x1000u, 0x1234u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "find:4660", "crc:28672"},
          "lower-bound candidate with a greater key is a miss");

    runtime.events.clear();
    runtime.node_crcs[0x7000u] = 0x1234u;
    runtime.node_files[0x7000u] = 0x3000u;
    runtime.files[0x3000u].reference_count = 2u;
    Check(pal4::SoundPool::ReleaseAudioFile(
              0x1000u, 0x1234u, runtime) == 0x3000u &&
              runtime.files.at(0x3000u).reference_count == 1u &&
              runtime.events == std::vector<std::string>{
                  "find:4660", "crc:28672", "node-file:28672",
                  "file:12288"},
          "positive reference decrements once and returns the file");

    runtime.events.clear();
    runtime.count = 32u;
    runtime.files[0x3000u].reference_count = 1u;
    Check(pal4::SoundPool::ReleaseAudioFile(
              0x1000u, 0x1234u, runtime) == 0x3000u &&
              runtime.files.at(0x3000u).reference_count == 0xffffffffu &&
              runtime.events.back() == "count",
          "small pool decrements a newly zero reference a second time");

    runtime.events.clear();
    runtime.count = 33u;
    runtime.first = 0x7000u;
    runtime.successors = {{0x7000u, 0x7100u},
                          {0x7100u, 0x7200u},
                          {0x7200u, 0x6000u}};
    runtime.node_files[0x7100u] = 0x3100u;
    runtime.node_files[0x7200u] = 0x3200u;
    runtime.files[0x3000u].reference_count = 0u;
    runtime.files[0x3100u].reference_count = 0xfffffffdu;
    runtime.files[0x3200u].reference_count = 2u;
    Check(pal4::SoundPool::ReleaseAudioFile(
              0x1000u, 0x1234u, runtime) == 0xabcdu &&
              runtime.files.at(0x3000u).reference_count == 0xfffffffeu &&
              runtime.files.at(0x3100u).reference_count == 0xfffffffcu &&
              runtime.files.at(0x3200u).reference_count == 2u &&
              runtime.events.back() == "remove:4096:28928",
          "large pool cleans and removes the most negative reference");

    runtime.events.clear();
    runtime.files[0x3000u].reference_count = 3u;
    runtime.files[0x3100u].reference_count = 4u;
    Check(pal4::SoundPool::CleanupExpired(
              0x1000u, runtime) == 0x6000u &&
              runtime.events.back() == "next:29184",
          "cleanup with no expired references returns root unchanged");
    runtime.events.clear();
    runtime.first = 0x6000u;
    Check(pal4::SoundPool::CleanupExpired(
              0x1000u, runtime) == 0x6000u &&
              runtime.events.empty(),
          "empty tree has no cleanup side effects");
}

void ControlFlagBranches() {
    ObjectRuntime runtime;
    runtime.words = {{{0x1000u, 4u}, 0x6003u},
                     {{0x1000u, 12u}, 0u},
                     {{0x6003u, 0u}, 0x6003u}};
    Check(pal4::SoundPool::SetControlFlag(
              0x1000u, 7u, runtime) == 7u &&
              runtime.bytes.at({0x1000u, 16u}) == 7u &&
              runtime.events == std::vector<std::string>{
                  "byte:4096:16:7"},
          "nonzero control byte updates pool without clearing its tree");
    runtime.events.clear();
    Check(pal4::SoundPool::SetControlFlag(
              0x1000u, 0u, runtime) == 3u &&
              runtime.bytes.at({0x1000u, 16u}) == 0u &&
              runtime.events == std::vector<std::string>{
                  "byte:4096:16:0", "read:4096:4", "read:4096:12",
                  "read:24579:0"},
          "zero control byte clears pool and returns root low byte");
}

}  // namespace

int main() {
    SingletonAndFactory();
    DestroyBranches();
    CleanupTreeAlias();
    ClearElementsBranches();
    AddAudioFileBranches();
    CleanupEntryBranches();
    ReleaseAudioFileBranches();
    ControlFlagBranches();
    std::cout << "Sound pool tests passed\n";
}
