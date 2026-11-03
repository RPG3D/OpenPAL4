#include "pal4/npc_info_manager.h"
#include "pal4/info_name_tree.h"

#include <utility>

namespace pal4 {

void NpcInfoRecord::Initialize(const std::uint32_t record,
                               INpcInfoRuntime& runtime) {
    for (const auto offset : {0u, 12u, 24u})
        runtime.InitializeString(record + offset);
    runtime.WriteWord(record + 76u, 0);
    runtime.WriteWord(record + 80u, 0);
}

void NpcInfoRecord::Read(const std::uint32_t record,
                         const std::uint32_t stream,
                         INpcInfoRuntime& runtime) {
    for (const auto offset : {0u, 12u, 24u})
        runtime.ReadStreamString(stream, record + offset);
    for (const auto offset : {36u, 48u})
        runtime.ReadStreamData(stream, record + offset, 12u);
    for (const auto offset : {60u, 64u, 68u})
        runtime.ReadStreamData(stream, record + offset, 4u);
    const auto kind = runtime.StreamKind(stream);
    if (kind == 1u || kind == 2u) {
        const auto facet = runtime.CreateLocaleFacet(runtime.StreamLocale(stream));
        runtime.CheckXml(facet, runtime.ReadWord(record + 76u));
        runtime.CheckXml(facet, runtime.ReadWord(record + 80u));
        runtime.DestroyLocaleFacet(facet);
    }
}

void NpcInfoRecord::InitializeDocuments(const std::uint32_t record,
                                        INpcInfoRuntime& runtime) {
    for (auto kind = 0u; kind != 2u; ++kind) {
        const auto storage = runtime.Allocate(0x3cu);
        runtime.WriteWord(record + 76u + kind * 4u,
                          storage ? runtime.InitializeXml(storage, kind) : 0u);
    }
}

void NpcInfoRecord::CleanupDocuments(const std::uint32_t record,
                                     INpcInfoRuntime& runtime) {
    for (const auto offset : {76u, 80u}) {
        const auto document = runtime.ReadWord(record + offset);
        if (document != 0u) {
            runtime.DestroyXml(document);
            runtime.Free(document);
            runtime.WriteWord(record + offset, 0);
        }
    }
}

void NpcInfoRecord::CleanupStrings(const std::uint32_t record,
                                   INpcInfoRuntime& runtime) {
    for (const auto offset : {24u, 12u, 0u})
        runtime.CleanupString(record + offset);
}

void NpcInfoRecord::Destroy(const std::uint32_t record,
                            INpcInfoRuntime& runtime) {
    if (!record) return;
    CleanupDocuments(record, runtime);
    CleanupStrings(record, runtime);
    runtime.Free(record);
}

NpcInfoManager::NpcInfoManager(const std::uint32_t address,
                               NpcInfoSharedTree& shared,
                               INpcInfoRuntime& runtime)
    : address_(address), shared_(shared), runtime_(runtime) {}

bool NpcInfoManager::Initialize() {
    if (initialized_) return true;
    runtime_.InitializeString(address_ + 4u);
    runtime_.WriteWord(address_ + 16u, 0);
    runtime_.WriteWord(address_ + 28u, 0);
    runtime_.WriteWord(address_ + 36u, 0);
    const auto candidate = runtime_.Allocate(0x24u);
    if (!candidate) return false;
    if (!shared_.sentinel) {
        shared_.sentinel = candidate;
        runtime_.WriteWord(shared_.sentinel, 0);
        runtime_.WriteWord(shared_.sentinel + 4u, 0);
        runtime_.WriteWord(shared_.sentinel + 8u, 0);
        runtime_.WriteWord(shared_.sentinel + 32u, 1);
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
    runtime_.WriteWord(address_, 0x844900u);
    runtime_.WriteWord(address_ + 24u, head);
    runtime_.WriteWord(address_ + 32u, 0);
    initialized_ = true;
    return true;
}

void NpcInfoManager::Clear() {
    const auto head = runtime_.ReadWord(address_ + 24u);
    if (head) {
        for (auto node = runtime_.ReadWord(head); node != head;
             node = InfoNameTree::Next(node, head, shared_.sentinel, runtime_))
            NpcInfoRecord::Destroy(runtime_.ReadWord(node + 28u), runtime_);
        InfoNameTree::ClearNodes(address_ + 20u, shared_.sentinel, runtime_);
    }
    runtime_.WriteWord(address_ + 16u, 0);
}

void NpcInfoManager::DestroyTreeContainer() {
    if (!initialized_) return;
    Clear();
    runtime_.Free(runtime_.ReadWord(address_ + 24u));
    runtime_.WriteWord(address_ + 24u, 0);
    runtime_.WriteWord(address_ + 32u, 0);
    --shared_.instances;
    if (!shared_.instances) {
        runtime_.Free(shared_.sentinel);
        shared_.sentinel = 0;
    }
    initialized_ = false;
}

void NpcInfoManager::Destroy() {
    if (!initialized_) return;
    runtime_.WriteWord(address_, 0x844900u);
    DestroyTreeContainer();
    runtime_.CleanupString(address_ + 4u);
}

bool NpcInfoManager::Insert(std::string name, const std::uint32_t record) {
    return InfoNameTree::Insert(address_ + 20u, name, record,
                                shared_.sentinel, runtime_).inserted;
}

std::uint32_t NpcInfoManager::CreateOrGet(
    const std::string_view name, const std::string_view resource,
    const std::array<std::uint32_t, 3>& position,
    const std::array<std::uint32_t, 3>& rotation) {
    if (!initialized_) return 0;
    if (const auto existing = Find(name)) return existing;
    const auto record = runtime_.Allocate(NpcInfoRecord::kBytes);
    if (!record) return 0;
    NpcInfoRecord::Initialize(record, runtime_);
    runtime_.AssignString(record, name);
    runtime_.AssignString(record + 12u, resource);
    for (auto index = 0u; index != 3u; ++index) {
        runtime_.WriteWord(record + 36u + index * 4u, position[index]);
        runtime_.WriteWord(record + 48u + index * 4u, rotation[index]);
    }
    runtime_.WriteWord(record + 60u, 1);
    runtime_.WriteWord(record + 64u, 1);
    runtime_.WriteWord(record + 72u, 0);
    NpcInfoRecord::InitializeDocuments(record, runtime_);
    if (!Insert(std::string(name), record)) {
        NpcInfoRecord::Destroy(record, runtime_);
        return 0;
    }
    runtime_.WriteWord(address_ + 16u, record);
    return record;
}

std::uint32_t NpcInfoManager::Find(const std::string_view name) {
    if (!initialized_) return 0;
    const auto head = runtime_.ReadWord(address_ + 24u);
    const auto node = InfoNameTree::Find(address_ + 20u, name,
                                          shared_.sentinel, runtime_);
    if (node == head) return 0;
    const auto record = runtime_.ReadWord(node + 28u);
    runtime_.WriteWord(address_ + 16u, record);
    return record;
}

bool NpcInfoManager::Remove(const std::string_view name) {
    runtime_.WriteWord(address_ + 16u, 0);
    if (!initialized_) return false;
    const auto head = runtime_.ReadWord(address_ + 24u);
    const auto node = InfoNameTree::Find(address_ + 20u, name,
                                          shared_.sentinel, runtime_);
    if (node == head) return false;
    NpcInfoRecord::Destroy(runtime_.ReadWord(node + 28u), runtime_);
    InfoNameTree::Erase(address_ + 20u, node, shared_.sentinel, runtime_);
    return true;
}

bool NpcInfoManager::ForEach(const Visitor visitor,
                             const std::uint32_t context) const {
    if (!visitor) return false;
    if (!initialized_) return true;
    const auto head = runtime_.ReadWord(address_ + 24u);
    for (auto node = runtime_.ReadWord(head); node != head;
         node = InfoNameTree::Next(node, head, shared_.sentinel, runtime_))
        if (!visitor(runtime_.ReadWord(node + 28u), context)) return false;
    return true;
}

bool NpcInfoManager::Load(const std::string_view directory,
                          const std::string_view name) {
    if (!initialized_) return false;
    const auto stream = runtime_.OpenNpcInfo(directory, name);
    if (!stream) return false;
    Clear();
    const auto count = static_cast<std::int32_t>(runtime_.ReadStreamWord(stream));
    for (auto index = 0; index < count; ++index) {
        const auto record = runtime_.Allocate(NpcInfoRecord::kBytes);
        if (!record) {
            runtime_.CloseStream(stream);
            return false;
        }
        NpcInfoRecord::Initialize(record, runtime_);
        NpcInfoRecord::InitializeDocuments(record, runtime_);
        NpcInfoRecord::Read(record, stream, runtime_);
        if (!Insert(runtime_.ReadStringValue(record), record)) {
            NpcInfoRecord::Destroy(record, runtime_);
            runtime_.CloseStream(stream);
            return false;
        }
    }
    runtime_.CloseStream(stream);
    return true;
}

}  // namespace pal4
