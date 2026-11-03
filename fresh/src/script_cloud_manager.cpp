#include "pal4/script_cloud_manager.h"
#include "pal4/info_name_tree.h"

#include <utility>

namespace pal4 {

void CameraRecord::Initialize(const std::uint32_t record,
                              IScriptCloudRuntime& runtime) {
    runtime.InitializeString(record);
    for (const auto offset : {36u, 40u, 44u, 56u, 60u, 64u})
        runtime.WriteWord(record + offset, 0);
    runtime.WriteWord(record + 48u, 1u);
    runtime.WriteWord(record + 52u, 0x3f800000u);
    const auto storage = runtime.Allocate(0x3cu);
    const auto document = storage ? runtime.InitializeXml(storage, 4u) : 0u;
    runtime.WriteWord(record + 64u, document);
    if (document) runtime.CreateXmlNode(document);
}

void CameraRecord::Cleanup(const std::uint32_t record,
                           IScriptCloudRuntime& runtime) {
    const auto document = runtime.ReadWord(record + 64u);
    if (document) {
        runtime.DestroyXml(document);
        runtime.Free(document);
        runtime.WriteWord(record + 64u, 0);
    }
    runtime.CleanupString(record);
}

void CameraRecord::Read(const std::uint32_t record,
                        const std::uint32_t stream,
                        IScriptCloudRuntime& runtime) {
    runtime.ReadStreamString(stream, record);
    runtime.ReadStreamData(stream, record + 12u, 12u);
    runtime.ReadStreamData(stream, record + 24u, 12u);
    for (auto offset = 36u; offset <= 60u; offset += 4u)
        runtime.ReadStreamData(stream, record + offset, 4u);
    const auto kind = runtime.StreamKind(stream);
    if (kind == 1u || kind == 2u) {
        const auto facet = runtime.CreateLocaleFacet(runtime.StreamLocale(stream));
        runtime.CheckXml(facet, runtime.ReadWord(record + 64u));
        runtime.DestroyLocaleFacet(facet);
    }
}

ScriptCloudManager::ScriptCloudManager(const std::uint32_t address,
                                       ScriptCloudSharedTree& shared,
                                       IScriptCloudRuntime& runtime)
    : address_(address), shared_(shared), runtime_(runtime) {}

std::uint32_t ScriptCloudManager::Register(
    std::uint8_t& guard, std::uint32_t& registered,
    IScriptCloudRuntime& runtime) {
    if (guard & 1u) return registered;
    guard |= 1u;
    registered = runtime.RegisterSingleton(kSingletonName, kSingletonId,
                                           0x524c90u, 0x524cf0u);
    return registered;
}

bool ScriptCloudManager::Create() {
    if (address_) return Initialize();
    address_ = runtime_.Allocate(0x14u);
    if (!address_) return false;
    owned_ = true;
    if (Initialize()) return true;
    runtime_.Free(address_);
    address_ = 0;
    owned_ = false;
    return false;
}

bool ScriptCloudManager::Initialize() {
    if (initialized_) return true;
    if (!address_) return false;
    runtime_.WriteWord(address_ + 8u, 0);
    const auto candidate = runtime_.Allocate(0x24u);
    if (!candidate) return false;
    if (!shared_.sentinel) {
        shared_.sentinel = candidate;
        runtime_.WriteWord(candidate, 0);
        runtime_.WriteWord(candidate + 4u, 0);
        runtime_.WriteWord(candidate + 8u, 0);
        runtime_.WriteWord(candidate + 32u, 1);
    } else {
        runtime_.Free(candidate);
    }
    const auto head = runtime_.Allocate(0x24u);
    if (!head) {
        if (!shared_.instances) {
            runtime_.Free(shared_.sentinel);
            shared_.sentinel = 0;
        }
        return false;
    }
    ++shared_.instances;
    runtime_.WriteWord(head, head);
    runtime_.WriteWord(head + 4u, shared_.sentinel);
    runtime_.WriteWord(head + 8u, head);
    runtime_.WriteWord(head + 32u, 0);
    runtime_.WriteWord(address_ + 4u, head);
    runtime_.WriteWord(address_ + 12u, 0);
    runtime_.WriteWord(address_ + 16u, head);
    initialized_ = true;
    return true;
}

void ScriptCloudManager::Clear() {
    const auto head = runtime_.ReadWord(address_ + 4u);
    if (head) {
        for (auto node = runtime_.ReadWord(head); node != head;
             node = InfoNameTree::Next(node, head, shared_.sentinel, runtime_)) {
            const auto record = runtime_.ReadWord(node + 28u);
            if (!record) continue;
            CameraRecord::Cleanup(record, runtime_);
            runtime_.Free(record);
        }
        InfoNameTree::ClearNodes(address_, shared_.sentinel, runtime_);
    }
    cursor_ = head;
    runtime_.WriteWord(address_ + 16u, head);
}

void ScriptCloudManager::Cleanup() {
    if (!initialized_) return;
    Clear();
    runtime_.Free(runtime_.ReadWord(address_ + 4u));
    runtime_.WriteWord(address_ + 4u, 0);
    runtime_.WriteWord(address_ + 12u, 0);
    --shared_.instances;
    if (!shared_.instances) {
        runtime_.Free(shared_.sentinel);
        shared_.sentinel = 0;
    }
    initialized_ = false;
}

void ScriptCloudManager::Destroy() {
    Cleanup();
    if (owned_) {
        runtime_.Free(address_);
        address_ = 0;
        owned_ = false;
    }
}

bool ScriptCloudManager::Insert(std::string name,
                                const std::uint32_t record) {
    return InfoNameTree::Insert(address_, name, record,
                                shared_.sentinel, runtime_).inserted;
}

std::uint32_t ScriptCloudManager::Find(const std::string_view name) const {
    if (!initialized_) return 0;
    const auto head = runtime_.ReadWord(address_ + 4u);
    const auto node = InfoNameTree::Find(address_, name,
                                          shared_.sentinel, runtime_);
    return node == head ? 0u : runtime_.ReadWord(node + 28u);
}

std::uint32_t ScriptCloudManager::First() {
    if (!initialized_) return 0;
    const auto head = runtime_.ReadWord(address_ + 4u);
    cursor_ = runtime_.ReadWord(head);
    runtime_.WriteWord(address_ + 16u, cursor_);
    return cursor_ == head ? 0u : runtime_.ReadWord(cursor_ + 28u);
}

std::uint32_t ScriptCloudManager::Next() {
    if (!initialized_) return 0;
    const auto head = runtime_.ReadWord(address_ + 4u);
    if (!cursor_ || cursor_ == head) return 0;
    cursor_ = InfoNameTree::Next(cursor_, head, shared_.sentinel, runtime_);
    runtime_.WriteWord(address_ + 16u, cursor_);
    return cursor_ == head ? 0u : runtime_.ReadWord(cursor_ + 28u);
}

bool ScriptCloudManager::Load(const std::string_view world,
                              const std::string_view scene,
                              const std::string_view camera) {
    if (!initialized_) return false;
    runtime_.ProcessCameraScripts();
    const auto stream = runtime_.OpenCameraFile(world, scene, camera);
    if (!stream) {
        runtime_.CleanupCameraScripts();
        return false;
    }
    Clear();
    const auto count = static_cast<std::int32_t>(runtime_.ReadStreamWord(stream));
    for (auto index = 0; index < count; ++index) {
        const auto record = runtime_.Allocate(CameraRecord::kBytes);
        if (!record) {
            runtime_.CloseStream(stream);
            runtime_.CleanupCameraScripts();
            return false;
        }
        CameraRecord::Initialize(record, runtime_);
        CameraRecord::Read(record, stream, runtime_);
        if (!Insert(runtime_.ReadStringValue(record), record)) {
            CameraRecord::Cleanup(record, runtime_);
            runtime_.Free(record);
            runtime_.CloseStream(stream);
            runtime_.CleanupCameraScripts();
            return false;
        }
    }
    runtime_.CloseStream(stream);
    runtime_.CleanupCameraScripts();
    return true;
}

}  // namespace pal4
