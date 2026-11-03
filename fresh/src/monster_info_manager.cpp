#include "pal4/monster_info_manager.h"
#include "pal4/info_name_tree.h"

#include <utility>

namespace pal4 {

void MonsterInfoRecord::Initialize(const std::uint32_t record,
                                   INpcInfoRuntime& runtime) {
    runtime.InitializeString(record);
    runtime.InitializeString(record + 12u);
    for (auto offset = 52u; offset <= 76u; offset += 4u)
        runtime.WriteWord(record + offset, 0);
}

void MonsterInfoRecord::InitializeDocuments(const std::uint32_t record,
                                             INpcInfoRuntime& runtime) {
    for (auto kind = 0u; kind != 2u; ++kind) {
        const auto storage = runtime.Allocate(0x3cu);
        runtime.WriteWord(record + 72u + kind * 4u,
                          storage ? runtime.InitializeXml(storage, kind + 2u) : 0u);
    }
}

void MonsterInfoRecord::CleanupData(const std::uint32_t record,
                                    INpcInfoRuntime& runtime) {
    for (const auto offset : {72u, 76u}) {
        const auto document = runtime.ReadWord(record + offset);
        if (document) {
            runtime.DestroyXml(document);
            runtime.Free(document);
            runtime.WriteWord(record + offset, 0);
        }
    }
    const auto array = runtime.ReadWord(record + 68u);
    if (array) {
        runtime.Free(array);
        runtime.WriteWord(record + 68u, 0);
    }
}

void MonsterInfoRecord::CleanupStrings(const std::uint32_t record,
                                       INpcInfoRuntime& runtime) {
    runtime.CleanupString(record + 12u);
    runtime.CleanupString(record);
}

void MonsterInfoRecord::Destroy(const std::uint32_t record,
                                INpcInfoRuntime& runtime) {
    if (!record) return;
    CleanupData(record, runtime);
    CleanupStrings(record, runtime);
    runtime.Free(record);
}

MonsterInfoManager::MonsterInfoManager(const std::uint32_t address,
                                       MonsterInfoSharedTree& shared,
                                       INpcInfoRuntime& runtime)
    : address_(address), shared_(shared), runtime_(runtime) {}

bool MonsterInfoManager::Initialize() {
    if (initialized_) return true;
    runtime_.InitializeString(address_ + 4u);
    runtime_.WriteWord(address_ + 16u, 0);
    runtime_.WriteWord(address_ + 28u, 0);
    runtime_.WriteWord(address_ + 36u, 0);
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
    runtime_.WriteWord(address_, 0x844904u);
    runtime_.WriteWord(address_ + 24u, head);
    runtime_.WriteWord(address_ + 32u, 0);
    initialized_ = true;
    return true;
}

void MonsterInfoManager::Clear() {
    const auto head = runtime_.ReadWord(address_ + 24u);
    if (head) {
        for (auto node = runtime_.ReadWord(head); node != head;
             node = InfoNameTree::Next(node, head, shared_.sentinel, runtime_))
            MonsterInfoRecord::Destroy(runtime_.ReadWord(node + 28u), runtime_);
        InfoNameTree::ClearNodes(address_ + 20u, shared_.sentinel, runtime_);
    }
    runtime_.WriteWord(address_ + 16u, 0);
}

void MonsterInfoManager::DestroyTreeContainer() {
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

void MonsterInfoManager::Destroy() {
    if (!initialized_) return;
    runtime_.WriteWord(address_, 0x844904u);
    DestroyTreeContainer();
    runtime_.CleanupString(address_ + 4u);
}

bool MonsterInfoManager::Insert(std::string name, const std::uint32_t record) {
    return InfoNameTree::Insert(address_ + 20u, name, record,
                                shared_.sentinel, runtime_).inserted;
}

std::uint32_t MonsterInfoManager::Find(const std::string_view name) {
    if (!initialized_) return 0;
    const auto head = runtime_.ReadWord(address_ + 24u);
    const auto node = InfoNameTree::Find(address_ + 20u, name,
                                          shared_.sentinel, runtime_);
    if (node == head) return 0;
    const auto record = runtime_.ReadWord(node + 28u);
    runtime_.WriteWord(address_ + 16u, record);
    return record;
}

bool MonsterInfoManager::ForEach(const Visitor visitor,
                                 const std::uint32_t context) const {
    if (!visitor) return false;
    if (!initialized_) return true;
    const auto head = runtime_.ReadWord(address_ + 24u);
    for (auto node = runtime_.ReadWord(head); node != head;
         node = InfoNameTree::Next(node, head, shared_.sentinel, runtime_))
        if (!visitor(runtime_.ReadWord(node + 28u), context)) return false;
    return true;
}

bool MonsterInfoManager::Load(const std::string_view directory,
                              const std::string_view name) {
    if (!initialized_) return false;
    const auto stream = runtime_.OpenMonsterInfo(directory, name);
    if (!stream) return false;
    Clear();
    const auto count = static_cast<std::int32_t>(runtime_.ReadStreamWord(stream));
    for (auto index = 0; index < count; ++index) {
        const auto record = runtime_.Allocate(MonsterInfoRecord::kBytes);
        if (!record) {
            runtime_.CloseStream(stream);
            return false;
        }
        MonsterInfoRecord::Initialize(record, runtime_);
        MonsterInfoRecord::InitializeDocuments(record, runtime_);
        MonsterInfoRecord::Read(record, stream, runtime_);
        if (!Insert(runtime_.ReadStringValue(record), record)) {
            MonsterInfoRecord::Destroy(record, runtime_);
            runtime_.CloseStream(stream);
            return false;
        }
    }
    runtime_.CloseStream(stream);
    return true;
}

}  // namespace pal4
