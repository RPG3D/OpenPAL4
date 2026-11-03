#include "pal4/audio_manager.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool okay, const char* message) {
    if (!okay) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct PointerCopyRuntime final : pal4::IAudioPointerCopyRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t Read(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void Write(std::uint32_t address, std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        words[address] = value;
    }
};

void CopyAudioPointerElements() {
    PointerCopyRuntime runtime;
    Check(pal4::AudioPointerArrayOps::CopyElements(
              0x1000u, 0x1000u, 0x2000u, runtime) == 0x2000u &&
              runtime.events.empty(),
          "empty pointer range preserves destination without memory access");
    runtime.words = {{0x1000u, 11u}, {0x1004u, 22u}, {0x1008u, 33u}};
    Check(pal4::AudioPointerArrayOps::CopyElements(
              0x1000u, 0x100cu, 0x2000u, runtime) == 0x200cu &&
              runtime.words.at(0x2000u) == 11u &&
              runtime.words.at(0x2004u) == 22u &&
              runtime.words.at(0x2008u) == 33u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "write:8192:11", "read:4100",
                  "write:8196:22", "read:4104", "write:8200:33"},
          "three-word copy reads and writes each element in forward order");
    runtime.events.clear();
    Check(pal4::AudioPointerArrayOps::CopyElements(
              0x1000u, 0x100cu, 0x1004u, runtime) == 0x1010u &&
              runtime.words.at(0x1004u) == 11u &&
              runtime.words.at(0x1008u) == 11u &&
              runtime.words.at(0x100cu) == 11u,
          "overlapping destination follows native forward-copy behavior");
}

void SetAudioInitialFlag() {
    std::uint32_t flag = 0xaabbccddu;
    pal4::AudioInitialFlag::Set(flag);
    Check(flag == 1u, "initial flag writes the entire 32-bit word");
    pal4::AudioInitialFlag::Set(flag);
    Check(flag == 1u, "initial flag can be written repeatedly");
}

struct ConfigRuntime final : pal4::IAudioDriverConfigRuntime {
    pal4::AudioDriverConfigState* outer = nullptr;
    std::vector<std::string> events;
    void InitializeEmpty(pal4::StringBufferState& text) override {
        Check(&text == &outer->provider && outer->sample_rate == 17u,
              "provider initialization precedes numeric defaults");
        text = {};
        events.push_back("empty");
    }
    void AssignText(pal4::StringBufferState& text, const char* source,
                    std::uint32_t length) override {
        Check(&text == &outer->provider && outer->sample_rate == 44100u &&
                  outer->bits_per_sample == 16u &&
                  outer->channel_count == 2u,
              "provider assignment follows numeric defaults");
        events.push_back(std::string(source, length));
        text.length = length;
    }
};

struct ObjectRuntime final : pal4::IAudioManagerObjectRuntime {
    std::vector<std::string> events;
    std::uint32_t allocation = 0x6000u;
    std::uint32_t initialized = 0x7000u;
    std::uint32_t free_result = 0x33u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t Initialize(std::uint32_t address) override {
        events.push_back("initialize:" + std::to_string(address));
        return initialized;
    }
    void Destruct(std::uint32_t address) override {
        events.push_back("destruct:" + std::to_string(address));
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
};

struct SingletonRuntime final : pal4::IAudioManagerSingletonRuntime {
    std::vector<std::string> events;
    std::uint32_t register_result = 0x8000u;
    pal4::AudioManagerFactory factory = nullptr;
    pal4::AudioManagerDeleter deleter = nullptr;
    void GetRegistryInstance() override { events.push_back("registry"); }
    std::uint32_t Register(const char* name, std::uint32_t identifier,
                           pal4::AudioManagerFactory creator,
                           pal4::AudioManagerDeleter destroyer) override {
        events.push_back(std::string(name) + ":" +
                         std::to_string(identifier));
        factory = creator;
        deleter = destroyer;
        return register_result;
    }
};

struct InitRuntime final : pal4::IAudioManagerInitRuntime {
    std::vector<std::string> events;
    std::uint32_t next_memory = 0x5000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        const auto address = next_memory;
        next_memory += 0x1000u;
        return address;
    }
    void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                       std::uint32_t value) override {
        events.push_back("write:" + std::to_string(node) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(value));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    void InitializeTail(pal4::AudioManagerPrefixState& manager) override {
        Check(manager.tree_root != 0u && manager.tree_count == 0u,
              "tail initialization follows tree root setup");
        events.push_back("tail");
    }
};

struct TreeRuntime final : pal4::IAudioManagerTreeRuntime {
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> words;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    std::uint32_t free_result = 0x55u;
    std::uint32_t ReadNodeWord(std::uint32_t node,
                               std::uint32_t offset) override {
        events.push_back("read:" + std::to_string(node) + ":" +
                         std::to_string(offset));
        return words.at({node, offset});
    }
    void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                       std::uint32_t value) override {
        events.push_back("write:" + std::to_string(node) + ":" +
                         std::to_string(offset) + ":" +
                         std::to_string(value));
        words[{node, offset}] = value;
    }
    void FreeNodesRecursive(std::uint32_t root) override {
        events.push_back("recursive:" + std::to_string(root));
    }
    void FirstElement(pal4::AudioManagerPrefixState&) override {
        events.push_back("first");
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void RemoveNode(pal4::AudioManagerPrefixState&,
                    std::uint32_t node) override {
        events.push_back("remove:" + std::to_string(node));
    }
    std::uint32_t Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return free_result;
    }
};

struct ShutdownRuntime final : pal4::IAudioManagerShutdownRuntime {
    std::vector<std::string> events;
    void CleanupTail(pal4::AudioManagerPrefixState& manager) override {
        Check(manager.header_flags[0] == 0u,
              "shutdown disables active byte before tail cleanup");
        events.push_back("tail");
    }
    void CleanupSamples(pal4::AudioManagerPrefixState&) override {
        events.push_back("samples");
    }
    std::uint32_t FreeBuffer(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return address + 1u;
    }
};

struct SystemRuntime final : pal4::IAudioSystemRuntime {
    pal4::AudioManagerPrefixState* manager = nullptr;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> pointers;
    bool startup = true;
    std::uint32_t driver = 0x1100u;
    std::uint32_t provider = 0x2200u;
    std::int32_t provider_open = 0;
    std::uint32_t supported = 2u;
    std::uint32_t listener = 0x3300u;
    std::uint32_t next_memory = 0x5000u;
    std::uint32_t sample_calls = 0u;
    std::uint32_t sample3d_calls = 0u;
    std::uint32_t sample_successes = 0xffffffffu;
    std::uint32_t sample3d_successes = 0xffffffffu;
    std::uint32_t provider_name = 0u;
    pal4::AudioListenerPositionBits position_seen{};
    pal4::AudioListenerPositionBits front_seen{};
    pal4::AudioListenerPositionBits up_seen{};

    void InitializeEmpty(pal4::StringBufferState& text) override {
        text = {};
        events.push_back("config-empty");
    }
    void AssignText(pal4::StringBufferState& text, const char* source,
                    std::uint32_t length) override {
        text.data = 0xa000u;
        text.length = length;
        events.push_back("config:" + std::string(source, length));
    }
    void Log(std::uint32_t message_id) override {
        events.push_back("log:" + std::to_string(message_id));
    }
    void SetRedistDirectory(const char* path) override {
        events.push_back(std::string("redist:") + path);
    }
    void PrepareMiles() override { events.push_back("prepare"); }
    bool StartupMiles() override {
        events.push_back("startup");
        return startup;
    }
    std::uint32_t OpenDigitalDriver(std::uint32_t rate, std::uint32_t bits,
                                    std::uint32_t channels,
                                    std::uint32_t flags) override {
        events.push_back("driver:" + std::to_string(rate) + ":" +
                         std::to_string(bits) + ":" +
                         std::to_string(channels) + ":" +
                         std::to_string(flags));
        return driver;
    }
    void ReadLastError() override { events.push_back("last-error"); }
    void SetMasterVolume(std::uint32_t address,
                         std::uint32_t bits) override {
        events.push_back("volume:" + std::to_string(address) + ":" +
                         std::to_string(bits));
    }
    std::uint32_t Find3DProvider(std::uint32_t name) override {
        provider_name = name;
        events.push_back("find-provider:" + std::to_string(name));
        return provider;
    }
    std::int32_t Open3DProvider(std::uint32_t address) override {
        events.push_back("open-provider:" + std::to_string(address));
        return provider_open;
    }
    std::uint32_t ProviderAttribute(std::uint32_t address,
                                    const char* name) override {
        events.push_back("attribute:" + std::to_string(address) + ":" + name);
        return supported;
    }
    std::uint32_t Open3DListener(std::uint32_t address) override {
        events.push_back("listener:" + std::to_string(address));
        return listener;
    }
    pal4::AudioListenerPositionBits ListenerPosition() override {
        events.push_back("position-globals");
        return {0x3f800000u, 0u, 0x40000000u};
    }
    void Set3DPosition(std::uint32_t address,
                       pal4::AudioListenerPositionBits position) override {
        Check(address == listener, "3D position targets opened listener");
        position_seen = position;
        events.push_back("set-position");
    }
    void Set3DOrientation(
        std::uint32_t address, pal4::AudioListenerPositionBits front,
        pal4::AudioListenerPositionBits up) override {
        Check(address == listener, "3D orientation targets opened listener");
        front_seen = front;
        up_seen = up;
        events.push_back("set-orientation");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        const auto address = next_memory;
        next_memory += 0x1000u;
        events.push_back("allocate:" + std::to_string(bytes) + ":" +
                         std::to_string(address));
        return address;
    }
    void CopyPointer(std::uint32_t destination,
                     std::uint32_t source) override {
        pointers[destination] = pointers.at(source);
        events.push_back("copy-pointer:" + std::to_string(destination) +
                         ":" + std::to_string(source));
    }
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override {
        events.push_back("range-noop:" + std::to_string(begin) + ":" +
                         std::to_string(end));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
    std::uint32_t AllocateSample(std::uint32_t address) override {
        Check(address == driver, "2D sample uses opened driver");
        events.push_back("sample2d");
        return sample_calls++ < sample_successes
                   ? 0x8000u + sample_calls * 4u : 0u;
    }
    void InitializeSample(std::uint32_t sample) override {
        events.push_back("init-sample:" + std::to_string(sample));
    }
    std::uint32_t Allocate3DSample(std::uint32_t address) override {
        Check(address == provider, "3D sample uses opened provider");
        events.push_back("sample3d");
        return sample3d_calls++ < sample3d_successes
                   ? 0x9000u + sample3d_calls * 4u : 0u;
    }
    void InsertHandle(pal4::AudioPointerArrayState& array,
                      std::uint32_t handle) override {
        const bool is_2d = &array == &manager->samples2d;
        Check(is_2d || &array == &manager->spare,
              "sample insertion uses corresponding handle pool");
        if (array.begin == 0u)
            array.begin = array.end = is_2d ? 0xb000u : 0xc000u;
        pointers[array.end] = handle;
        array.end += 4u;
        events.push_back(is_2d ? "insert2d" : "insert3d");
    }
    void TraverseTree(pal4::AudioManagerPrefixState&) override {
        events.push_back("traverse-tree");
    }
    void CleanupConfig(pal4::AudioDriverConfigState& config) override {
        Check(config.provider.data == 0xa000u,
              "local default config is cleaned even when caller supplied one");
        events.push_back("cleanup-config");
    }
};

std::size_t EventIndex(const std::vector<std::string>& events,
                       const std::string& value);

struct SystemShutdownRuntime final : pal4::IAudioSystemShutdownRuntime {
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::uint32_t> nodes;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::vector<std::string> events;
    bool package_open = false;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        return pointers.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t value) override {
        pointers[address] = value;
        events.push_back("write-pointer:" + std::to_string(address));
    }
    void StopObject(std::uint32_t object) override {
        events.push_back("stop:" + std::to_string(object));
        if (object == 0xb1u)
            pointers[0x2000u] = 0u;
    }
    void DeleteObject(std::uint32_t object,
                      std::uint32_t flags) override {
        events.push_back("delete:" + std::to_string(object) + ":" +
                         std::to_string(flags));
    }
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override {
        events.push_back("range:" + std::to_string(begin) + ":" +
                         std::to_string(end));
    }
    void ReleaseSample(std::uint32_t sample) override {
        events.push_back("release2d:" + std::to_string(sample));
    }
    void Release3DSample(std::uint32_t sample) override {
        events.push_back("release3d:" + std::to_string(sample));
    }
    bool PackageOpen(const pal4::AudioManagerPrefixState&) override {
        events.push_back("package-check");
        return package_open;
    }
    void ClosePackage(pal4::AudioManagerPrefixState&) override {
        events.push_back("package-close");
    }
    void ClearSoundPool() override { events.push_back("sound-pool"); }
    void Close3DListener(std::uint32_t listener) override {
        events.push_back("close-listener:" + std::to_string(listener));
    }
    void Close3DProvider(std::uint32_t provider) override {
        events.push_back("close-provider:" + std::to_string(provider));
    }
    void CloseDigitalDriver(std::uint32_t driver) override {
        events.push_back("close-driver:" + std::to_string(driver));
    }
    void ShutdownMiles() override { events.push_back("miles-shutdown"); }
    std::uint32_t ReadNodeWord(std::uint32_t node,
                               std::uint32_t offset) override {
        return nodes.at({node, offset});
    }
    void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                       std::uint32_t value) override {
        nodes[{node, offset}] = value;
        events.push_back("node-write:" + std::to_string(offset) + ":" +
                         std::to_string(value));
    }
    void FreeNodesRecursive(std::uint32_t root) override {
        events.push_back("recursive:" + std::to_string(root));
    }
    void FirstElement(pal4::AudioManagerPrefixState&) override {}
    std::uint32_t NextNode(std::uint32_t node) override {
        events.push_back("next:" + std::to_string(node));
        return successors.at(node);
    }
    void RemoveNode(pal4::AudioManagerPrefixState&,
                    std::uint32_t node) override {
        events.push_back("remove:" + std::to_string(node));
    }
    std::uint32_t Free(std::uint32_t) override {
        Check(false, "shutdown resets tree nodes without freeing root");
        return 0u;
    }
};

struct VolumeRuntime final : pal4::IAudioSystemVolumeRuntime {
    pal4::AudioManagerPrefixState* manager = nullptr;
    std::map<std::uint32_t, std::uint32_t> pointers;
    std::map<std::uint32_t, std::uint32_t> kinds;
    std::vector<std::string> events;
    bool shrink_on_refresh = false;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        return pointers.at(address);
    }
    std::uint32_t SetStreamVolume(std::uint32_t stream,
                                  std::uint32_t bits) override {
        events.push_back("stream:" + std::to_string(stream) + ":" +
                         std::to_string(bits) + ":" +
                         std::to_string(manager->volume_bits[1]) + ":" +
                         std::to_string(manager->volume_bits[2]));
        return 0x39u;
    }
    std::uint32_t ObjectKind(std::uint32_t object) override {
        events.push_back("kind:" + std::to_string(object));
        return kinds.at(object);
    }
    std::uint32_t RefreshObject(std::uint32_t object) override {
        events.push_back("refresh:" + std::to_string(object));
        if (shrink_on_refresh && object == 0xc1u)
            manager->samples3d.end = manager->samples3d.begin + 4u;
        return 0x48u;
    }
};

struct PlaybackRuntime final : pal4::IAudioPlaybackRuntime {
    std::vector<std::string> events;
    std::map<std::uint32_t, std::uint32_t> pointers;
    pal4::AudioManagerPrefixState* manager = nullptr;
    bool refill_music_pool = false;
    bool refill_3d_pool = false;
    pal4::AudioListenerPositionBits position_seen{};
    std::int32_t status = 0;
    std::uint32_t memory = 0x3000u;
    void CleanupSounds(pal4::AudioPointerArrayState& array,
                       std::uint32_t source) override {
        events.push_back("cleanup:" + std::to_string(source));
        array.end = array.begin;
        if (refill_music_pool && manager != nullptr &&
            &array == &manager->effects) {
            manager->samples2d.begin = 0x4000u;
            manager->samples2d.end = 0x4004u;
        }
        if (refill_3d_pool && manager != nullptr &&
            &array == &manager->samples3d) {
            manager->spare.begin = 0x5000u;
            manager->spare.end = 0x5004u;
        }
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return memory;
    }
    std::uint32_t ConstructSoundObject(std::uint32_t address,
                                       std::uint32_t identifier,
                                       std::int32_t flags) override {
        events.push_back("construct:" + std::to_string(address) + ":" +
                         std::to_string(identifier) + ":" +
                         std::to_string(flags));
        return address + 16u;
    }
    std::int32_t PlaySoundObject(std::uint32_t object,
                                 std::uint32_t source,
                                 pal4::AudioManagerPrefixState&,
                                 std::int32_t mode) override {
        events.push_back("play:" + std::to_string(object) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(mode));
        return status;
    }
    void InsertObject(pal4::AudioPointerArrayState& array,
                      std::uint32_t object) override {
        events.push_back("insert:" + std::to_string(object));
        array.end += 4u;
    }
    std::uint32_t ObjectId(std::uint32_t object) override {
        events.push_back("id:" + std::to_string(object));
        return object + 4u;
    }
    void Log(std::uint32_t message_id) override {
        events.push_back("log:" + std::to_string(message_id));
    }
    void DestroyObject(std::uint32_t object,
                       std::uint32_t flags) override {
        events.push_back("destroy:" + std::to_string(object) + ":" +
                         std::to_string(flags));
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return pointers.at(address);
    }
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override {
        events.push_back("range:" + std::to_string(begin) + ":" +
                         std::to_string(end));
    }
    std::uint32_t ConstructMusicObject(std::uint32_t address,
                                       std::uint32_t identifier) override {
        events.push_back("music-construct:" + std::to_string(address) +
                         ":" + std::to_string(identifier));
        return address + 32u;
    }
    std::int32_t PlayMusicObject(std::uint32_t object,
                                 std::uint32_t sample,
                                 std::uint32_t source,
                                 pal4::AudioManagerPrefixState&,
                                 std::int32_t mode) override {
        events.push_back("music-play:" + std::to_string(object) + ":" +
                         std::to_string(sample) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(mode));
        return status;
    }
    std::uint32_t Construct3DObject(std::uint32_t address,
                                    std::uint32_t identifier) override {
        events.push_back("3d-construct:" + std::to_string(address) + ":" +
                         std::to_string(identifier));
        return address + 48u;
    }
    void Set3DSourcePosition(
        std::uint32_t object,
        pal4::AudioListenerPositionBits position) override {
        position_seen = position;
        events.push_back("3d-position:" + std::to_string(object));
    }
    std::int32_t Play3DObject(
        std::uint32_t object, std::uint32_t sample,
        std::uint32_t source, pal4::AudioManagerPrefixState&,
        std::int32_t first_mode, std::int32_t second_mode,
        std::int32_t third_mode) override {
        events.push_back("3d-play:" + std::to_string(object) + ":" +
                         std::to_string(sample) + ":" +
                         std::to_string(source) + ":" +
                         std::to_string(first_mode) + ":" +
                         std::to_string(second_mode) + ":" +
                         std::to_string(third_mode));
        return status;
    }
};

struct ProviderSearchRuntime final : pal4::IAudioProviderSearchRuntime {
    std::vector<std::pair<std::uint32_t, const char*>> providers;
    std::map<std::uint32_t, std::int32_t> open_results;
    std::vector<std::string> events;
    bool last_error = false;
    bool Enumerate(std::uint32_t& cursor, std::uint32_t& provider,
                   const char*& name) override {
        events.push_back("enumerate:" + std::to_string(cursor));
        if (cursor >= providers.size())
            return false;
        provider = providers[cursor].first;
        name = providers[cursor].second;
        ++cursor;
        return true;
    }
    std::int32_t Open(std::uint32_t provider) override {
        events.push_back("open:" + std::to_string(provider));
        return open_results.at(provider);
    }
    void Close(std::uint32_t provider) override {
        events.push_back("close:" + std::to_string(provider));
    }
    bool HasLastError() override {
        events.push_back("has-error");
        return last_error;
    }
    void ReadLastError() override { events.push_back("read-error"); }
    void Log(std::uint32_t message_id) override {
        events.push_back("log:" + std::to_string(message_id));
    }
};

void Find3DProviderBranches() {
    ProviderSearchRuntime runtime;
    runtime.providers = {{11u, "Software 3D"}, {22u, "Hardware 3D"},
                         {33u, "Hardware 3D accelerated"}};
    runtime.open_results = {{11u, 0}, {22u, 1}, {33u, 0}};
    Check(pal4::AudioSystem::Find3DProvider("Hardware", runtime) == 33u &&
              runtime.events == std::vector<std::string>{
                  "enumerate:0", "enumerate:1", "open:22",
                  "enumerate:2", "open:33", "close:33", "log:93"},
          "matching provider search skips unrelated and unusable entries");

    runtime.events.clear();
    runtime.last_error = true;
    Check(pal4::AudioSystem::Find3DProvider("Missing", runtime) == 11u &&
              runtime.events == std::vector<std::string>{
                  "enumerate:0", "enumerate:1", "enumerate:2",
                  "enumerate:3", "has-error", "read-error", "log:94",
                  "enumerate:0", "open:11", "close:11", "log:95"},
          "failed name search restarts enumeration for any usable provider");

    runtime.events.clear();
    runtime.open_results = {{11u, 1}, {22u, 1}, {33u, 1}};
    Check(pal4::AudioSystem::Find3DProvider("Hardware", runtime) == 0u &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "read-error") == 2 &&
              EventIndex(runtime.events, "log:94") <
                  EventIndex(runtime.events, "log:96") &&
              runtime.events.back() == "log:96" &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "close:11") == runtime.events.end(),
          "both exhausted searches log errors and return zero");

    runtime.events.clear();
    runtime.providers.clear();
    runtime.last_error = false;
    Check(pal4::AudioSystem::Find3DProvider("", runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "enumerate:0", "has-error", "log:94",
                  "enumerate:0", "has-error", "log:96"},
          "empty provider directory skips error retrieval");
}

struct CleanupRuntime final : pal4::IAudioCleanupRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, bool> active;
    std::map<std::uint32_t, std::string> names;
    std::map<std::uint32_t, std::int64_t> priorities;
    std::vector<std::string> events;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t object) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(object));
        slots[address] = object;
    }
    bool IsActive(std::uint32_t object) override {
        events.push_back("active:" + std::to_string(object));
        return active.at(object);
    }
    pal4::AudioObjectNameView Name(std::uint32_t object) override {
        events.push_back("name:" + std::to_string(object));
        const auto& name = names.at(object);
        return {name.c_str(), static_cast<std::uint32_t>(name.size())};
    }
    std::int64_t Priority(std::uint32_t object) override {
        events.push_back("priority:" + std::to_string(object));
        return priorities.at(object);
    }
    void Destroy(std::uint32_t object, std::uint32_t flags) override {
        events.push_back("destroy:" + std::to_string(object) + ":" +
                         std::to_string(flags));
    }
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override {
        events.push_back("range:" + std::to_string(begin) + ":" +
                         std::to_string(end));
    }
};

void CleanupSoundBranches() {
    pal4::AudioPointerArrayState array{0x1000u, 0x100cu, 0x1010u};
    CleanupRuntime runtime;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u}, {0x1008u, 33u}};
    runtime.active = {{11u, true}, {22u, false}, {33u, true}};
    runtime.names = {{11u, "other"}, {22u, "target"}, {33u, "target"}};
    runtime.priorities = {{11u, 30}, {22u, 20}, {33u, 10}};
    Check(pal4::AudioSystem::CleanupSounds(
              array, "target", runtime) == 0x1008u &&
              array.end == 0x1008u && runtime.slots.at(0x1004u) == 33u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "active:11", "read:4100", "active:22",
                  "destroy:22:1", "read:4104", "write:4100:33",
                  "range:4104:4108"},
          "first inactive object is deleted before matching and range shrinks");

    runtime.events.clear();
    runtime.active[33u] = true;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u}, {0x1008u, 33u}};
    runtime.active[22u] = true;
    runtime.priorities[22u] = 5;
    array.end = 0x100cu;
    Check(pal4::AudioSystem::CleanupSounds(
              array, "target", runtime) == 0x1008u &&
              array.end == 0x1008u && runtime.slots.at(0x1004u) == 33u &&
              EventIndex(runtime.events, "priority:22") <
                  EventIndex(runtime.events, "destroy:22:1") &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "range:4104:4108") == runtime.events.end(),
          "matching objects choose minimum signed priority without range callback");

    runtime.events.clear();
    runtime.priorities[33u] = -9;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u}, {0x1008u, 33u}};
    array.end = 0x100cu;
    Check(pal4::AudioSystem::CleanupSounds(
              array, "target", runtime) == 0x100cu &&
              array.end == 0x1008u &&
              runtime.events.back() == "destroy:33:1",
          "matching last slot returns old end and supports negative priority");

    runtime.events.clear();
    runtime.priorities[11u] = -20;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u}, {0x1008u, 33u}};
    array.end = 0x100cu;
    Check(pal4::AudioSystem::CleanupSounds(
              array, "absent", runtime) == 0x100cu &&
              array.end == 0x1008u &&
              runtime.slots.at(0x1000u) == 22u &&
              runtime.slots.at(0x1004u) == 33u &&
              EventIndex(runtime.events, "destroy:11:1") <
                  EventIndex(runtime.events, "write:4096:22"),
          "unmatched names select global minimum and return old end");

    runtime.events.clear();
    array.end = array.begin;
    Check(pal4::AudioSystem::CleanupSounds(
              array, "absent", runtime) == array.begin &&
              runtime.events.empty(),
          "empty collection returns its unchanged end");
}

struct ObjectLookupRuntime final : pal4::IAudioObjectLookupRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, std::uint32_t> identifiers;
    std::vector<std::string> events;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    std::uint32_t ObjectId(std::uint32_t object) override {
        events.push_back("id:" + std::to_string(object));
        return identifiers.at(object);
    }
};

void FindSoundObjectBranches() {
    pal4::AudioManagerPrefixState manager{};
    manager.streams = {0x1000u, 0x1008u, 0x1008u};
    manager.effects = {0x2000u, 0x2004u, 0x2004u};
    manager.samples3d = {0x3000u, 0x3004u, 0x3004u};
    ObjectLookupRuntime runtime;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 12u},
                     {0x2000u, 21u}, {0x3000u, 31u}};
    runtime.identifiers = {{11u, 1u}, {12u, 2u},
                           {21u, 2u}, {31u, 2u}};
    Check(pal4::AudioSystem::FindSoundObject(manager, 2u, runtime) == 12u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "id:11", "read:4100", "id:12"},
          "stream match precedes duplicate ids in later collections");

    runtime.events.clear();
    manager.streams.end = manager.streams.begin;
    Check(pal4::AudioSystem::FindSoundObject(manager, 2u, runtime) == 21u &&
              runtime.events == std::vector<std::string>{
                  "read:8192", "id:21"},
          "effects are searched after streams");

    runtime.events.clear();
    manager.effects.end = manager.effects.begin;
    Check(pal4::AudioSystem::FindSoundObject(manager, 2u, runtime) == 31u &&
              runtime.events == std::vector<std::string>{
                  "read:12288", "id:31"},
          "3D objects are searched after effects");

    runtime.events.clear();
    Check(pal4::AudioSystem::FindSoundObject(manager, 9u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:12288", "id:31"},
          "unmatched id returns zero after all live entries");
}

struct StreamUpdateRuntime final : pal4::IAudioStreamUpdateRuntime {
    pal4::AudioManagerPrefixState* manager = nullptr;
    std::map<std::uint32_t, std::uint32_t> slots;
    std::vector<std::string> events;
    bool shrink = false;
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    std::uint32_t Update(std::uint32_t stream,
                         std::uint32_t time_bits) override {
        events.push_back("update:" + std::to_string(stream) + ":" +
                         std::to_string(time_bits));
        if (shrink)
            manager->streams.end = manager->streams.begin + 4u;
        return 0x77u;
    }
};

void UpdateAllSoundsBranches() {
    pal4::AudioManagerPrefixState manager{};
    StreamUpdateRuntime runtime;
    runtime.manager = &manager;
    Check(pal4::AudioSystem::UpdateAllSounds(
              manager, 0x3f000000u, runtime) == 0u &&
              runtime.events.empty(),
          "null stream array has no update side effects");

    manager.streams = {0x1000u, 0x100cu, 0x100cu};
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 0u}, {0x1008u, 33u}};
    Check(pal4::AudioSystem::UpdateAllSounds(
              manager, 0x3f000000u, runtime) == 3u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "update:11:1056964608", "read:4100",
                  "read:4104", "update:33:1056964608"},
          "non-null streams update in order and null entries are skipped");

    runtime.events.clear();
    runtime.shrink = true;
    Check(pal4::AudioSystem::UpdateAllSounds(
              manager, 0x40000000u, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "read:4096", "update:11:1073741824"},
          "update callback can shrink the live stream array");

    runtime.events.clear();
    manager.streams.end = manager.streams.begin;
    Check(pal4::AudioSystem::UpdateAllSounds(
              manager, 0u, runtime) == 0u && runtime.events.empty(),
          "allocated but empty stream array returns zero count");
}

struct ObjectRemovalRuntime final : pal4::IAudioObjectRemovalRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::uint32_t kind = 0u;
    std::vector<std::string> events;
    std::uint32_t Kind(std::uint32_t object) override {
        events.push_back("kind:" + std::to_string(object));
        return kind;
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t object) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(object));
        slots[address] = object;
    }
    void Destroy(std::uint32_t object, std::uint32_t flags) override {
        events.push_back("destroy:" + std::to_string(object) + ":" +
                         std::to_string(flags));
    }
};

struct HandleRuntime final : pal4::IAudioHandleRuntime {
    pal4::AudioManagerPrefixState manager{};
    std::map<std::uint32_t, std::uint32_t> slots;
    std::map<std::uint32_t, std::uint32_t> identifiers;
    std::map<std::uint32_t, std::uint32_t> kinds;
    std::vector<std::string> events;
    std::uint32_t playing_result = 0x102u;
    pal4::AudioListenerPositionBits position_seen{};
    pal4::AudioManagerPrefixState& GetManager() override {
        events.push_back("manager");
        return manager;
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    std::uint32_t ObjectId(std::uint32_t object) override {
        events.push_back("id:" + std::to_string(object));
        return identifiers.at(object);
    }
    std::uint32_t Kind(std::uint32_t object) override {
        events.push_back("kind:" + std::to_string(object));
        return kinds.at(object);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t object) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(object));
        slots[address] = object;
    }
    void Destroy(std::uint32_t object, std::uint32_t flags) override {
        events.push_back("destroy:" + std::to_string(object) + ":" +
                         std::to_string(flags));
    }
    void Start(std::uint32_t object) override {
        events.push_back("start:" + std::to_string(object));
    }
    void Stop(std::uint32_t object) override {
        events.push_back("stop:" + std::to_string(object));
    }
    void Pause(std::uint32_t object) override {
        events.push_back("pause:" + std::to_string(object));
    }
    std::uint32_t IsPlaying(std::uint32_t object) override {
        events.push_back("playing:" + std::to_string(object));
        return playing_result;
    }
    std::uint32_t FadeIn(std::uint32_t object,
                         std::uint32_t time_bits) override {
        events.push_back("fade-in:" + std::to_string(object) + ":" +
                         std::to_string(time_bits));
        return 0x91u;
    }
    std::uint32_t FadeOut(std::uint32_t object,
                          std::uint32_t time_bits) override {
        events.push_back("fade-out:" + std::to_string(object) + ":" +
                         std::to_string(time_bits));
        return 0x92u;
    }
    void WriteProperty(std::uint32_t object,
                       std::uint32_t value) override {
        events.push_back("property:" + std::to_string(object) + ":" +
                         std::to_string(value));
    }
    void Set3DPosition(
        std::uint32_t object,
        pal4::AudioListenerPositionBits position) override {
        position_seen = position;
        events.push_back("position:" + std::to_string(object));
    }
};

void AudioHandleBranches() {
    HandleRuntime runtime;
    runtime.manager.streams = {0x1000u, 0x1008u, 0x1008u};
    runtime.manager.effects = {0x2000u, 0x2004u, 0x2004u};
    runtime.manager.samples3d = {0x3000u, 0x3004u, 0x3004u};
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u},
                     {0x2000u, 33u}, {0x3000u, 44u}};
    runtime.identifiers = {{11u, 1u}, {22u, 2u},
                           {33u, 3u}, {44u, 4u}};
    runtime.kinds = {{11u, 0u}, {22u, 0u},
                     {33u, 1u}, {44u, 2u}};
    Check(pal4::AudioHandle::IsActive({2u}, runtime) &&
              runtime.events == std::vector<std::string>{
                  "manager", "read:4096", "id:11", "read:4100", "id:22"},
          "handle activity resolves id through the manager once");

    runtime.events.clear();
    Check(pal4::AudioHandle::Play({2u}, runtime) == 1u &&
              runtime.manager.streams.end == 0x1004u &&
              runtime.events == std::vector<std::string>{
                  "manager", "read:4096", "id:11", "read:4100", "id:22",
                  "start:22", "manager", "kind:22", "read:4096",
                  "read:4100", "destroy:22:1"},
          "play starts object then obtains manager again and removes it");

    runtime.events.clear();
    Check(pal4::AudioHandle::Stop({3u}, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "manager", "read:4096", "id:11", "read:8192",
                  "id:33", "stop:33"},
          "stop dispatches virtual slot without deleting the object");

    runtime.events.clear();
    Check(pal4::AudioHandle::Pause({4u}, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "manager", "read:4096", "id:11", "read:8192",
                  "id:33", "read:12288", "id:44", "pause:44"},
          "pause resolves the third collection and leaves it live");

    runtime.events.clear();
    Check(pal4::AudioHandle::IsPlaying({4u}, runtime) == 2u &&
              runtime.events.back() == "playing:44",
          "is-playing forwards only the virtual return low byte");

    runtime.events.clear();
    Check(!pal4::AudioHandle::IsActive({99u}, runtime) &&
              runtime.events.back() == "id:44",
          "missing handle is inactive after searching all collections");
    runtime.events.clear();
    Check(pal4::AudioHandle::Play({99u}, runtime) == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "start:22") == runtime.events.end() &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "manager") == 1,
          "missing play does not start or remove an object");
    runtime.events.clear();
    Check(pal4::AudioHandle::Stop({99u}, runtime) == 0u &&
              pal4::AudioHandle::Pause({99u}, runtime) == 0u &&
              pal4::AudioHandle::IsPlaying({99u}, runtime) == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "playing:44") == runtime.events.end(),
          "missing stop, pause and play-state calls return zero");

    runtime.events.clear();
    Check(pal4::AudioHandle::FadeIn(
              {1u}, 0x3f800000u, runtime) == 0x91u &&
              runtime.events == std::vector<std::string>{
                  "manager", "read:4096", "id:11", "kind:11",
                  "fade-in:11:1065353216"},
          "stream fade-in forwards exact float bits and return value");
    runtime.events.clear();
    Check(pal4::AudioHandle::FadeOut(
              {1u}, 0x40000000u, runtime) == 0x92u &&
              runtime.events.back() == "fade-out:11:1073741824",
          "stream fade-out forwards exact float bits and return value");
    runtime.events.clear();
    Check(pal4::AudioHandle::FadeIn(
              {3u}, 0u, runtime) == 1u &&
              runtime.events.back() == "kind:33" &&
              pal4::AudioHandle::FadeOut({3u}, 0u, runtime) == 1u &&
              runtime.events.back() == "kind:33",
          "non-stream fades return object kind without invoking stream fade");
    runtime.events.clear();
    Check(pal4::AudioHandle::FadeIn(
              {99u}, 0u, runtime) == 0u &&
              pal4::AudioHandle::FadeOut({99u}, 0u, runtime) == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "kind:11") == runtime.events.end(),
          "missing fades return zero without type dispatch");

    runtime.events.clear();
    Check(pal4::AudioHandle::SetProperty(
              {1u}, 0xabcdu, runtime) == 0xabcdu &&
              runtime.events.back() == "property:11:43981",
          "stream property writes object offset 52 and returns input word");
    runtime.events.clear();
    Check(pal4::AudioHandle::SetProperty(
              {3u}, 7u, runtime) == 1u &&
              runtime.events.back() == "kind:33" &&
              pal4::AudioHandle::SetProperty({99u}, 7u, runtime) == 0u,
          "property write rejects non-stream and missing objects");

    runtime.events.clear();
    const pal4::AudioListenerPositionBits position{
        0x3f800000u, 0x80000000u, 0x40400000u};
    Check(pal4::AudioHandle::Set3DPosition(
              {4u}, position, runtime) == 1u &&
              runtime.events.back() == "position:44" &&
              runtime.position_seen.x == position.x &&
              runtime.position_seen.y == position.y &&
              runtime.position_seen.z == position.z,
          "3D object position forwards all three coordinate words");
    runtime.events.clear();
    Check(pal4::AudioHandle::Set3DPosition(
              {1u}, position, runtime) == 0u &&
              runtime.events.back() == "kind:11" &&
              pal4::AudioHandle::Set3DPosition(
                  {99u}, position, runtime) == 0u,
          "position update rejects stream and missing objects");
}

void RemoveSampleBranches() {
    pal4::AudioManagerPrefixState manager{};
    manager.streams = {0x1000u, 0x100cu, 0x100cu};
    manager.effects = {0x2000u, 0x2008u, 0x2008u};
    manager.samples3d = {0x3000u, 0x3008u, 0x3008u};
    ObjectRemovalRuntime runtime;
    runtime.slots = {{0x1000u, 11u}, {0x1004u, 22u},
                     {0x1008u, 33u}, {0x2000u, 44u},
                     {0x2004u, 55u}, {0x3000u, 66u},
                     {0x3004u, 77u}};
    Check(pal4::AudioManager::RemoveSampleByKind(manager, 22u, runtime) ==
                  0x1008u && manager.streams.end == 0x1008u &&
              runtime.slots.at(0x1004u) == 33u &&
              runtime.events == std::vector<std::string>{
                  "kind:22", "read:4096", "read:4100",
                  "destroy:22:1", "read:4104", "write:4100:33"},
          "type-zero object is destroyed and removed only from streams");

    runtime.events.clear();
    runtime.kind = 1u;
    Check(pal4::AudioManager::RemoveSampleByKind(manager, 44u, runtime) ==
                  0x2004u && manager.effects.end == 0x2004u &&
              runtime.slots.at(0x2000u) == 55u &&
              runtime.events == std::vector<std::string>{
                  "kind:44", "read:8192", "destroy:44:1",
                  "read:8196", "write:8192:55"},
          "nonzero type searches effects before 3D objects");

    runtime.events.clear();
    Check(pal4::AudioManager::RemoveSampleByKind(manager, 66u, runtime) ==
                  0x3008u && manager.samples3d.end == 0x3004u &&
              runtime.slots.at(0x3000u) == 77u &&
              runtime.events == std::vector<std::string>{
                  "kind:66", "read:8192", "read:12288",
                  "destroy:66:1", "read:12292", "write:12288:77"},
          "3D fallback returns the old end after compaction");

    runtime.events.clear();
    Check(pal4::AudioManager::RemoveSampleByKind(manager, 99u, runtime) ==
                  0x3004u && runtime.events == std::vector<std::string>{
                  "kind:99", "read:8192", "read:12288"},
          "missing nonzero-type object returns 3D end unchanged");

    runtime.events.clear();
    runtime.kind = 0u;
    Check(pal4::AudioManager::RemoveSampleByKind(manager, 99u, runtime) ==
                  0x1008u && runtime.events == std::vector<std::string>{
                  "kind:99", "read:4096", "read:4100"},
          "missing type-zero object does not inspect other collections");
}

struct SamplePoolRuntime final : pal4::IAudioSamplePoolRuntime {
    std::map<std::uint32_t, std::uint32_t> slots;
    std::vector<std::string> events;
    std::uint32_t next_allocation = 0x5000u;
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate:" + std::to_string(bytes));
        return next_allocation;
    }
    std::uint32_t ReadPointer(std::uint32_t address) override {
        events.push_back("read:" + std::to_string(address));
        return slots.at(address);
    }
    void WritePointer(std::uint32_t address,
                      std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address) + ":" +
                         std::to_string(value));
        slots[address] = value;
    }
    void RangeNoOp(std::uint32_t begin, std::uint32_t end) override {
        events.push_back("range:" + std::to_string(begin) + ":" +
                         std::to_string(end));
    }
    void Free(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
    }
};

void AddSampleBranches() {
    pal4::AudioManagerPrefixState manager{};
    SamplePoolRuntime runtime;
    Check(pal4::AudioManager::AddSample(manager, 11u, runtime) == 0x5004u &&
              manager.samples2d.begin == 0x5000u &&
              manager.samples2d.end == 0x5004u &&
              manager.samples2d.capacity == 0x5004u &&
              runtime.slots.at(0x5000u) == 11u &&
              runtime.events == std::vector<std::string>{
                  "allocate:4", "write:20480:11", "range:0:0", "free:0"},
          "first sample allocates one slot and forwards null old buffer cleanup");

    runtime.events.clear();
    runtime.next_allocation = 0x6000u;
    Check(pal4::AudioManager::AddSample(manager, 22u, runtime) == 0x6008u &&
              manager.samples2d.begin == 0x6000u &&
              manager.samples2d.capacity == 0x6008u &&
              runtime.slots.at(0x6000u) == 11u &&
              runtime.slots.at(0x6004u) == 22u &&
              runtime.events == std::vector<std::string>{
                  "allocate:8", "read:20480", "write:24576:11",
                  "write:24580:22", "range:20480:20484", "free:20480"},
          "one full slot grows capacity by one and copies old handle");

    runtime.events.clear();
    manager.samples2d.capacity = 0x600cu;
    Check(pal4::AudioManager::AddSample(manager, 33u, runtime) == 0x600cu &&
              runtime.slots.at(0x6008u) == 33u &&
              runtime.events == std::vector<std::string>{"write:24584:33"},
          "spare-capacity insertion writes the end slot without allocation");

    runtime.events.clear();
    runtime.next_allocation = 0x7000u;
    Check(pal4::AudioManager::AddSample(manager, 44u, runtime) == 0x7010u &&
              manager.samples2d.capacity == 0x7018u &&
              runtime.slots.at(0x7000u) == 11u &&
              runtime.slots.at(0x7004u) == 22u &&
              runtime.slots.at(0x7008u) == 33u &&
              runtime.slots.at(0x700cu) == 44u &&
              runtime.events.front() == "allocate:24" &&
              runtime.events.back() == "free:24576",
          "full pool of three doubles capacity and preserves sample order");

    runtime.events.clear();
    runtime.next_allocation = 0x8000u;
    Check(pal4::AudioManager::Add3DSample(
              manager, 55u, runtime) == 0x8004u &&
              manager.spare.begin == 0x8000u &&
              manager.spare.end == 0x8004u &&
              manager.samples2d.begin == 0x7000u &&
              runtime.slots.at(0x8000u) == 55u &&
              runtime.events == std::vector<std::string>{
                  "allocate:4", "write:32768:55", "range:0:0", "free:0"},
          "3D append initializes only the spare sample pool");

    runtime.events.clear();
    manager.spare.capacity = 0x8008u;
    Check(pal4::AudioManager::Add3DSample(
              manager, 66u, runtime) == 0x8008u &&
              runtime.slots.at(0x8004u) == 66u &&
              runtime.events == std::vector<std::string>{"write:32772:66"},
          "3D append reuses available spare-pool capacity");

    runtime.events.clear();
    runtime.next_allocation = 0x9000u;
    Check(pal4::AudioManager::Add3DSample(
              manager, 77u, runtime) == 0x900cu &&
              manager.spare.capacity == 0x9010u &&
              runtime.slots.at(0x9000u) == 55u &&
              runtime.slots.at(0x9004u) == 66u &&
              runtime.slots.at(0x9008u) == 77u &&
              runtime.events.front() == "allocate:16" &&
              runtime.events.back() == "free:32768",
          "full 3D sample pool doubles while preserving handles");
}

void Play3DSoundBranches() {
    pal4::AudioManagerPrefixState manager{};
    manager.state = 12u;
    manager.samples3d = {0x2000u, 0x2000u, 0x2010u};
    PlaybackRuntime runtime;
    runtime.manager = &manager;
    runtime.pointers = {{0x5000u, 0x7777u}, {0x5004u, 0x8888u}};
    const pal4::AudioListenerPositionBits position{0x3f800000u,
                                                   0x80000000u,
                                                   0x40400000u};
    std::uint32_t output = 99u;
    Check(&pal4::AudioSystem::Play3DSound(
              manager, output, 0xa000u, position, 1, 2, 3, runtime) ==
                  &output && output == 0u && runtime.events.empty(),
          "3D playback requires both digital driver and provider");
    manager.driver = 0xd0u;
    Check(&pal4::AudioSystem::Play3DSound(
              manager, output, 0xa000u, position, 1, 2, 3, runtime) ==
                  &output && output == 0u && runtime.events.empty(),
          "3D playback requires a provider after driver initialization");
    manager.provider = 0xe0u;
    Check(&pal4::AudioSystem::Play3DSound(
              manager, output, 0xa000u, position, 1, 2, 3, runtime) ==
                  &output && output == 0u && manager.state == 12u &&
              runtime.events == std::vector<std::string>{"cleanup:40960"},
          "empty 3D sample pool returns after unsuccessful cleanup");

    runtime.events.clear();
    runtime.refill_3d_pool = true;
    Check(&pal4::AudioSystem::Play3DSound(
              manager, output, 0xa100u, position, 4, 5, 6, runtime) ==
                  &output && output == 0x3034u && manager.state == 13u &&
              manager.spare.end == manager.spare.begin &&
              manager.samples3d.end == 0x2004u &&
              runtime.position_seen.x == position.x &&
              runtime.position_seen.y == position.y &&
              runtime.position_seen.z == position.z &&
              runtime.events == std::vector<std::string>{
                  "cleanup:41216", "read:20480", "range:20480:20484",
                  "allocate:96", "3d-construct:12288:12",
                  "3d-position:12336", "3d-play:12336:30583:41216:4:5:6",
                  "insert:12336", "id:12336"},
          "3D cleanup replenishes pool, passes position and inserts object");

    runtime.refill_3d_pool = false;
    for (const auto status : {1, 4, 2, 3}) {
        runtime.events.clear();
        runtime.status = status;
        manager.spare.end = manager.spare.begin + 8u;
        Check(&pal4::AudioSystem::Play3DSound(
                  manager, output, 0xa200u, position, 7, 8, 9,
                  runtime) == &output && output == 0u &&
                  manager.spare.end == manager.spare.begin + 4u &&
                  runtime.events.front() == "read:20484" &&
                  EventIndex(runtime.events,
                             "3d-play:12336:34952:41472:7:8:9") <
                      EventIndex(runtime.events,
                                 status == 2 ? "log:125"
                                 : status == 3 ? "log:91" : "log:92") &&
                  (status == 3
                       ? std::find(runtime.events.begin(),
                                   runtime.events.end(),
                                   "destroy:12336:1") ==
                             runtime.events.end()
                       : runtime.events.back() == "destroy:12336:1"),
              "3D playback status preserves native logging and ownership");
    }
}

void PlayMusicBranches() {
    pal4::AudioManagerPrefixState manager{};
    manager.state = 7u;
    manager.effects = {0x2000u, 0x2000u, 0x2010u};
    PlaybackRuntime runtime;
    runtime.manager = &manager;
    runtime.pointers = {{0x4000u, 0x5555u}, {0x4004u, 0x6666u}};
    std::uint32_t output = 99u;
    Check(&pal4::AudioSystem::PlayMusic(
              manager, output, 0xa000u, 3, runtime) == &output &&
              output == 0u && runtime.events.empty(),
          "music playback without digital driver returns zero");

    manager.driver = 0xd0u;
    runtime.refill_music_pool = true;
    Check(&pal4::AudioSystem::PlayMusic(
              manager, output, 0xa000u, 3, runtime) == &output &&
              output == 0x3024u && manager.state == 8u &&
              manager.samples2d.end == manager.samples2d.begin &&
              manager.effects.end == 0x2004u &&
              runtime.events == std::vector<std::string>{
                  "cleanup:40960", "read:16384", "range:16384:16388",
                  "allocate:48", "music-construct:12288:7",
                  "music-play:12320:21845:40960:3",
                  "insert:12320", "id:12320"},
          "empty music pool cleanup can replenish and consume a sample");

    runtime.events.clear();
    runtime.refill_music_pool = false;
    Check(&pal4::AudioSystem::PlayMusic(
              manager, output, 0xa100u, 4, runtime) == &output &&
              output == 0u && manager.state == 8u &&
              runtime.events == std::vector<std::string>{"cleanup:41216"},
          "unreplenished pool returns without allocating a music object");

    for (const auto status : {1, 4, 2, 3}) {
        runtime.events.clear();
        runtime.status = status;
        manager.samples2d.end = manager.samples2d.begin + 8u;
        Check(&pal4::AudioSystem::PlayMusic(
                  manager, output, 0xa200u, 5, runtime) == &output &&
                  output == 0u &&
                  manager.samples2d.end == manager.samples2d.begin + 4u &&
                  runtime.events.front() == "read:16388" &&
                  EventIndex(runtime.events, "music-play:12320:26214:41472:5") <
                      EventIndex(runtime.events,
                                 status == 2 ? "log:125"
                                 : status == 3 ? "log:91" : "log:92") &&
                  (status == 3
                       ? std::find(runtime.events.begin(),
                                   runtime.events.end(),
                                   "destroy:12320:1") ==
                             runtime.events.end()
                       : runtime.events.back() == "destroy:12320:1"),
              "music status uses native log and ownership branch");
    }
}

void PlaySoundBranches() {
    pal4::AudioManagerPrefixState manager{};
    manager.state = 5u;
    manager.default_provider_count = 2u;
    manager.streams = {0x1000u, 0x1004u, 0x1010u};
    PlaybackRuntime runtime;
    std::uint32_t output = 0x7777u;
    Check(&pal4::AudioSystem::PlaySound(
              manager, output, 0xa000u, 3, 7, runtime) == &output &&
              output == 0u && manager.state == 5u &&
              runtime.events.empty(),
          "missing driver zeros output without allocating sound object");

    manager.driver = 0xd0u;
    Check(&pal4::AudioSystem::PlaySound(
              manager, output, 0xa000u, 3, 7, runtime) == &output &&
              output == 0x3014u && manager.state == 6u &&
              manager.streams.end == 0x1008u &&
              runtime.events == std::vector<std::string>{
                  "allocate:64", "construct:12288:5:7",
                  "play:12304:40960:3", "insert:12304",
                  "id:12304"},
          "success constructs object, inserts stream and returns its id");

    runtime.events.clear();
    runtime.status = 1;
    Check(&pal4::AudioSystem::PlaySound(
              manager, output, 0xa100u, 4, 8, runtime) == &output &&
              output == 0u && manager.state == 7u &&
              runtime.events == std::vector<std::string>{
                  "cleanup:41216", "allocate:64",
                  "construct:12288:6:8", "play:12304:41216:4",
                  "log:90", "destroy:12304:1"},
          "at-capacity stream cleanup precedes logged status-one destruction");
    for (const auto status : {2, 4, 3}) {
        runtime.events.clear();
        runtime.status = status;
        manager.streams.end = manager.streams.begin;
        Check(&pal4::AudioSystem::PlaySound(
                  manager, output, 0xa200u, 5, 9, runtime) == &output &&
                  output == 0u &&
                  runtime.events.back() == "destroy:12304:1" &&
                  (status == 2
                       ? std::find(runtime.events.begin(),
                                   runtime.events.end(), "log:90") ==
                             runtime.events.end() &&
                             std::find(runtime.events.begin(),
                                       runtime.events.end(), "log:91") ==
                                 runtime.events.end()
                       : EventIndex(runtime.events,
                                    status == 4 ? "log:90" : "log:91") <
                             runtime.events.size() - 1u),
              "error status maps to native log and destroying branch");
    }
}

void ToggleAudioVolumes() {
    pal4::AudioManagerPrefixState manager{};
    manager.volume_bits[0] = 0x3f000000u;
    manager.volume_bits[1] = 0x3e800000u;
    manager.volume_bits[2] = 0x3e000000u;
    manager.streams = {0x1000u, 0x1008u, 0x1008u};
    manager.effects = {0x2000u, 0x2008u, 0x2008u};
    manager.samples3d = {0x3000u, 0x3008u, 0x3008u};
    pal4::AudioVolumeGlobalsState globals{};
    VolumeRuntime runtime;
    runtime.manager = &manager;
    runtime.pointers = {{0x1000u, 0xa1u}, {0x1004u, 0u},
                        {0x2000u, 0xb1u}, {0x2004u, 0xb2u},
                        {0x3000u, 0xc1u}, {0x3004u, 0xc2u}};
    runtime.kinds = {{0xb1u, 1u}, {0xb2u, 2u},
                     {0xc1u, 2u}, {0xc2u, 1u}};
    Check(pal4::AudioSystem::SetActive(
              manager, true, globals, runtime) == 2u &&
              globals.guard == 7u &&
              globals.saved_bits[0] == 0x3f000000u &&
              globals.saved_bits[1] == 0x3e800000u &&
              globals.saved_bits[2] == 0x3e000000u &&
              runtime.events == std::vector<std::string>{
                  "stream:161:1056964608:1048576000:1040187392",
                  "kind:177", "refresh:177", "kind:178",
                  "kind:193", "refresh:193", "kind:194"},
          "enable snapshots volumes then refreshes only matching object kinds");

    runtime.events.clear();
    runtime.shrink_on_refresh = true;
    manager.volume_bits[0] = 0x3f800000u;
    manager.volume_bits[1] = 0x40000000u;
    manager.volume_bits[2] = 0x40400000u;
    Check(pal4::AudioSystem::SetActive(
              manager, false, globals, runtime) == 1u &&
              manager.volume_bits[0] == 0u &&
              manager.volume_bits[1] == 0u &&
              manager.volume_bits[2] == 0u &&
              globals.saved_bits[0] == 0x3f800000u &&
              globals.saved_bits[1] == 0x40000000u &&
              globals.saved_bits[2] == 0x40400000u &&
              runtime.events.front() ==
                  "stream:161:0:1073741824:1077936128" &&
              runtime.events.back() == "refresh:193" &&
              manager.samples3d.end == 0x3004u,
          "disable saves volumes and observes callback-shrunk third array");
    runtime.events.clear();
    runtime.shrink_on_refresh = false;
    Check(pal4::AudioSystem::SetActive(
              manager, true, globals, runtime) == 1u &&
              manager.volume_bits[0] == 0x3f800000u &&
              manager.volume_bits[1] == 0x40000000u &&
              manager.volume_bits[2] == 0x40400000u,
          "later enable restores the most recently saved three volumes");

    pal4::AudioManagerPrefixState empty{};
    empty.volume_bits[1] = 0x41000000u;
    pal4::AudioVolumeGlobalsState partial{};
    partial.guard = 5u;
    partial.saved_bits[0] = 0x3f000000u;
    partial.saved_bits[2] = 0x40000000u;
    runtime.manager = &empty;
    runtime.events.clear();
    Check(pal4::AudioSystem::SetActive(
              empty, 0x7fu, partial, runtime) == 0x7fu &&
              partial.guard == 7u &&
              empty.volume_bits[0] == 0x3f000000u &&
              empty.volume_bits[1] == 0x41000000u &&
              empty.volume_bits[2] == 0x40000000u &&
              runtime.events.empty(),
          "partial guard captures missing group and empty path retains input byte");
}

void ShutdownSystem() {
    pal4::AudioManagerPrefixState manager{};
    manager.streams = {0x1000u, 0x100cu, 0x1010u};
    manager.effects = {0x2000u, 0x2008u, 0x2010u};
    manager.samples3d = {0x3000u, 0x3004u, 0x3010u};
    manager.samples2d = {0x4000u, 0x4008u, 0x4010u};
    manager.spare = {0x5000u, 0x5004u, 0x5010u};
    manager.listener = 0x1100u;
    manager.provider = 0x2200u;
    manager.driver = 0x3300u;
    manager.tree_root = 0x6000u;
    manager.tree_count = 2u;
    pal4::AudioManagerTreeGlobalState tree{0x7000u, 1u};
    SystemShutdownRuntime runtime;
    runtime.pointers = {{0x1000u, 0xa1u}, {0x1004u, 0u},
                        {0x1008u, 0xa3u}, {0x2000u, 0xb1u},
                        {0x2004u, 0xb2u}, {0x3000u, 0xc1u},
                        {0x4000u, 0xd1u}, {0x4004u, 0u},
                        {0x5000u, 0xe1u}};
    runtime.nodes[{0x6000u, 0u}] = 0x6100u;
    runtime.nodes[{0x6000u, 4u}] = 0x6200u;
    runtime.package_open = true;
    Check(pal4::AudioSystem::Shutdown(manager, tree, runtime) == 0x6000u &&
              manager.streams.end == manager.streams.begin &&
              manager.effects.end == manager.effects.begin &&
              manager.samples3d.end == manager.samples3d.begin &&
              manager.samples2d.end == manager.samples2d.begin &&
              manager.spare.end == manager.spare.begin &&
              manager.tree_count == 0u && manager.tree_root == 0x6000u &&
              tree.sentinel == 0x7000u && tree.users == 1u &&
              runtime.pointers.at(0x1000u) == 0u &&
              runtime.pointers.at(0x2000u) == 0u &&
              runtime.nodes.at({0x6000u, 4u}) == 0x7000u &&
              runtime.nodes.at({0x6000u, 0u}) == 0x6000u,
          "shutdown empties collections and resets but retains tree root");
    Check(EventIndex(runtime.events, "stop:161") <
                  EventIndex(runtime.events, "stop:177") &&
              EventIndex(runtime.events, "stop:177") <
                  EventIndex(runtime.events, "stop:193") &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "delete:177:1") == runtime.events.end() &&
              EventIndex(runtime.events, "release2d:209") <
                  EventIndex(runtime.events, "release3d:225") &&
              EventIndex(runtime.events, "package-close") <
                  EventIndex(runtime.events, "sound-pool") &&
              EventIndex(runtime.events, "close-listener:4352") <
                  EventIndex(runtime.events, "close-provider:8704") &&
              EventIndex(runtime.events, "close-provider:8704") <
                  EventIndex(runtime.events, "close-driver:13056") &&
              EventIndex(runtime.events, "close-driver:13056") <
                  EventIndex(runtime.events, "miles-shutdown") &&
              EventIndex(runtime.events, "miles-shutdown") <
                  EventIndex(runtime.events, "recursive:25088"),
          "shutdown follows native object, handle, package and Miles order");

    manager = {};
    manager.tree_root = 0x8000u;
    runtime.events.clear();
    runtime.package_open = false;
    runtime.nodes[{0x8000u, 0u}] = 0x8100u;
    runtime.successors[0x8100u] = 0x8000u;
    Check(pal4::AudioSystem::Shutdown(manager, tree, runtime) == 0x8000u &&
              runtime.events.front() == "range:0:0" &&
              EventIndex(runtime.events, "next:33024") <
                  EventIndex(runtime.events, "remove:33024") &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "package-close") == runtime.events.end() &&
              manager.tree_root == 0x8000u && tree.sentinel == 0x7000u,
          "empty arrays and linked-node tree take alternate shutdown path");
}

std::size_t EventIndex(const std::vector<std::string>& events,
                       const std::string& value) {
    const auto found = std::find(events.begin(), events.end(), value);
    Check(found != events.end(), "expected audio event is present");
    return static_cast<std::size_t>(found - events.begin());
}

void StartSystemFailures() {
    pal4::AudioManagerPrefixState manager{};
    manager.header_flags[0] = 1u;
    SystemRuntime runtime;
    runtime.manager = &manager;
    Check(pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              runtime.events.empty(),
          "already active audio system returns before any runtime call");

    manager.header_flags[0] = 0u;
    manager.driver = manager.provider = manager.listener = 0x99u;
    runtime.startup = false;
    Check(!pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              manager.header_flags[0] == 1u &&
              manager.driver == 0u && manager.provider == 0u &&
              manager.listener == 0u &&
              manager.volume_bits[0] == 0x3f800000u &&
              runtime.events == std::vector<std::string>{
                  "log:83", "redist:mss\\win32\\", "prepare", "startup"},
          "Miles startup failure leaves native active byte set without config");

    manager.header_flags[0] = 0u;
    runtime.events.clear();
    runtime.startup = true;
    runtime.driver = 0u;
    Check(!pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              runtime.events == std::vector<std::string>{
                  "log:83", "redist:mss\\win32\\", "prepare", "startup",
                  "config-empty", "config:Miles Fast 2D",
                  "driver:44100:16:2:0", "last-error", "log:84",
                  "cleanup-config"},
          "digital-driver failure logs last error and cleans local config");

    manager.header_flags[0] = 0u;
    runtime.events.clear();
    runtime.driver = 0x1100u;
    runtime.provider = 0u;
    Check(!pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              runtime.provider_name == 0xa000u &&
              EventIndex(runtime.events, "find-provider:40960") <
                  EventIndex(runtime.events, "log:89") &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "open-provider:0") == runtime.events.end() &&
              runtime.events.back() == "cleanup-config",
          "missing provider skips open and cleans default config");

    manager.header_flags[0] = 0u;
    runtime.events.clear();
    runtime.provider = 0x2200u;
    runtime.provider_open = -1;
    const pal4::AudioDriverConfigState requested{
        22050u, 8u, 1u, {0xb000u, 6u, 6u}};
    Check(!pal4::AudioSystem::Initialize(manager, &requested, runtime) &&
              runtime.provider_name == 0xb000u &&
              EventIndex(runtime.events, "driver:22050:8:1:0") <
                  EventIndex(runtime.events, "open-provider:8704") &&
              runtime.events.back() == "cleanup-config",
          "provider-open failure uses supplied config but cleans local default");
}

void StartSystemSuccess() {
    pal4::AudioManagerPrefixState manager{};
    manager.default_provider_count = 3u;
    manager.sample_capacity = 4u;
    manager.streams = {0x1000u, 0x1004u, 0x1004u};
    manager.samples3d = {0x2000u, 0x2004u, 0x2004u};
    manager.effects = {0x3000u, 0x3004u, 0x3004u};
    SystemRuntime runtime;
    runtime.manager = &manager;
    runtime.pointers = {{0x1000u, 11u}, {0x2000u, 22u},
                        {0x3000u, 33u}};
    runtime.supported = 12u;
    runtime.sample_successes = 2u;
    runtime.sample3d_successes = 1u;
    Check(pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              manager.header_flags[0] == 1u &&
              manager.driver == 0x1100u &&
              manager.provider == 0x2200u &&
              manager.listener == 0x3300u &&
              manager.streams.begin == 0x5000u &&
              manager.streams.end == 0x5004u &&
              manager.streams.capacity == 0x500cu &&
              manager.samples3d.begin == 0x6000u &&
              manager.samples3d.capacity == 0x6028u &&
              manager.effects.begin == 0x7000u &&
              manager.effects.capacity == 0x7010u &&
              runtime.pointers.at(0x5000u) == 11u &&
              runtime.pointers.at(0x6000u) == 22u &&
              runtime.pointers.at(0x7000u) == 33u &&
              manager.sample_capacity == 3u &&
              manager.max_3d_samples == 2u &&
              manager.samples2d.end - manager.samples2d.begin == 8u &&
              manager.spare.end - manager.spare.begin == 4u,
          "successful startup reserves three arrays and adjusts failed pools");
    Check(runtime.position_seen.x == 0x3f800000u &&
              runtime.position_seen.y == 0u &&
              runtime.position_seen.z == 0xc0000000u &&
              runtime.front_seen.x == 0x3f800000u &&
              runtime.front_seen.z == 0x3f800000u &&
              runtime.up_seen.x == 0u &&
              runtime.up_seen.y == 0x3f800000u &&
              runtime.up_seen.z == 0x80000000u,
          "listener uses native position sign flip and orientation constants");
    Check(EventIndex(runtime.events, "attribute:8704:Maximum supported samples") <
                  EventIndex(runtime.events, "listener:8704") &&
              EventIndex(runtime.events, "allocate:12:20480") <
                  EventIndex(runtime.events, "allocate:40:24576") &&
              EventIndex(runtime.events, "allocate:40:24576") <
                  EventIndex(runtime.events, "allocate:16:28672") &&
              EventIndex(runtime.events, "sample2d") <
                  EventIndex(runtime.events, "sample3d") &&
              EventIndex(runtime.events, "traverse-tree") <
                  EventIndex(runtime.events, "log:88") &&
              runtime.events.back() == "cleanup-config" &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "sample2d") == 3 &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "sample3d") == 2,
          "successful startup preserves provider, reserve and sample order");
}

void StartSystemWithExistingCapacity() {
    pal4::AudioManagerPrefixState manager{};
    manager.default_provider_count = 2u;
    manager.sample_capacity = 2u;
    manager.streams = {0x1000u, 0x1004u, 0x1010u};
    manager.samples3d = {0x2000u, 0x2000u, 0x2010u};
    manager.effects = {0x3000u, 0x3004u, 0x3010u};
    SystemRuntime runtime;
    runtime.manager = &manager;
    runtime.supported = 0u;
    Check(pal4::AudioSystem::Initialize(manager, nullptr, runtime) &&
              manager.streams.begin == 0x1000u &&
              manager.samples3d.begin == 0x2000u &&
              manager.effects.begin == 0x3000u &&
              manager.max_3d_samples == 0u &&
              manager.sample_capacity == 2u &&
              runtime.sample_calls == 2u &&
              runtime.sample3d_calls == 0u &&
              std::count(runtime.events.begin(), runtime.events.end(),
                         "sample2d") == 2 &&
              std::find_if(runtime.events.begin(), runtime.events.end(),
                           [](const std::string& event) {
                               return event.rfind("allocate:", 0u) == 0u ||
                                      event.rfind("free:", 0u) == 0u ||
                                      event.rfind("range-noop:", 0u) == 0u;
                           }) == runtime.events.end(),
          "existing capacity avoids reserves and zero provider limit skips 3D pool");
}

void ShutdownManager() {
    ShutdownRuntime runtime;
    pal4::AudioManagerPrefixState manager{};
    manager.header_flags[0] = 1u;
    manager.state = 7u;
    manager.streams = {0x1000u, 0x1004u, 0x1008u};
    manager.effects = {0x2000u, 0x2004u, 0x2008u};
    manager.samples3d = {0x3000u, 0x3004u, 0x3008u};
    manager.samples2d = {0x4000u, 0x4004u, 0x4008u};
    manager.spare = {0x5000u, 0x5004u, 0x5008u};
    Check(pal4::AudioManager::Deinitialize(manager, runtime) == 0x1001u &&
              manager.header_flags[0] == 0u && manager.state == 1u &&
              manager.streams.begin == 0u &&
              manager.streams.end == 0u &&
              manager.effects.capacity == 0u &&
              manager.samples3d.begin == 0u &&
              manager.samples2d.end == 0u &&
              manager.spare.capacity == 0u &&
              runtime.events == std::vector<std::string>{
                  "tail", "samples", "free:20480", "free:16384",
                  "free:12288", "free:8192", "free:4096"},
          "shutdown releases arrays from high to low offset and returns last free");
    runtime.events.clear();
    Check(pal4::AudioManager::Deinitialize(manager, runtime) == 1u &&
              runtime.events == std::vector<std::string>{
                  "tail", "samples", "free:0", "free:0",
                  "free:0", "free:0", "free:0"},
          "shutdown still forwards null buffers to each native free");
}

void CleanupManagerTree() {
    TreeRuntime runtime;
    pal4::AudioManagerPrefixState manager{};
    manager.tree_root = 0x6000u;
    manager.tree_count = 3u;
    runtime.words[{0x6000u, 0u}] = 0x7000u;
    runtime.words[{0x6000u, 4u}] = 0x7100u;
    pal4::AudioManagerTreeGlobalState tree{0x5000u, 2u};
    Check(pal4::AudioManager::CleanupTree(manager, tree, runtime) == 1u &&
              manager.tree_root == 0u && manager.tree_count == 0u &&
              tree.sentinel == 0x5000u && tree.users == 1u &&
              runtime.events == std::vector<std::string>{
                  "read:24576:0", "read:24576:4", "recursive:28928",
                  "write:24576:4:20480", "write:24576:0:24576",
                  "write:24576:8:24576", "first", "free:24576"},
          "nonempty tree recursively frees nodes and retains shared sentinel");

    runtime.events.clear();
    manager.tree_root = 0x8000u;
    manager.tree_count = 0u;
    runtime.words[{0x8000u, 0u}] = 0x8100u;
    runtime.successors = {{0x8100u, 0x8200u}, {0x8200u, 0x8000u}};
    Check(pal4::AudioManager::CleanupTree(manager, tree, runtime) == 0x55u &&
              manager.tree_root == 0u && tree.sentinel == 0u &&
              tree.users == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:32768:0", "next:33024", "remove:33024",
                  "next:33280", "remove:33280", "free:32768",
                  "free:20480"},
          "empty-count tree removes linked nodes and frees last sentinel");
}

void CleanupSampleTreeBranches() {
    TreeRuntime runtime;
    pal4::AudioManagerPrefixState manager{};
    manager.tree_root = 0x6000u;
    manager.tree_count = 2u;
    runtime.words[{0x6000u, 0u}] = 0x6100u;
    runtime.words[{0x6000u, 4u}] = 0x6200u;
    pal4::AudioManagerTreeGlobalState tree{0x5000u, 2u};
    Check(pal4::AudioManager::CleanupSampleTree(
              manager, tree, runtime) == 1u &&
              manager.tree_root == 0u && manager.tree_count == 0u &&
              tree.sentinel == 0x5000u && tree.users == 1u &&
              runtime.events == std::vector<std::string>{
                  "read:24576:0", "read:24576:4", "recursive:25088",
                  "write:24576:4:20480", "write:24576:0:24576",
                  "write:24576:8:24576", "free:24576"},
          "sample-tree cleanup skips the earlier tree's iterator callback");

    runtime.events.clear();
    manager.tree_root = 0x7000u;
    runtime.words[{0x7000u, 0u}] = 0x7100u;
    runtime.successors = {{0x7100u, 0x7200u}, {0x7200u, 0x7000u}};
    Check(pal4::AudioManager::CleanupSampleTree(
              manager, tree, runtime) == 0x55u &&
              manager.tree_root == 0u && tree.users == 0u &&
              tree.sentinel == 0u &&
              runtime.events == std::vector<std::string>{
                  "read:28672:0", "next:28928", "remove:28928",
                  "next:29184", "remove:29184", "free:28672",
                  "free:20480"},
          "sample-tree linked cleanup frees root and last shared sentinel");
}

void EraseTreeRangeBranches() {
    TreeRuntime runtime;
    pal4::AudioManagerPrefixState manager{};
    manager.tree_root = 0x6000u;
    manager.tree_count = 2u;
    const pal4::AudioManagerTreeGlobalState tree{0x5000u, 1u};
    runtime.words = {{{0x6000u, 0u}, 0x6100u},
                     {{0x6000u, 4u}, 0x6100u},
                     {{0x6100u, 0u}, 0x6200u},
                     {{0x6100u, 8u}, 0x6300u},
                     {{0x6200u, 0u}, 0x5000u},
                     {{0x6200u, 8u}, 0x6400u}};
    std::uint32_t output = 0u;
    Check(&pal4::AudioManager::EraseTreeRange(
              manager, tree, output, 0x6100u, 0x6000u, runtime) ==
                  &output && output == 0x6000u &&
              manager.tree_count == 0u &&
              runtime.words.at({0x6000u, 4u}) == 0x5000u &&
              runtime.words.at({0x6000u, 0u}) == 0x6000u &&
              runtime.words.at({0x6000u, 8u}) == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "read:24576:0", "read:24576:4",
                  "read:24832:8", "recursive:25344", "read:24832:0",
                  "free:24832", "read:25088:8", "recursive:25600",
                  "read:25088:0", "free:25088",
                  "write:24576:4:20480", "write:24576:0:24576",
                  "write:24576:8:24576", "read:24576:0"},
          "whole-tree range frees right subtrees and spine before root reset");

    runtime.events.clear();
    manager.tree_count = 3u;
    runtime.words[{0x6000u, 0u}] = 0x6100u;
    runtime.successors = {{0x6200u, 0x6300u}, {0x6300u, 0x6400u}};
    Check(&pal4::AudioManager::EraseTreeRange(
              manager, tree, output, 0x6200u, 0x6400u, runtime) ==
                  &output && output == 0x6400u &&
              manager.tree_count == 3u &&
              runtime.events == std::vector<std::string>{
                  "read:24576:0", "next:25088", "remove:25088",
                  "next:25344", "remove:25344"},
          "partial range computes successors before removal and retains count");

    runtime.events.clear();
    output = 0u;
    Check(&pal4::AudioManager::EraseTreeRange(
              manager, tree, output, 0x6200u, 0x6200u, runtime) ==
                  &output && output == 0x6200u &&
              runtime.events == std::vector<std::string>{"read:24576:0"},
          "empty range forwards its iterator without deleting nodes");

    runtime.events.clear();
    manager.tree_count = 0u;
    Check(&pal4::AudioManager::EraseTreeRange(
              manager, tree, output, 0x6200u, 0x6200u, runtime) ==
                  &output && output == 0x6200u &&
              runtime.events.empty(),
          "zero-count tree avoids reading root for an empty range");
}

using RebalanceNode = std::array<std::uint32_t, 5>;

void RemoveTreeNodeCase(const std::vector<RebalanceNode>& nodes,
                        const std::uint32_t root,
                        const std::uint32_t removed,
                        const std::uint32_t successor,
                        const std::vector<std::uint32_t>& expected,
                        const char* message) {
    constexpr std::uint32_t header = 0x6000u;
    constexpr std::uint32_t nil = 0x5000u;
    TreeRuntime runtime;
    runtime.words[std::make_pair(nil, 0u)] = nil;
    runtime.words[std::make_pair(nil, 4u)] = header;
    runtime.words[std::make_pair(nil, 8u)] = nil;
    runtime.words[std::make_pair(nil, 16u)] = 1u;
    runtime.words[std::make_pair(header, 0u)] = nodes.front()[0];
    runtime.words[std::make_pair(header, 4u)] = root;
    runtime.words[std::make_pair(header, 8u)] = nodes.back()[0];
    for (const auto& entry : nodes) {
        runtime.words[std::make_pair(entry[0], 0u)] = entry[1];
        runtime.words[std::make_pair(entry[0], 4u)] = entry[2];
        runtime.words[std::make_pair(entry[0], 8u)] = entry[3];
        runtime.words[std::make_pair(entry[0], 16u)] = entry[4];
    }
    runtime.successors[removed] = successor;
    pal4::AudioManagerPrefixState manager{};
    manager.tree_root = header;
    manager.tree_count = static_cast<std::uint32_t>(nodes.size());
    const pal4::AudioManagerTreeGlobalState tree{nil, 1u};
    std::uint32_t output = 0u;
    Check(&pal4::AudioManager::RemoveTreeNode(
              manager, tree, output, removed, runtime) == &output &&
              output == successor &&
              manager.tree_count == expected.size() &&
              runtime.events.front() == "next:" + std::to_string(removed) &&
              EventIndex(runtime.events,
                         "free:" + std::to_string(removed)) <
                  runtime.events.size(),
          message);

    const auto word = [&](std::uint32_t node, std::uint32_t offset) {
        return runtime.words.at(std::make_pair(node, offset));
    };
    const auto new_root = word(header, 4u);
    std::vector<std::uint32_t> visited;
    std::function<std::uint32_t(std::uint32_t, std::uint32_t)> check_node =
        [&](std::uint32_t node, std::uint32_t parent) -> std::uint32_t {
        if (node == nil)
            return 1u;
        Check(word(node, 4u) == parent &&
                  std::find(visited.begin(), visited.end(), node) ==
                      visited.end() && visited.size() < nodes.size(),
              message);
        if (word(node, 16u) == 0u)
            Check(word(word(node, 0u), 16u) == 1u &&
                      word(word(node, 8u), 16u) == 1u,
                  message);
        const auto left_height = check_node(word(node, 0u), node);
        visited.push_back(node);
        const auto right_height = check_node(word(node, 8u), node);
        Check(left_height == right_height, message);
        return left_height + (word(node, 16u) == 1u ? 1u : 0u);
    };
    (void)check_node(new_root, header);
    Check(visited == expected &&
              word(header, 0u) ==
                  (expected.empty() ? header : expected.front()) &&
              word(header, 8u) ==
                  (expected.empty() ? header : expected.back()) &&
              (new_root == nil || word(new_root, 16u) == 1u),
          message);
}

void RemoveTreeNodeBranches() {
    constexpr std::uint32_t nil = 0x5000u;
    constexpr std::uint32_t header = 0x6000u;
    RemoveTreeNodeCase({{{0x6200u, nil, header, nil, 1u}}},
                       0x6200u, 0x6200u, header, {},
                       "single black root deletion empties the tree");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 0u},
                         {0x6200u, 0x6100u, header, nil, 1u}}},
                       0x6200u, 0x6100u, 0x6200u, {0x6200u},
                       "red leaf deletion preserves black root");
    RemoveTreeNodeCase({{{0x6200u, nil, header, 0x6300u, 1u},
                         {0x6300u, nil, 0x6200u, nil, 0u}}},
                       0x6200u, 0x6200u, 0x6300u, {0x6300u},
                       "black root with one red child promotes the child");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6300u, 1u},
                         {0x6300u, nil, 0x6200u, nil, 1u}}},
                       0x6200u, 0x6100u, 0x6200u,
                       {0x6200u, 0x6300u},
                       "black leaf deletion recolors a black sibling");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6400u, 1u},
                         {0x6400u, nil, 0x6200u, 0x6500u, 1u},
                         {0x6500u, nil, 0x6400u, nil, 0u}}},
                       0x6200u, 0x6100u, 0x6200u,
                       {0x6200u, 0x6400u, 0x6500u},
                       "left black leaf triggers far-red right rotation case");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6400u, 1u},
                         {0x6300u, nil, 0x6400u, nil, 0u},
                         {0x6400u, 0x6300u, 0x6200u, nil, 1u}}},
                       0x6200u, 0x6100u, 0x6200u,
                       {0x6200u, 0x6300u, 0x6400u},
                       "near-red right sibling rotates twice after left deletion");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 0u},
                         {0x6200u, 0x6100u, 0x6400u, nil, 1u},
                         {0x6400u, 0x6200u, header, 0x6500u, 1u},
                         {0x6500u, nil, 0x6400u, nil, 1u}}},
                       0x6400u, 0x6500u, header,
                       {0x6100u, 0x6200u, 0x6400u},
                       "right black leaf triggers mirrored far-red case");
    RemoveTreeNodeCase({{{0x6200u, nil, 0x6400u, 0x6300u, 1u},
                         {0x6300u, nil, 0x6200u, nil, 0u},
                         {0x6400u, 0x6200u, header, 0x6500u, 1u},
                         {0x6500u, nil, 0x6400u, nil, 1u}}},
                       0x6400u, 0x6500u, header,
                       {0x6200u, 0x6300u, 0x6400u},
                       "near-red left sibling rotates twice after right deletion");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6400u, 1u},
                         {0x6300u, nil, 0x6400u, nil, 1u},
                         {0x6400u, 0x6300u, 0x6200u, 0x6500u, 0u},
                         {0x6500u, nil, 0x6400u, nil, 1u}}},
                       0x6200u, 0x6100u, 0x6200u,
                       {0x6200u, 0x6300u, 0x6400u, 0x6500u},
                       "red sibling rotation preserves black height");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6400u, 1u},
                         {0x6300u, nil, 0x6400u, nil, 0u},
                         {0x6400u, 0x6300u, 0x6200u, 0x6500u, 1u},
                         {0x6500u, nil, 0x6400u, nil, 0u}}},
                       0x6200u, 0x6200u, 0x6300u,
                       {0x6100u, 0x6300u, 0x6400u, 0x6500u},
                       "two-child deletion moves non-direct red successor");
    RemoveTreeNodeCase({{{0x6100u, nil, 0x6200u, nil, 1u},
                         {0x6200u, 0x6100u, header, 0x6300u, 1u},
                         {0x6300u, nil, 0x6200u, nil, 1u}}},
                       0x6200u, 0x6200u, 0x6300u,
                       {0x6100u, 0x6300u},
                       "two-child deletion repairs black direct successor");
}

void FreeTreeNodesBranches() {
    TreeRuntime runtime;
    const pal4::AudioManagerTreeGlobalState tree{0x5000u, 1u};
    Check(pal4::AudioManager::FreeTreeNodes(
              tree.sentinel, tree, runtime) == tree.sentinel &&
              runtime.events.empty(),
          "sentinel subtree returns without reading or freeing nodes");

    runtime.words = {{{0x6100u, 0u}, 0x6200u},
                     {{0x6100u, 8u}, 0x6300u},
                     {{0x6200u, 0u}, 0x5000u},
                     {{0x6200u, 8u}, 0x6400u},
                     {{0x6300u, 0u}, 0x5000u},
                     {{0x6300u, 8u}, 0x5000u},
                     {{0x6400u, 0u}, 0x5000u},
                     {{0x6400u, 8u}, 0x5000u}};
    Check(pal4::AudioManager::FreeTreeNodes(
              0x6100u, tree, runtime) == tree.sentinel &&
              runtime.events == std::vector<std::string>{
                  "read:24832:8", "read:25344:8", "read:25344:0",
                  "free:25344", "read:24832:0", "free:24832",
                  "read:25088:8", "read:25600:8", "read:25600:0",
                  "free:25600", "read:25088:0", "free:25088"},
          "recursive right subtrees free before left-spine nodes");
}

void TreeIteratorBranches() {
    TreeRuntime runtime;
    const pal4::AudioManagerTreeGlobalState tree{0x5000u, 1u};
    runtime.words = {{{0x6000u, 4u}, 0x6200u},
                     {{0x6000u, 8u}, 0x6400u},
                     {{0x6100u, 0u}, 0x5000u},
                     {{0x6100u, 4u}, 0x6200u},
                     {{0x6100u, 8u}, 0x5000u},
                     {{0x6200u, 0u}, 0x6100u},
                     {{0x6200u, 4u}, 0x6000u},
                     {{0x6200u, 8u}, 0x6400u},
                     {{0x6300u, 0u}, 0x5000u},
                     {{0x6300u, 4u}, 0x6400u},
                     {{0x6300u, 8u}, 0x5000u},
                     {{0x6400u, 0u}, 0x6300u},
                     {{0x6400u, 4u}, 0x6200u},
                     {{0x6400u, 8u}, 0x5000u}};
    Check(pal4::AudioManager::LastTreeNode(
              0x6400u, tree, runtime) == 0x6300u &&
              runtime.events == std::vector<std::string>{
                  "read:25600:0", "read:25344:0"},
          "last-node helper follows left children until sentinel");

    runtime.events.clear();
    std::uint32_t cursor = 0x6200u;
    Check(pal4::AudioManager::NextTreeNode(
              cursor, tree, runtime) == 0x5000u &&
              cursor == 0x6300u &&
              runtime.events == std::vector<std::string>{
                  "read:25088:8", "read:25600:0", "read:25344:0"},
          "successor descends to leftmost node of the right subtree");

    runtime.events.clear();
    cursor = 0x6100u;
    Check(pal4::AudioManager::NextTreeNode(
              cursor, tree, runtime) == 0x6200u &&
              cursor == 0x6200u &&
              runtime.events == std::vector<std::string>{
                  "read:24832:8", "read:24832:4", "read:25088:8",
                  "read:24832:8"},
          "successor of a left child is its parent");

    runtime.events.clear();
    cursor = 0x6400u;
    Check(pal4::AudioManager::NextTreeNode(
              cursor, tree, runtime) == 0x6000u &&
              cursor == 0x6000u &&
              runtime.events == std::vector<std::string>{
                  "read:25600:8", "read:25600:4", "read:25088:8",
                  "read:25088:4", "read:24576:8", "read:25088:8"},
          "rightmost successor climbs to the header iterator");
}

void InitializeManagerPrefix() {
    InitRuntime runtime;
    pal4::AudioManagerPrefixState manager{};
    manager.header_flags[1] = 0x54u;
    manager.unknown32[0] = 0x61u;
    manager.unknown124[1] = 0x73u;
    manager.tree_aux = 0x12345678u;
    manager.streams = {1u, 2u, 3u};
    manager.effects = {4u, 5u, 6u};
    manager.samples3d = {7u, 8u, 9u};
    manager.samples2d = {10u, 11u, 12u};
    manager.spare = {13u, 14u, 15u};
    pal4::AudioManagerTreeGlobalState tree{};
    Check(&pal4::AudioManager::Initialize(manager, tree, runtime) ==
                  &manager &&
              manager.volume_bits[0] == 0x3f800000u &&
              manager.volume_bits[1] == 0x3f800000u &&
              manager.volume_bits[2] == 0x3f800000u &&
              manager.header_flags[0] == 0u &&
              manager.header_flags[1] == 0x54u &&
              manager.unknown32[0] == 0x61u &&
              manager.unknown124[1] == 0x73u &&
              manager.state == 1u && manager.driver == 0u &&
              manager.provider == 0u && manager.listener == 0u &&
              manager.streams.begin == 0u &&
              manager.effects.end == 0u &&
              manager.samples3d.capacity == 0u &&
              manager.samples2d.begin == 0u &&
              manager.spare.end == 0u &&
              manager.max_3d_samples == 0u &&
              manager.sample_capacity == 8u &&
              manager.default_provider_count == 3u &&
              manager.tree_aux == 0x12345600u &&
              manager.tree_root == 0x6000u &&
              manager.tree_count == 0u &&
              tree.sentinel == 0x5000u && tree.users == 1u,
          "initialization writes observed prefix and retains unknown bytes");
    Check(runtime.events == std::vector<std::string>{
              "allocate:20", "write:20480:4:0", "write:20480:16:1",
              "write:20480:0:0", "write:20480:8:0",
              "allocate:20", "write:24576:4:20480",
              "write:24576:16:0", "write:24576:0:24576",
              "write:24576:8:24576", "tail"},
          "first manager retains candidate as global sentinel before root setup");

    runtime.events.clear();
    pal4::AudioManagerPrefixState second{};
    Check(&pal4::AudioManager::Initialize(second, tree, runtime) == &second &&
              tree.sentinel == 0x5000u && tree.users == 2u &&
              second.tree_root == 0x8000u &&
              runtime.events == std::vector<std::string>{
                  "allocate:20", "write:28672:4:0",
                  "write:28672:16:1", "free:28672",
                  "allocate:20", "write:32768:4:20480",
                  "write:32768:16:0", "write:32768:0:32768",
                  "write:32768:8:32768", "tail"},
          "later manager frees unused candidate and reuses global sentinel");
}

void DefaultConfig() {
    pal4::AudioDriverConfigState config{};
    config.sample_rate = 17u;
    config.provider = {1u, 2u, 3u};
    ConfigRuntime runtime;
    runtime.outer = &config;
    Check(&pal4::AudioDriverConfig::Construct(config, runtime) == &config &&
              config.sample_rate == 44100u &&
              config.bits_per_sample == 16u &&
              config.channel_count == 2u &&
              config.provider.length == 13u &&
              runtime.events == std::vector<std::string>{
                  "empty", "Miles Fast 2D"},
          "default driver config initializes string then exact audio defaults");
}

void SingletonAndObjectLifecycle() {
    SingletonRuntime singleton;
    pal4::AudioManagerSingletonState state{0x80u, 0u};
    Check(pal4::AudioManager::GetInstance(state, singleton) == 0x8000u &&
              state.guard == 0x81u && state.instance == 0x8000u &&
              singleton.events == std::vector<std::string>{
                  "registry", "auAudioMgr:1006"} &&
              singleton.factory == &pal4::AudioManager::Factory &&
              singleton.deleter == &pal4::AudioManager::Destroy,
          "first lookup registers named singleton with executable callbacks");
    singleton.events.clear();
    singleton.register_result = 0x9000u;
    Check(pal4::AudioManager::GetInstance(state, singleton) == 0x8000u &&
              singleton.events.empty(),
          "guarded lookup reuses cached registration result");
    state = {0u, 0u};
    singleton.register_result = 0u;
    Check(pal4::AudioManager::GetInstance(state, singleton) == 0u &&
              state.guard == 1u &&
              pal4::AudioManager::GetInstance(state, singleton) == 0u &&
              singleton.events == std::vector<std::string>{
                  "registry", "auAudioMgr:1006"},
          "failed registration still latches the once guard");

    ObjectRuntime object;
    Check(singleton.factory(object) == 0x7000u &&
              object.events == std::vector<std::string>{
                  "allocate:1049160", "initialize:24576"},
          "factory allocates native object size then initializes it");
    object.events.clear();
    object.allocation = 0u;
    Check(singleton.factory(object) == 0u &&
              object.events == std::vector<std::string>{
                  "allocate:1049160"},
          "failed allocation does not initialize the manager");
    object.events.clear();
    Check(singleton.deleter(0u, object) == 0u && object.events.empty(),
          "null destructor has no native side effects");
    Check(singleton.deleter(0x6000u, object) == 0x33u &&
              object.events == std::vector<std::string>{
                  "destruct:24576", "free:24576"},
          "non-null destructor cleans before freeing and forwards free result");
}

}  // namespace

int main() {
    CopyAudioPointerElements();
    SetAudioInitialFlag();
    DefaultConfig();
    SingletonAndObjectLifecycle();
    InitializeManagerPrefix();
    CleanupManagerTree();
    CleanupSampleTreeBranches();
    EraseTreeRangeBranches();
    RemoveTreeNodeBranches();
    FreeTreeNodesBranches();
    TreeIteratorBranches();
    ShutdownManager();
    StartSystemFailures();
    StartSystemSuccess();
    StartSystemWithExistingCapacity();
    ShutdownSystem();
    ToggleAudioVolumes();
    PlaySoundBranches();
    PlayMusicBranches();
    Play3DSoundBranches();
    Find3DProviderBranches();
    CleanupSoundBranches();
    FindSoundObjectBranches();
    UpdateAllSoundsBranches();
    RemoveSampleBranches();
    AudioHandleBranches();
    AddSampleBranches();
    std::cout << "Audio manager tests passed\n";
}
