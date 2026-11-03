#include "pal4/audio_manager.h"
#include <cstring>

namespace pal4 {

std::uint32_t AudioPointerArrayOps::CopyElements(
    std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, IAudioPointerCopyRuntime& runtime) {
    while (begin != end) {
        const auto value = runtime.Read(begin);
        runtime.Write(destination, value);
        begin += 4u;
        destination += 4u;
    }
    return destination;
}

void AudioInitialFlag::Set(std::uint32_t& flag) {
    flag = 1u;
}

AudioDriverConfigState& AudioDriverConfig::Construct(
    AudioDriverConfigState& config, IAudioDriverConfigRuntime& runtime) {
    runtime.InitializeEmpty(config.provider);
    config.sample_rate = 44100u;
    config.bits_per_sample = 16u;
    config.channel_count = 2u;
    const auto* provider = kDefaultProvider;
    runtime.AssignText(config.provider, provider,
                       static_cast<std::uint32_t>(std::strlen(provider)));
    return config;
}

std::uint32_t AudioManager::GetInstance(
    AudioManagerSingletonState& state,
    IAudioManagerSingletonRuntime& runtime) {
    if ((state.guard & 1u) != 0u)
        return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    runtime.GetRegistryInstance();
    state.instance = runtime.Register(
        "auAudioMgr", 1006u, &AudioManager::Factory,
        &AudioManager::Destroy);
    return state.instance;
}

std::uint32_t AudioManager::Factory(IAudioManagerObjectRuntime& runtime) {
    const auto memory = runtime.Allocate(kAllocationSize);
    return memory == 0u ? 0u : runtime.Initialize(memory);
}

std::uint32_t AudioManager::Destroy(
    const std::uint32_t address, IAudioManagerObjectRuntime& runtime) {
    if (address == 0u)
        return 0u;
    runtime.Destruct(address);
    return runtime.Free(address);
}

AudioManagerPrefixState& AudioManager::Initialize(
    AudioManagerPrefixState& manager, AudioManagerTreeGlobalState& tree,
    IAudioManagerInitRuntime& runtime) {
    for (auto& bits : manager.volume_bits)
        bits = 0x3f800000u;
    manager.header_flags[0] = 0u;
    manager.state = 1u;
    manager.driver = manager.provider = manager.listener = 0u;
    manager.streams = {};
    manager.effects = {};
    manager.samples3d = {};
    manager.samples2d = {};
    manager.spare = {};
    manager.max_3d_samples = 0u;
    manager.sample_capacity = 8u;
    manager.default_provider_count = 3u;
    manager.tree_aux &= 0xffffff00u;

    const auto candidate = runtime.Allocate(20u);
    runtime.WriteNodeWord(candidate, 4u, 0u);
    runtime.WriteNodeWord(candidate, 16u, 1u);
    if (tree.sentinel == 0u) {
        tree.sentinel = candidate;
        runtime.WriteNodeWord(candidate, 0u, 0u);
        runtime.WriteNodeWord(candidate, 8u, 0u);
    } else {
        runtime.Free(candidate);
    }
    ++tree.users;

    const auto root = runtime.Allocate(20u);
    runtime.WriteNodeWord(root, 4u, tree.sentinel);
    runtime.WriteNodeWord(root, 16u, 0u);
    manager.tree_root = root;
    manager.tree_count = 0u;
    runtime.WriteNodeWord(root, 0u, root);
    runtime.WriteNodeWord(root, 8u, root);
    runtime.InitializeTail(manager);
    return manager;
}

namespace {
std::uint32_t CleanupAudioTree(
    AudioManagerPrefixState& manager, AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime, const bool reset_iterator) {
    const auto root = manager.tree_root;
    auto first = runtime.ReadNodeWord(root, 0u);
    if (manager.tree_count != 0u) {
        runtime.FreeNodesRecursive(runtime.ReadNodeWord(root, 4u));
        runtime.WriteNodeWord(root, 4u, tree.sentinel);
        manager.tree_count = 0u;
        runtime.WriteNodeWord(root, 0u, root);
        runtime.WriteNodeWord(root, 8u, root);
        if (reset_iterator)
            runtime.FirstElement(manager);
    } else {
        while (first != root) {
            const auto current = first;
            first = runtime.NextNode(current);
            runtime.RemoveNode(manager, current);
        }
    }
    (void)runtime.Free(root);
    manager.tree_root = 0u;
    manager.tree_count = 0u;
    --tree.users;
    if (tree.users != 0u)
        return tree.users;
    const auto sentinel = tree.sentinel;
    tree.sentinel = 0u;
    return sentinel == 0u ? 0u : runtime.Free(sentinel);
}
}  // namespace

std::uint32_t AudioManager::CleanupTree(
    AudioManagerPrefixState& manager, AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime) {
    return CleanupAudioTree(manager, tree, runtime, true);
}

std::uint32_t AudioManager::CleanupSampleTree(
    AudioManagerPrefixState& manager, AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime) {
    return CleanupAudioTree(manager, tree, runtime, false);
}

std::uint32_t& AudioManager::EraseTreeRange(
    AudioManagerPrefixState& manager,
    const AudioManagerTreeGlobalState& tree, std::uint32_t& output,
    std::uint32_t first, const std::uint32_t last,
    IAudioManagerTreeRuntime& runtime) {
    const auto root = manager.tree_root;
    if (manager.tree_count != 0u &&
        first == runtime.ReadNodeWord(root, 0u) && last == root) {
        auto node = runtime.ReadNodeWord(root, 4u);
        while (node != tree.sentinel) {
            runtime.FreeNodesRecursive(runtime.ReadNodeWord(node, 8u));
            const auto next = runtime.ReadNodeWord(node, 0u);
            (void)runtime.Free(node);
            node = next;
        }
        runtime.WriteNodeWord(root, 4u, tree.sentinel);
        manager.tree_count = 0u;
        runtime.WriteNodeWord(root, 0u, root);
        runtime.WriteNodeWord(root, 8u, root);
        output = runtime.ReadNodeWord(root, 0u);
        return output;
    }
    while (first != last) {
        const auto current = first;
        first = runtime.NextNode(current);
        runtime.RemoveNode(manager, current);
    }
    output = first;
    return output;
}

namespace {
class AudioTreeRebalancer final {
public:
    AudioTreeRebalancer(AudioManagerPrefixState& manager,
                        const AudioManagerTreeGlobalState& tree,
                        IAudioManagerTreeRuntime& runtime)
        : manager_(manager), nil_(tree.sentinel), runtime_(runtime) {}

    std::uint32_t Remove(const std::uint32_t node) {
        const auto successor = runtime_.NextNode(node);
        auto detached = node;
        std::uint32_t replacement;
        if (Left(detached) == nil_)
            replacement = Right(detached);
        else if (Right(detached) == nil_)
            replacement = Left(detached);
        else {
            detached = Right(detached);
            while (Left(detached) != nil_)
                detached = Left(detached);
            replacement = Right(detached);
        }

        std::uint32_t replacement_parent;
        if (detached != node) {
            SetParent(Left(node), detached);
            SetLeft(detached, Left(node));
            if (detached == Right(node)) {
                replacement_parent = detached;
                SetParent(replacement, detached);
            } else {
                replacement_parent = Parent(detached);
                SetParent(replacement, replacement_parent);
                SetLeft(replacement_parent, replacement);
                SetRight(detached, Right(node));
                SetParent(Right(node), detached);
            }
            ReplaceParentLink(node, detached);
            SetParent(detached, Parent(node));
            const auto old_color = Color(detached);
            SetColor(detached, Color(node));
            SetColor(node, old_color);
            detached = node;
        } else {
            replacement_parent = Parent(detached);
            SetParent(replacement, replacement_parent);
            ReplaceParentLink(detached, replacement);
            if (Minimum() == detached) {
                if (Right(detached) == nil_)
                    SetMinimum(Parent(detached));
                else
                    SetMinimum(Leftmost(replacement));
            }
            if (Maximum() == detached) {
                if (Left(detached) == nil_)
                    SetMaximum(Parent(detached));
                else
                    SetMaximum(Rightmost(replacement));
            }
        }

        if (Color(detached) == 1u)
            RestoreBlackHeight(replacement, replacement_parent);
        (void)runtime_.Free(detached);
        --manager_.tree_count;
        return successor;
    }

private:
    [[nodiscard]] std::uint32_t Read(const std::uint32_t node,
                                     const std::uint32_t offset) const {
        return runtime_.ReadNodeWord(node, offset);
    }
    void Write(const std::uint32_t node, const std::uint32_t offset,
               const std::uint32_t value) {
        runtime_.WriteNodeWord(node, offset, value);
    }
    [[nodiscard]] std::uint32_t Left(std::uint32_t node) const {
        return Read(node, 0u);
    }
    [[nodiscard]] std::uint32_t Parent(std::uint32_t node) const {
        return Read(node, 4u);
    }
    [[nodiscard]] std::uint32_t Right(std::uint32_t node) const {
        return Read(node, 8u);
    }
    [[nodiscard]] std::uint32_t Color(std::uint32_t node) const {
        return Read(node, 16u);
    }
    void SetLeft(std::uint32_t node, std::uint32_t value) {
        Write(node, 0u, value);
    }
    void SetParent(std::uint32_t node, std::uint32_t value) {
        Write(node, 4u, value);
    }
    void SetRight(std::uint32_t node, std::uint32_t value) {
        Write(node, 8u, value);
    }
    void SetColor(std::uint32_t node, std::uint32_t value) {
        Write(node, 16u, value);
    }
    [[nodiscard]] std::uint32_t Root() const {
        return Read(manager_.tree_root, 4u);
    }
    [[nodiscard]] std::uint32_t Minimum() const {
        return Read(manager_.tree_root, 0u);
    }
    [[nodiscard]] std::uint32_t Maximum() const {
        return Read(manager_.tree_root, 8u);
    }
    void SetRoot(std::uint32_t node) {
        Write(manager_.tree_root, 4u, node);
    }
    void SetMinimum(std::uint32_t node) {
        Write(manager_.tree_root, 0u, node);
    }
    void SetMaximum(std::uint32_t node) {
        Write(manager_.tree_root, 8u, node);
    }
    [[nodiscard]] std::uint32_t Leftmost(std::uint32_t node) const {
        while (Left(node) != nil_)
            node = Left(node);
        return node;
    }
    [[nodiscard]] std::uint32_t Rightmost(std::uint32_t node) const {
        while (Right(node) != nil_)
            node = Right(node);
        return node;
    }
    void ReplaceParentLink(std::uint32_t old_node,
                           std::uint32_t new_node) {
        if (Root() == old_node)
            SetRoot(new_node);
        else if (Left(Parent(old_node)) == old_node)
            SetLeft(Parent(old_node), new_node);
        else
            SetRight(Parent(old_node), new_node);
    }
    void RotateLeft(const std::uint32_t node) {
        const auto pivot = Right(node);
        SetRight(node, Left(pivot));
        if (Left(pivot) != nil_)
            SetParent(Left(pivot), node);
        SetParent(pivot, Parent(node));
        ReplaceParentLink(node, pivot);
        SetLeft(pivot, node);
        SetParent(node, pivot);
    }
    void RotateRight(const std::uint32_t node) {
        const auto pivot = Left(node);
        SetLeft(node, Right(pivot));
        if (Right(pivot) != nil_)
            SetParent(Right(pivot), node);
        SetParent(pivot, Parent(node));
        ReplaceParentLink(node, pivot);
        SetRight(pivot, node);
        SetParent(node, pivot);
    }
    void RestoreBlackHeight(std::uint32_t node, std::uint32_t parent) {
        while (node != Root() && Color(node) == 1u) {
            if (node == Left(parent)) {
                auto sibling = Right(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateLeft(parent);
                    sibling = Right(parent);
                }
                if (Color(Left(sibling)) == 1u &&
                    Color(Right(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = parent;
                    parent = Parent(parent);
                } else {
                    if (Color(Right(sibling)) == 1u) {
                        SetColor(Left(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateRight(sibling);
                        sibling = Right(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Right(sibling), 1u);
                    RotateLeft(parent);
                    break;
                }
            } else {
                auto sibling = Left(parent);
                if (Color(sibling) == 0u) {
                    SetColor(sibling, 1u);
                    SetColor(parent, 0u);
                    RotateRight(parent);
                    sibling = Left(parent);
                }
                if (Color(Right(sibling)) == 1u &&
                    Color(Left(sibling)) == 1u) {
                    SetColor(sibling, 0u);
                    node = parent;
                    parent = Parent(parent);
                } else {
                    if (Color(Left(sibling)) == 1u) {
                        SetColor(Right(sibling), 1u);
                        SetColor(sibling, 0u);
                        RotateLeft(sibling);
                        sibling = Left(parent);
                    }
                    SetColor(sibling, Color(parent));
                    SetColor(parent, 1u);
                    SetColor(Left(sibling), 1u);
                    RotateRight(parent);
                    break;
                }
            }
        }
        SetColor(node, 1u);
    }

    AudioManagerPrefixState& manager_;
    std::uint32_t nil_;
    IAudioManagerTreeRuntime& runtime_;
};
}  // namespace

std::uint32_t& AudioManager::RemoveTreeNode(
    AudioManagerPrefixState& manager,
    const AudioManagerTreeGlobalState& tree, std::uint32_t& output,
    const std::uint32_t node, IAudioManagerTreeRuntime& runtime) {
    AudioTreeRebalancer rebalancer(manager, tree, runtime);
    output = rebalancer.Remove(node);
    return output;
}

std::uint32_t AudioManager::FreeTreeNodes(
    std::uint32_t node, const AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime) {
    while (node != tree.sentinel) {
        const auto right = runtime.ReadNodeWord(node, 8u);
        (void)FreeTreeNodes(right, tree, runtime);
        const auto left = runtime.ReadNodeWord(node, 0u);
        (void)runtime.Free(node);
        node = left;
    }
    return tree.sentinel;
}

std::uint32_t AudioManager::LastTreeNode(
    std::uint32_t node, const AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime) {
    auto child = runtime.ReadNodeWord(node, 0u);
    while (child != tree.sentinel) {
        node = child;
        child = runtime.ReadNodeWord(node, 0u);
    }
    return node;
}

std::uint32_t AudioManager::NextTreeNode(
    std::uint32_t& cursor, const AudioManagerTreeGlobalState& tree,
    IAudioManagerTreeRuntime& runtime) {
    auto node = runtime.ReadNodeWord(cursor, 8u);
    if (node != tree.sentinel) {
        auto child = runtime.ReadNodeWord(node, 0u);
        while (child != tree.sentinel) {
            node = child;
            child = runtime.ReadNodeWord(node, 0u);
        }
        cursor = node;
        return child;
    }
    auto parent = runtime.ReadNodeWord(cursor, 4u);
    while (cursor == runtime.ReadNodeWord(parent, 8u)) {
        cursor = parent;
        parent = runtime.ReadNodeWord(parent, 4u);
    }
    if (runtime.ReadNodeWord(cursor, 8u) != parent)
        cursor = parent;
    return parent;
}

std::uint32_t AudioManager::Deinitialize(
    AudioManagerPrefixState& manager,
    IAudioManagerShutdownRuntime& runtime) {
    manager.header_flags[0] = 0u;
    runtime.CleanupTail(manager);
    runtime.CleanupSamples(manager);
    (void)runtime.FreeBuffer(manager.spare.begin);
    manager.spare = {};
    (void)runtime.FreeBuffer(manager.samples2d.begin);
    manager.samples2d = {};
    (void)runtime.FreeBuffer(manager.samples3d.begin);
    manager.samples3d = {};
    (void)runtime.FreeBuffer(manager.effects.begin);
    manager.effects = {};
    const auto result = runtime.FreeBuffer(manager.streams.begin);
    manager.streams = {};
    manager.state = 1u;
    return result;
}

namespace {

void ReservePointers(AudioPointerArrayState& array,
                     const std::uint32_t count, const bool notify_range,
                     IAudioSystemRuntime& runtime) {
    const auto capacity = array.begin == 0u
                              ? 0u : (array.capacity - array.begin) / 4u;
    if (capacity >= count)
        return;
    const auto old_begin = array.begin;
    const auto old_end = array.end;
    const auto old_count = old_begin == 0u
                               ? 0u : (old_end - old_begin) / 4u;
    const auto memory = runtime.Allocate(4u * count);
    for (auto index = 0u; index < old_count; ++index)
        runtime.CopyPointer(memory + index * 4u,
                            old_begin + index * 4u);
    if (notify_range)
        runtime.RangeNoOp(old_begin, old_end);
    runtime.Free(old_begin);
    array.begin = memory;
    array.end = memory + old_count * 4u;
    array.capacity = memory + count * 4u;
}

}  // namespace

bool AudioSystem::Initialize(
    AudioManagerPrefixState& manager,
    const AudioDriverConfigState* requested,
    IAudioSystemRuntime& runtime) {
    if (manager.header_flags[0] == 1u)
        return true;
    manager.listener = manager.driver = manager.provider = 0u;
    for (auto& bits : manager.volume_bits)
        bits = 0x3f800000u;
    manager.header_flags[0] = 1u;
    runtime.Log(83u);
    runtime.SetRedistDirectory("mss\\win32\\");
    runtime.PrepareMiles();
    if (!runtime.StartupMiles())
        return false;

    AudioDriverConfigState defaults{};
    (void)AudioDriverConfig::Construct(defaults, runtime);
    const auto& config = requested == nullptr ? defaults : *requested;
    manager.driver = runtime.OpenDigitalDriver(
        config.sample_rate, config.bits_per_sample,
        config.channel_count, 0u);
    if (manager.driver == 0u) {
        runtime.ReadLastError();
        runtime.Log(84u);
        runtime.CleanupConfig(defaults);
        return false;
    }

    runtime.Log(85u);
    runtime.SetMasterVolume(manager.driver, 0x3f800000u);
    manager.provider = runtime.Find3DProvider(config.provider.data);
    if (manager.provider == 0u ||
        runtime.Open3DProvider(manager.provider) != 0) {
        runtime.Log(89u);
        runtime.CleanupConfig(defaults);
        return false;
    }

    runtime.Log(86u);
    const auto supported = runtime.ProviderAttribute(
        manager.provider, "Maximum supported samples");
    manager.max_3d_samples = supported > 10u ? 10u : supported;
    runtime.Log(87u);
    manager.listener = runtime.Open3DListener(manager.provider);
    const auto position = runtime.ListenerPosition();
    runtime.Set3DPosition(manager.listener,
                          {position.x, position.y, position.z ^ 0x80000000u});
    runtime.Set3DOrientation(
        manager.listener,
        {position.x, position.y, 0x3f800000u},
        {0u, 0x3f800000u, 0x80000000u});

    ReservePointers(manager.streams, manager.default_provider_count,
                    false, runtime);
    ReservePointers(manager.samples3d, manager.max_3d_samples,
                    true, runtime);
    ReservePointers(manager.effects, manager.sample_capacity,
                    true, runtime);

    for (auto index = 0u; index < manager.sample_capacity; ++index) {
        const auto sample = runtime.AllocateSample(manager.driver);
        if (sample == 0u) {
            manager.sample_capacity = index + 1u;
            break;
        }
        runtime.InitializeSample(sample);
        runtime.InsertHandle(manager.samples2d, sample);
    }
    for (auto index = 0u; index < manager.max_3d_samples; ++index) {
        const auto sample = runtime.Allocate3DSample(manager.provider);
        if (sample == 0u) {
            manager.max_3d_samples = index + 1u;
            break;
        }
        runtime.InsertHandle(manager.spare, sample);
    }
    runtime.TraverseTree(manager);
    runtime.Log(88u);
    runtime.CleanupConfig(defaults);
    return true;
}

std::uint32_t AudioSystem::Shutdown(
    AudioManagerPrefixState& manager, AudioManagerTreeGlobalState& tree,
    IAudioSystemShutdownRuntime& runtime) {
    auto clear_objects = [&](AudioPointerArrayState& array,
                             const bool notify_range) {
        for (auto index = 0u; array.begin != 0u &&
             index < (array.end - array.begin) / 4u; ++index) {
            const auto slot = array.begin + index * 4u;
            const auto object = runtime.ReadPointer(slot);
            if (object == 0u)
                continue;
            runtime.StopObject(object);
            const auto current = runtime.ReadPointer(slot);
            if (current != 0u)
                runtime.DeleteObject(current, 1u);
            runtime.WritePointer(slot, 0u);
        }
        if (notify_range)
            runtime.RangeNoOp(array.begin, array.end);
        array.end = array.begin;
    };
    clear_objects(manager.streams, false);
    clear_objects(manager.effects, true);
    clear_objects(manager.samples3d, true);

    for (auto index = 0u; manager.samples2d.begin != 0u &&
         index < (manager.samples2d.end - manager.samples2d.begin) / 4u;
         ++index)
        runtime.ReleaseSample(
            runtime.ReadPointer(manager.samples2d.begin + index * 4u));
    runtime.RangeNoOp(manager.samples2d.begin, manager.samples2d.end);
    manager.samples2d.end = manager.samples2d.begin;
    for (auto index = 0u; manager.spare.begin != 0u &&
         index < (manager.spare.end - manager.spare.begin) / 4u;
         ++index)
        runtime.Release3DSample(
            runtime.ReadPointer(manager.spare.begin + index * 4u));
    manager.spare.end = manager.spare.begin;

    if (runtime.PackageOpen(manager))
        runtime.ClosePackage(manager);
    runtime.ClearSoundPool();
    runtime.Close3DListener(manager.listener);
    runtime.Close3DProvider(manager.provider);
    runtime.CloseDigitalDriver(manager.driver);
    runtime.ShutdownMiles();

    const auto root = manager.tree_root;
    auto first = runtime.ReadNodeWord(root, 0u);
    if (manager.tree_count != 0u) {
        runtime.FreeNodesRecursive(runtime.ReadNodeWord(root, 4u));
        runtime.WriteNodeWord(root, 4u, tree.sentinel);
        manager.tree_count = 0u;
        runtime.WriteNodeWord(root, 0u, root);
        runtime.WriteNodeWord(root, 8u, root);
    } else {
        while (first != root) {
            const auto current = first;
            first = runtime.NextNode(current);
            runtime.RemoveNode(manager, current);
        }
    }
    return root;
}

std::uint8_t AudioSystem::SetActive(
    AudioManagerPrefixState& manager, const std::uint8_t enabled,
    AudioVolumeGlobalsState& globals,
    IAudioSystemVolumeRuntime& runtime) {
    for (auto group = 0u; group < 3u; ++group) {
        const auto bit = static_cast<std::uint8_t>(1u << group);
        if ((globals.guard & bit) == 0u) {
            globals.guard = static_cast<std::uint8_t>(globals.guard | bit);
            globals.saved_bits[group] = manager.volume_bits[group];
        }
    }

    std::uint32_t result = enabled;
    auto set_volume = [&](const std::uint32_t group) {
        if (enabled)
            manager.volume_bits[group] = globals.saved_bits[group];
        else {
            globals.saved_bits[group] = manager.volume_bits[group];
            manager.volume_bits[group] = 0u;
        }
    };
    auto process = [&](AudioPointerArrayState& array,
                       const std::uint32_t kind) {
        for (auto index = 0u;; ++index) {
            if (array.begin == 0u)
                return;
            result = (array.end - array.begin) / 4u;
            if (index >= result)
                return;
            result = array.begin;
            const auto slot = array.begin + index * 4u;
            const auto object = runtime.ReadPointer(slot);
            if (object == 0u)
                continue;
            if (kind == 0u) {
                const auto callback_result = runtime.SetStreamVolume(
                    object, manager.volume_bits[0]);
                if (enabled)
                    result = callback_result;
            } else {
                result = runtime.ObjectKind(object);
                if (result == kind) {
                    const auto current = runtime.ReadPointer(slot);
                    result = runtime.RefreshObject(current);
                }
            }
        }
    };
    set_volume(0u);
    process(manager.streams, 0u);
    set_volume(1u);
    process(manager.effects, 1u);
    set_volume(2u);
    process(manager.samples3d, 2u);
    if (!enabled)
        result = manager.samples3d.begin == 0u
                     ? 0u : (manager.samples3d.end - manager.samples3d.begin) / 4u;
    return static_cast<std::uint8_t>(result);
}

std::uint32_t& AudioSystem::PlaySound(
    AudioManagerPrefixState& manager, std::uint32_t& output,
    const std::uint32_t source, const std::int32_t mode,
    const std::int32_t flags, IAudioPlaybackRuntime& runtime) {
    if (manager.driver == 0u) {
        output = 0u;
        return output;
    }
    const auto count = manager.streams.begin == 0u
                           ? 0u : (manager.streams.end - manager.streams.begin) / 4u;
    if (count >= manager.default_provider_count)
        runtime.CleanupSounds(manager.streams, source);
    const auto memory = runtime.Allocate(64u);
    std::uint32_t object = 0u;
    if (memory != 0u) {
        const auto identifier = manager.state++;
        object = runtime.ConstructSoundObject(memory, identifier, flags);
    }
    switch (runtime.PlaySoundObject(object, source, manager, mode)) {
        case 0:
            runtime.InsertObject(manager.streams, object);
            output = runtime.ObjectId(object);
            return output;
        case 1:
        case 4:
            runtime.Log(90u);
            break;
        case 2:
            break;
        default:
            runtime.Log(91u);
            break;
    }
    runtime.DestroyObject(object, 1u);
    output = 0u;
    return output;
}

std::uint32_t& AudioSystem::PlayMusic(
    AudioManagerPrefixState& manager, std::uint32_t& output,
    const std::uint32_t source, const std::int32_t mode,
    IAudioPlaybackRuntime& runtime) {
    if (manager.driver == 0u) {
        output = 0u;
        return output;
    }
    auto& pool = manager.samples2d;
    if (pool.begin == 0u || pool.end == pool.begin)
        runtime.CleanupSounds(manager.effects, source);
    if (pool.begin == 0u || pool.end == pool.begin) {
        output = 0u;
        return output;
    }

    const auto sample_slot = pool.end - 4u;
    const auto sample = runtime.ReadPointer(sample_slot);
    runtime.RangeNoOp(sample_slot, pool.end);
    pool.end -= 4u;
    const auto memory = runtime.Allocate(48u);
    std::uint32_t object = 0u;
    if (memory != 0u) {
        const auto identifier = manager.state++;
        object = runtime.ConstructMusicObject(memory, identifier);
    }
    switch (runtime.PlayMusicObject(object, sample, source, manager, mode)) {
        case 0:
            runtime.InsertObject(manager.effects, object);
            output = runtime.ObjectId(object);
            return output;
        case 1:
        case 4:
            runtime.Log(92u);
            runtime.DestroyObject(object, 1u);
            break;
        case 2:
            runtime.Log(125u);
            runtime.DestroyObject(object, 1u);
            break;
        default:
            runtime.Log(91u);
            break;
    }
    output = 0u;
    return output;
}

std::uint32_t& AudioSystem::Play3DSound(
    AudioManagerPrefixState& manager, std::uint32_t& output,
    const std::uint32_t source, const AudioListenerPositionBits position,
    const std::int32_t first_mode, const std::int32_t second_mode,
    const std::int32_t third_mode, IAudioPlaybackRuntime& runtime) {
    if (manager.driver == 0u || manager.provider == 0u) {
        output = 0u;
        return output;
    }
    auto& pool = manager.spare;
    if (pool.begin == 0u || pool.end == pool.begin)
        runtime.CleanupSounds(manager.samples3d, source);
    if (pool.begin == 0u || pool.end == pool.begin) {
        output = 0u;
        return output;
    }

    const auto sample_slot = pool.end - 4u;
    const auto sample = runtime.ReadPointer(sample_slot);
    runtime.RangeNoOp(sample_slot, pool.end);
    pool.end -= 4u;
    const auto memory = runtime.Allocate(96u);
    std::uint32_t object = 0u;
    if (memory != 0u) {
        const auto identifier = manager.state++;
        object = runtime.Construct3DObject(memory, identifier);
    }
    runtime.Set3DSourcePosition(object, position);
    switch (runtime.Play3DObject(object, sample, source, manager,
                                 first_mode, second_mode, third_mode)) {
        case 0:
            runtime.InsertObject(manager.samples3d, object);
            output = runtime.ObjectId(object);
            return output;
        case 1:
        case 4:
            runtime.Log(92u);
            runtime.DestroyObject(object, 1u);
            break;
        case 2:
            runtime.Log(125u);
            runtime.DestroyObject(object, 1u);
            break;
        default:
            runtime.Log(91u);
            break;
    }
    output = 0u;
    return output;
}

std::uint32_t AudioSystem::CleanupSounds(
    AudioPointerArrayState& array, const char* requested_name,
    IAudioCleanupRuntime& runtime) {
    for (auto slot = array.begin; slot != array.end; slot += 4u) {
        const auto object = runtime.ReadPointer(slot);
        if (runtime.IsActive(object))
            continue;
        runtime.Destroy(object, 1u);
        for (auto next = slot + 4u; next != array.end; next += 4u)
            runtime.WritePointer(next - 4u, runtime.ReadPointer(next));
        runtime.RangeNoOp(array.end - 4u, array.end);
        array.end -= 4u;
        return array.end;
    }

    auto selected = array.end;
    for (auto slot = array.begin; slot != array.end; slot += 4u) {
        const auto object = runtime.ReadPointer(slot);
        const auto name = runtime.Name(object);
        if (std::memcmp(name.data, requested_name, name.length + 1u) == 0 &&
            (selected == array.end ||
             runtime.Priority(runtime.ReadPointer(selected)) >
                 runtime.Priority(object)))
            selected = slot;
    }

    if (selected == array.end) {
        for (auto slot = array.begin; slot != array.end; slot += 4u) {
            const auto object = runtime.ReadPointer(slot);
            if (selected == array.end ||
                runtime.Priority(runtime.ReadPointer(selected)) >
                    runtime.Priority(object))
                selected = slot;
        }
        if (selected == array.end)
            return array.end;
        const auto object = runtime.ReadPointer(selected);
        if (object != 0u)
            runtime.Destroy(object, 1u);
        const auto old_end = array.end;
        for (auto next = selected + 4u; next != old_end; next += 4u)
            runtime.WritePointer(next - 4u, runtime.ReadPointer(next));
        array.end -= 4u;
        return old_end;
    }

    const auto object = runtime.ReadPointer(selected);
    if (object != 0u)
        runtime.Destroy(object, 1u);
    const auto old_end = array.end;
    for (auto next = selected + 4u; next != old_end; next += 4u)
        runtime.WritePointer(next - 4u, runtime.ReadPointer(next));
    array.end -= 4u;
    return selected + 4u == old_end ? old_end : array.end;
}

std::uint32_t AudioSystem::FindSoundObject(
    const AudioManagerPrefixState& manager, const std::uint32_t identifier,
    IAudioObjectLookupRuntime& runtime) {
    for (const auto* array : {&manager.streams, &manager.effects,
                              &manager.samples3d}) {
        for (auto slot = array->begin; slot != array->end; slot += 4u) {
            const auto object = runtime.ReadPointer(slot);
            if (runtime.ObjectId(object) == identifier)
                return object;
        }
    }
    return 0u;
}

std::uint32_t AudioSystem::UpdateAllSounds(
    AudioManagerPrefixState& manager, const std::uint32_t time_bits,
    IAudioStreamUpdateRuntime& runtime) {
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u;; ++index) {
        const auto begin = manager.streams.begin;
        if (begin == 0u)
            break;
        result = (manager.streams.end - begin) / 4u;
        if (index >= result)
            break;
        result = begin;
        const auto stream = runtime.ReadPointer(begin + 4u * index);
        if (stream != 0u)
            result = runtime.Update(stream, time_bits);
    }
    return result;
}

std::uint32_t AudioManager::RemoveSampleByKind(
    AudioManagerPrefixState& manager, const std::uint32_t object,
    IAudioObjectRemovalRuntime& runtime) {
    const auto remove_from = [&](AudioPointerArrayState& array,
                                 const bool return_old_end) -> std::uint32_t {
        for (auto slot = array.begin; slot != array.end; slot += 4u) {
            if (runtime.ReadPointer(slot) != object)
                continue;
            if (object != 0u)
                runtime.Destroy(object, 1u);
            const auto old_end = array.end;
            for (auto next = slot + 4u; next != old_end; next += 4u)
                runtime.WritePointer(next - 4u, runtime.ReadPointer(next));
            array.end -= 4u;
            return return_old_end ? old_end : array.end;
        }
        return array.end;
    };

    if (runtime.Kind(object) == 0u)
        return remove_from(manager.streams, false);
    const auto effect_end = manager.effects.end;
    const auto effect_result = remove_from(manager.effects, false);
    if (effect_result != effect_end)
        return effect_result;
    return remove_from(manager.samples3d, true);
}

namespace {
std::uint32_t AppendSampleHandle(
    AudioPointerArrayState& pool, const std::uint32_t sample,
    IAudioSamplePoolRuntime& runtime) {
    if (pool.end != pool.capacity) {
        runtime.WritePointer(pool.end, sample);
        pool.end += 4u;
        return pool.end;
    }

    const auto old_begin = pool.begin;
    const auto old_end = pool.end;
    const auto old_count = old_begin == 0u ? 0u : (old_end - old_begin) / 4u;
    const auto new_count = old_count + (old_count > 1u ? old_count : 1u);
    const auto new_begin = runtime.Allocate(new_count * 4u);
    for (std::uint32_t index = 0u; index < old_count; ++index)
        runtime.WritePointer(new_begin + 4u * index,
                             runtime.ReadPointer(old_begin + 4u * index));
    runtime.WritePointer(new_begin + 4u * old_count, sample);
    runtime.RangeNoOp(old_begin, old_end);
    runtime.Free(old_begin);
    pool.begin = new_begin;
    pool.end = new_begin + 4u * (old_count + 1u);
    pool.capacity = new_begin + 4u * new_count;
    return pool.end;
}
}  // namespace

std::uint32_t AudioManager::AddSample(
    AudioManagerPrefixState& manager, const std::uint32_t sample,
    IAudioSamplePoolRuntime& runtime) {
    return AppendSampleHandle(manager.samples2d, sample, runtime);
}

std::uint32_t AudioManager::Add3DSample(
    AudioManagerPrefixState& manager, const std::uint32_t sample,
    IAudioSamplePoolRuntime& runtime) {
    return AppendSampleHandle(manager.spare, sample, runtime);
}

bool AudioHandle::IsActive(const AudioHandleState& handle,
                           IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    return AudioSystem::FindSoundObject(
               manager, handle.identifier, runtime) != 0u;
}

std::uint8_t AudioHandle::Play(const AudioHandleState& handle,
                               IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    runtime.Start(object);
    auto& remove_manager = runtime.GetManager();
    (void)AudioManager::RemoveSampleByKind(remove_manager, object, runtime);
    return 1u;
}

std::uint8_t AudioHandle::Stop(const AudioHandleState& handle,
                               IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    runtime.Stop(object);
    return 1u;
}

std::uint8_t AudioHandle::Pause(const AudioHandleState& handle,
                                IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    runtime.Pause(object);
    return 1u;
}

std::uint8_t AudioHandle::IsPlaying(const AudioHandleState& handle,
                                    IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    return object == 0u ? 0u :
        static_cast<std::uint8_t>(runtime.IsPlaying(object));
}

std::uint32_t AudioHandle::FadeIn(
    const AudioHandleState& handle, const std::uint32_t time_bits,
    IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    const auto kind = runtime.Kind(object);
    return kind == 0u ? runtime.FadeIn(object, time_bits) : kind;
}

std::uint32_t AudioHandle::FadeOut(
    const AudioHandleState& handle, const std::uint32_t time_bits,
    IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    const auto kind = runtime.Kind(object);
    return kind == 0u ? runtime.FadeOut(object, time_bits) : kind;
}

std::uint32_t AudioHandle::SetProperty(
    const AudioHandleState& handle, const std::uint32_t value,
    IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u)
        return 0u;
    const auto kind = runtime.Kind(object);
    if (kind == 0u) {
        runtime.WriteProperty(object, value);
        return value;
    }
    return kind;
}

std::uint8_t AudioHandle::Set3DPosition(
    const AudioHandleState& handle, const AudioListenerPositionBits position,
    IAudioHandleRuntime& runtime) {
    auto& manager = runtime.GetManager();
    const auto object = AudioSystem::FindSoundObject(
        manager, handle.identifier, runtime);
    if (object == 0u || runtime.Kind(object) != 2u)
        return 0u;
    runtime.Set3DPosition(object, position);
    return 1u;
}

}  // namespace pal4
