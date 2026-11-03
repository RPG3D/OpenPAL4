#pragma once

#include "pal4/npc_info_manager.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class MonsterInfoRecord final {
public:
    static constexpr std::uint32_t kBytes = 0x50u;
    static void Initialize(std::uint32_t record, INpcInfoRuntime& runtime);
    static void Read(std::uint32_t record, std::uint32_t stream,
                     INpcInfoRuntime& runtime);
    static void InitializeDocuments(std::uint32_t record,
                                    INpcInfoRuntime& runtime);
    static void CleanupData(std::uint32_t record,
                            INpcInfoRuntime& runtime);
    static void CleanupStrings(std::uint32_t record,
                               INpcInfoRuntime& runtime);
    static void Destroy(std::uint32_t record, INpcInfoRuntime& runtime);
};

struct MonsterInfoSharedTree {
    std::uint32_t sentinel = 0;
    std::uint32_t instances = 0;
};

class MonsterInfoManager final {
public:
    using Visitor = bool (*)(std::uint32_t record, std::uint32_t context);
    MonsterInfoManager(std::uint32_t address, MonsterInfoSharedTree& shared,
                       INpcInfoRuntime& runtime);
    [[nodiscard]] bool Initialize();
    void Clear();
    void DestroyTreeContainer();
    void Destroy();
    [[nodiscard]] std::uint32_t Find(std::string_view name);
    [[nodiscard]] bool ForEach(Visitor visitor, std::uint32_t context) const;
    [[nodiscard]] bool Load(std::string_view directory, std::string_view name);

private:
    [[nodiscard]] bool Insert(std::string name, std::uint32_t record);
    std::uint32_t address_;
    MonsterInfoSharedTree& shared_;
    INpcInfoRuntime& runtime_;
    bool initialized_ = false;
};

}  // namespace pal4
