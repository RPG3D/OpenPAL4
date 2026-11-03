#pragma once

#include "pal4/string_buffer.h"
#include "pal4/crt_atexit_stub.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct AudioDriverConfigState final {
    std::uint32_t sample_rate = 0u;
    std::uint32_t bits_per_sample = 0u;
    std::uint32_t channel_count = 0u;
    StringBufferState provider;
};
static_assert(sizeof(AudioDriverConfigState) == 24u);
static_assert(offsetof(AudioDriverConfigState, provider) == 12u);

class IAudioDriverConfigRuntime {
public:
    virtual ~IAudioDriverConfigRuntime() = default;
    virtual void InitializeEmpty(StringBufferState& text) = 0;
    virtual void AssignText(StringBufferState& text, const char* source,
                            std::uint32_t length) = 0;
};

class AudioDriverConfig final {
public:
    static constexpr char kDefaultProvider[] = "Miles Fast 2D";
    // IDA 0x657ff0: default Miles driver settings.
    [[nodiscard]] static AudioDriverConfigState& Construct(
        AudioDriverConfigState& config, IAudioDriverConfigRuntime& runtime);
};

class IAudioManagerObjectRuntime {
public:
    virtual ~IAudioManagerObjectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Initialize(std::uint32_t address) = 0;
    virtual void Destruct(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class AudioManager;
using AudioManagerFactory = std::uint32_t (*)(IAudioManagerObjectRuntime&);
using AudioManagerDeleter = std::uint32_t (*)(
    std::uint32_t, IAudioManagerObjectRuntime&);

class IAudioManagerSingletonRuntime {
public:
    virtual ~IAudioManagerSingletonRuntime() = default;
    virtual void GetRegistryInstance() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        const char* name, std::uint32_t identifier,
        AudioManagerFactory factory, AudioManagerDeleter deleter) = 0;
};

struct AudioManagerSingletonState final {
    std::uint8_t guard = 0u;
    std::uint32_t instance = 0u;
};

struct AudioPointerArrayState final {
    std::uint32_t begin = 0u;
    std::uint32_t end = 0u;
    std::uint32_t capacity = 0u;
};
static_assert(sizeof(AudioPointerArrayState) == 12u);

class IAudioPointerCopyRuntime {
public:
    virtual ~IAudioPointerCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
};

class AudioPointerArrayOps final {
public:
    // IDA 0x65a090: copy 32-bit elements forward and return destination end.
    [[nodiscard]] static std::uint32_t CopyElements(
        std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
        IAudioPointerCopyRuntime& runtime);
};

class AudioInitialFlag final {
public:
    // IDA 0x65a0f0: set the pointed-to 32-bit word to one.
    static void Set(std::uint32_t& flag);
};

struct AudioManagerPrefixState final {
    std::uint32_t volume_bits[3]{};
    std::uint8_t header_flags[4]{};
    std::uint32_t state = 0u;
    std::uint32_t driver = 0u;
    std::uint32_t provider = 0u;
    std::uint32_t listener = 0u;
    std::uint8_t unknown32[4]{};
    AudioPointerArrayState streams;
    std::uint8_t unknown48[4]{};
    AudioPointerArrayState effects;
    std::uint8_t unknown64[4]{};
    AudioPointerArrayState samples3d;
    std::uint8_t unknown80[4]{};
    AudioPointerArrayState samples2d;
    std::uint8_t unknown96[4]{};
    AudioPointerArrayState spare;
    std::uint32_t max_3d_samples = 0u;
    std::uint32_t sample_capacity = 0u;
    std::uint32_t default_provider_count = 0u;
    std::uint8_t unknown124[4]{};
    std::uint32_t tree_root = 0u;
    std::uint32_t tree_aux = 0u;
    std::uint32_t tree_count = 0u;
    std::uint32_t tail_word = 0u;
};
static_assert(sizeof(AudioManagerPrefixState) == 144u);
static_assert(offsetof(AudioManagerPrefixState, streams) == 36u);
static_assert(offsetof(AudioManagerPrefixState, spare) == 100u);
static_assert(offsetof(AudioManagerPrefixState, tree_root) == 128u);

struct AudioManagerTreeGlobalState final {
    std::uint32_t sentinel = 0u;
    std::uint32_t users = 0u;
};

class IAudioManagerInitRuntime {
public:
    virtual ~IAudioManagerInitRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                               std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void InitializeTail(AudioManagerPrefixState& manager) = 0;
};

class IAudioManagerTreeRuntime {
public:
    virtual ~IAudioManagerTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadNodeWord(
        std::uint32_t node, std::uint32_t offset) = 0;
    virtual void WriteNodeWord(std::uint32_t node, std::uint32_t offset,
                               std::uint32_t value) = 0;
    virtual void FreeNodesRecursive(std::uint32_t root) = 0;
    virtual void FirstElement(AudioManagerPrefixState& manager) = 0;
    [[nodiscard]] virtual std::uint32_t NextNode(std::uint32_t node) = 0;
    virtual void RemoveNode(AudioManagerPrefixState& manager,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class IAudioManagerShutdownRuntime {
public:
    virtual ~IAudioManagerShutdownRuntime() = default;
    virtual void CleanupTail(AudioManagerPrefixState& manager) = 0;
    virtual void CleanupSamples(AudioManagerPrefixState& manager) = 0;
    [[nodiscard]] virtual std::uint32_t FreeBuffer(
        std::uint32_t address) = 0;
};

struct AudioListenerPositionBits final {
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t z = 0u;
};

class IAudioSystemRuntime : public IAudioDriverConfigRuntime {
public:
    virtual void Log(std::uint32_t message_id) = 0;
    virtual void SetRedistDirectory(const char* path) = 0;
    virtual void PrepareMiles() = 0;
    [[nodiscard]] virtual bool StartupMiles() = 0;
    [[nodiscard]] virtual std::uint32_t OpenDigitalDriver(
        std::uint32_t sample_rate, std::uint32_t bits_per_sample,
        std::uint32_t channel_count, std::uint32_t flags) = 0;
    virtual void ReadLastError() = 0;
    virtual void SetMasterVolume(std::uint32_t driver,
                                 std::uint32_t volume_bits) = 0;
    [[nodiscard]] virtual std::uint32_t Find3DProvider(
        std::uint32_t provider_name) = 0;
    [[nodiscard]] virtual std::int32_t Open3DProvider(
        std::uint32_t provider) = 0;
    [[nodiscard]] virtual std::uint32_t ProviderAttribute(
        std::uint32_t provider, const char* name) = 0;
    [[nodiscard]] virtual std::uint32_t Open3DListener(
        std::uint32_t provider) = 0;
    [[nodiscard]] virtual AudioListenerPositionBits ListenerPosition() = 0;
    virtual void Set3DPosition(std::uint32_t listener,
                               AudioListenerPositionBits position) = 0;
    virtual void Set3DOrientation(
        std::uint32_t listener, AudioListenerPositionBits front,
        AudioListenerPositionBits up) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void CopyPointer(std::uint32_t destination,
                             std::uint32_t source) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateSample(
        std::uint32_t driver) = 0;
    virtual void InitializeSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate3DSample(
        std::uint32_t provider) = 0;
    virtual void InsertHandle(AudioPointerArrayState& array,
                              std::uint32_t handle) = 0;
    virtual void TraverseTree(AudioManagerPrefixState& manager) = 0;
    virtual void CleanupConfig(AudioDriverConfigState& config) = 0;
};

class IAudioSystemShutdownRuntime : public IAudioManagerTreeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t value) = 0;
    virtual void StopObject(std::uint32_t object) = 0;
    virtual void DeleteObject(std::uint32_t object,
                              std::uint32_t flags) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    virtual void ReleaseSample(std::uint32_t sample) = 0;
    virtual void Release3DSample(std::uint32_t sample) = 0;
    [[nodiscard]] virtual bool PackageOpen(
        const AudioManagerPrefixState& manager) = 0;
    virtual void ClosePackage(AudioManagerPrefixState& manager) = 0;
    virtual void ClearSoundPool() = 0;
    virtual void Close3DListener(std::uint32_t listener) = 0;
    virtual void Close3DProvider(std::uint32_t provider) = 0;
    virtual void CloseDigitalDriver(std::uint32_t driver) = 0;
    virtual void ShutdownMiles() = 0;
};

struct AudioVolumeGlobalsState final {
    std::uint32_t saved_bits[3]{};
    std::uint8_t guard = 0u;
};

class IAudioSystemVolumeRuntime {
public:
    virtual ~IAudioSystemVolumeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SetStreamVolume(
        std::uint32_t stream, std::uint32_t volume_bits) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectKind(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t RefreshObject(
        std::uint32_t object) = 0;
};

class IAudioPlaybackRuntime {
public:
    virtual ~IAudioPlaybackRuntime() = default;
    virtual void CleanupSounds(AudioPointerArrayState& array,
                               std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructSoundObject(
        std::uint32_t memory, std::uint32_t identifier,
        std::int32_t flags) = 0;
    [[nodiscard]] virtual std::int32_t PlaySoundObject(
        std::uint32_t object, std::uint32_t source,
        AudioManagerPrefixState& manager, std::int32_t mode) = 0;
    virtual void InsertObject(AudioPointerArrayState& array,
                              std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectId(
        std::uint32_t object) = 0;
    virtual void Log(std::uint32_t message_id) = 0;
    virtual void DestroyObject(std::uint32_t object,
                               std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructMusicObject(
        std::uint32_t memory, std::uint32_t identifier) = 0;
    [[nodiscard]] virtual std::int32_t PlayMusicObject(
        std::uint32_t object, std::uint32_t sample,
        std::uint32_t source, AudioManagerPrefixState& manager,
        std::int32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t Construct3DObject(
        std::uint32_t memory, std::uint32_t identifier) = 0;
    virtual void Set3DSourcePosition(
        std::uint32_t object, AudioListenerPositionBits position) = 0;
    [[nodiscard]] virtual std::int32_t Play3DObject(
        std::uint32_t object, std::uint32_t sample,
        std::uint32_t source, AudioManagerPrefixState& manager,
        std::int32_t first_mode, std::int32_t second_mode,
        std::int32_t third_mode) = 0;
};

class IAudioProviderSearchRuntime {
public:
    virtual ~IAudioProviderSearchRuntime() = default;
    [[nodiscard]] virtual bool Enumerate(std::uint32_t& cursor,
                                         std::uint32_t& provider,
                                         const char*& name) = 0;
    [[nodiscard]] virtual std::int32_t Open(std::uint32_t provider) = 0;
    virtual void Close(std::uint32_t provider) = 0;
    [[nodiscard]] virtual bool HasLastError() = 0;
    virtual void ReadLastError() = 0;
    virtual void Log(std::uint32_t message_id) = 0;
};

struct AudioObjectNameView final {
    const char* data = nullptr;
    std::uint32_t length = 0u;
};

class IAudioCleanupRuntime {
public:
    virtual ~IAudioCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t object) = 0;
    [[nodiscard]] virtual bool IsActive(std::uint32_t object) = 0;
    [[nodiscard]] virtual AudioObjectNameView Name(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int64_t Priority(
        std::uint32_t object) = 0;
    virtual void Destroy(std::uint32_t object, std::uint32_t flags) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
};

class IAudioObjectLookupRuntime {
public:
    virtual ~IAudioObjectLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectId(
        std::uint32_t object) = 0;
};

class IAudioStreamUpdateRuntime {
public:
    virtual ~IAudioStreamUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Update(
        std::uint32_t stream, std::uint32_t time_bits) = 0;
};

class IAudioObjectRemovalRuntime {
public:
    virtual ~IAudioObjectRemovalRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Kind(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t object) = 0;
    virtual void Destroy(std::uint32_t object, std::uint32_t flags) = 0;
};

class IAudioSamplePoolRuntime {
public:
    virtual ~IAudioSamplePoolRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ReadPointer(
        std::uint32_t address) = 0;
    virtual void WritePointer(std::uint32_t address,
                              std::uint32_t value) = 0;
    virtual void RangeNoOp(std::uint32_t begin, std::uint32_t end) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class AudioManager final {
public:
    static constexpr std::uint32_t kAllocationSize = 0x100248u;
    // IDA 0x658070: lazily register auAudioMgr under singleton id 1006.
    [[nodiscard]] static std::uint32_t GetInstance(
        AudioManagerSingletonState& state,
        IAudioManagerSingletonRuntime& runtime);
    // IDA 0x6580b0: allocate and initialize the manager object.
    [[nodiscard]] static std::uint32_t Factory(
        IAudioManagerObjectRuntime& runtime);
    // IDA 0x658110: destruct and free a non-null manager object.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t address, IAudioManagerObjectRuntime& runtime);
    // IDA 0x658130: initialize the observed manager prefix and shared tree.
    [[nodiscard]] static AudioManagerPrefixState& Initialize(
        AudioManagerPrefixState& manager,
        AudioManagerTreeGlobalState& tree,
        IAudioManagerInitRuntime& runtime);
    // IDA 0x658290: release per-manager tree and shared sentinel ownership.
    [[nodiscard]] static std::uint32_t CleanupTree(
        AudioManagerPrefixState& manager,
        AudioManagerTreeGlobalState& tree,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x659a80: release the same tree without iterator reset callback.
    [[nodiscard]] static std::uint32_t CleanupSampleTree(
        AudioManagerPrefixState& manager,
        AudioManagerTreeGlobalState& tree,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x659b30: erase a tree iterator range, including whole-tree fast path.
    [[nodiscard]] static std::uint32_t& EraseTreeRange(
        AudioManagerPrefixState& manager,
        const AudioManagerTreeGlobalState& tree, std::uint32_t& output,
        std::uint32_t first, std::uint32_t last,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x659c00: unlink one red-black tree node and restore coloring.
    [[nodiscard]] static std::uint32_t& RemoveTreeNode(
        AudioManagerPrefixState& manager,
        const AudioManagerTreeGlobalState& tree, std::uint32_t& output,
        std::uint32_t node, IAudioManagerTreeRuntime& runtime);
    // IDA 0x659fe0: free right subtrees recursively and walk the left spine.
    [[nodiscard]] static std::uint32_t FreeTreeNodes(
        std::uint32_t node, const AudioManagerTreeGlobalState& tree,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x65a020: descend through left children to the final node.
    [[nodiscard]] static std::uint32_t LastTreeNode(
        std::uint32_t node, const AudioManagerTreeGlobalState& tree,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x65a040: advance a tree iterator and return the final probe node.
    [[nodiscard]] static std::uint32_t NextTreeNode(
        std::uint32_t& cursor, const AudioManagerTreeGlobalState& tree,
        IAudioManagerTreeRuntime& runtime);
    // IDA 0x658350: release owned buffers and restore stopped state.
    [[nodiscard]] static std::uint32_t Deinitialize(
        AudioManagerPrefixState& manager,
        IAudioManagerShutdownRuntime& runtime);
    // IDA 0x659620: remove a sound object from its class-specific array.
    [[nodiscard]] static std::uint32_t RemoveSampleByKind(
        AudioManagerPrefixState& manager, std::uint32_t object,
        IAudioObjectRemovalRuntime& runtime);
    // IDA 0x659720: append a 2D sample handle with native capacity growth.
    [[nodiscard]] static std::uint32_t AddSample(
        AudioManagerPrefixState& manager, std::uint32_t sample,
        IAudioSamplePoolRuntime& runtime);
    // IDA 0x6598d0: append a 3D sample handle to the spare pool.
    [[nodiscard]] static std::uint32_t Add3DSample(
        AudioManagerPrefixState& manager, std::uint32_t sample,
        IAudioSamplePoolRuntime& runtime);
};

class AudioSystem final {
public:
    // IDA 0x7DE290: register Miles shutdown at process exit; ignore CRT status.
    [[nodiscard]] static int RegisterMilesShutdown(
        CrtExitCallback callback, ICrtAtexitApi& atexit_api);
    // IDA 0x658410: start Miles and initialize driver, provider and sample pools.
    [[nodiscard]] static bool Initialize(
        AudioManagerPrefixState& manager,
        const AudioDriverConfigState* requested,
        IAudioSystemRuntime& runtime);
    // IDA 0x6588f0: close objects, handles and Miles, then reset tree nodes.
    [[nodiscard]] static std::uint32_t Shutdown(
        AudioManagerPrefixState& manager,
        AudioManagerTreeGlobalState& tree,
        IAudioSystemShutdownRuntime& runtime);
    // IDA 0x658b70: save, mute or restore three audio volume groups.
    [[nodiscard]] static std::uint8_t SetActive(
        AudioManagerPrefixState& manager, std::uint8_t enabled,
        AudioVolumeGlobalsState& globals,
        IAudioSystemVolumeRuntime& runtime);
    // IDA 0x658d50: allocate and play a 64-byte sound object.
    [[nodiscard]] static std::uint32_t& PlaySound(
        AudioManagerPrefixState& manager, std::uint32_t& output,
        std::uint32_t source, std::int32_t mode, std::int32_t flags,
        IAudioPlaybackRuntime& runtime);
    // IDA 0x658e90: consume a pooled sample handle for music playback.
    [[nodiscard]] static std::uint32_t& PlayMusic(
        AudioManagerPrefixState& manager, std::uint32_t& output,
        std::uint32_t source, std::int32_t mode,
        IAudioPlaybackRuntime& runtime);
    // IDA 0x659060: play a positioned sound from the 3D sample pool.
    [[nodiscard]] static std::uint32_t& Play3DSound(
        AudioManagerPrefixState& manager, std::uint32_t& output,
        std::uint32_t source, AudioListenerPositionBits position,
        std::int32_t first_mode, std::int32_t second_mode,
        std::int32_t third_mode, IAudioPlaybackRuntime& runtime);
    // IDA 0x659260: prefer a matching usable provider, then any usable one.
    [[nodiscard]] static std::uint32_t Find3DProvider(
        const char* requested_name, IAudioProviderSearchRuntime& runtime);
    // IDA 0x6593e0: remove an inactive object, otherwise a selected live one.
    [[nodiscard]] static std::uint32_t CleanupSounds(
        AudioPointerArrayState& array, const char* requested_name,
        IAudioCleanupRuntime& runtime);
    // IDA 0x659580: search streams, effects and 3D objects in that order.
    [[nodiscard]] static std::uint32_t FindSoundObject(
        const AudioManagerPrefixState& manager, std::uint32_t identifier,
        IAudioObjectLookupRuntime& runtime);
    // IDA 0x6595e0: update each non-null stream with the supplied float bits.
    [[nodiscard]] static std::uint32_t UpdateAllSounds(
        AudioManagerPrefixState& manager, std::uint32_t time_bits,
        IAudioStreamUpdateRuntime& runtime);
};

struct AudioHandleState final {
    std::uint32_t identifier = 0u;
};

class IAudioHandleRuntime : public IAudioObjectLookupRuntime,
                            public IAudioObjectRemovalRuntime {
public:
    [[nodiscard]] virtual AudioManagerPrefixState& GetManager() = 0;
    virtual void Start(std::uint32_t object) = 0;
    virtual void Stop(std::uint32_t object) = 0;
    virtual void Pause(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t IsPlaying(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FadeIn(
        std::uint32_t object, std::uint32_t time_bits) = 0;
    [[nodiscard]] virtual std::uint32_t FadeOut(
        std::uint32_t object, std::uint32_t time_bits) = 0;
    virtual void WriteProperty(std::uint32_t object,
                               std::uint32_t value) = 0;
    virtual void Set3DPosition(
        std::uint32_t object, AudioListenerPositionBits position) = 0;
};

class AudioHandle final {
public:
    // IDA 0x65a130..0x65a1f0: id-based audio object control.
    [[nodiscard]] static bool IsActive(const AudioHandleState& handle,
                                       IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t Play(const AudioHandleState& handle,
                                           IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t Stop(const AudioHandleState& handle,
                                           IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t Pause(const AudioHandleState& handle,
                                            IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t IsPlaying(
        const AudioHandleState& handle, IAudioHandleRuntime& runtime);
    // IDA 0x65a220..0x65a2d0: type-gated stream and 3D controls.
    [[nodiscard]] static std::uint32_t FadeIn(
        const AudioHandleState& handle, std::uint32_t time_bits,
        IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint32_t FadeOut(
        const AudioHandleState& handle, std::uint32_t time_bits,
        IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetProperty(
        const AudioHandleState& handle, std::uint32_t value,
        IAudioHandleRuntime& runtime);
    [[nodiscard]] static std::uint8_t Set3DPosition(
        const AudioHandleState& handle, AudioListenerPositionBits position,
        IAudioHandleRuntime& runtime);
};

}  // namespace pal4
