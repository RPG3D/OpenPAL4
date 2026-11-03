#include "pal4/monster_behavior_tree.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_monster_behavior_tree_runtime.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t AddressOf(const void* pointer) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(pointer));
}

void Walk(const std::uint32_t node, const std::uint32_t nil,
          pal4::NativeMonsterBehaviorTreeRuntime& runtime,
          std::vector<std::uint32_t>& ids) {
    if (node == nil) return;
    Walk(runtime.ReadDword(node), nil, runtime, ids);
    ids.push_back(runtime.ReadDword(node + 12u));
    Walk(runtime.ReadDword(node + 8u), nil, runtime, ids);
}

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeMonsterBehaviorTreeRuntime runtime(memory);
    pal4::MonsterBehaviorTreeState first{};
    pal4::MonsterBehaviorTreeState second{};
    const auto first_address = AddressOf(&first);
    const auto second_address = AddressOf(&second);
    Check(pal4::MonsterBehaviorTree::Initialize(
              first, first_address, 0u, 0u, 0u, runtime) ==
              first_address + 4u &&
              runtime.SharedNil() != 0u &&
              runtime.SharedReferenceCount() == 1u &&
              memory.OutstandingAllocations() == 2u,
          "first behavior tree allocates nil and header");

    std::uint32_t final_node = 0u;
    for (std::uint32_t id = 0u; id < 5u; ++id) {
        const auto inserted = pal4::MonsterBehaviorTree::Insert(
            first, static_cast<std::int32_t>(id), 0x429a30u + id, runtime);
        if (id == 4u) final_node = inserted.node;
        Check(inserted.inserted && inserted.node != 0u &&
                  runtime.ReadDword(inserted.node + 12u) == id &&
                  runtime.ReadDword(inserted.node + 16u) == 0x429a30u + id,
              "native behavior node retains class and factory");
    }
    std::vector<std::uint32_t> ids;
    Walk(runtime.ReadDword(first.words[2] + 4u), runtime.SharedNil(),
         runtime, ids);
    Check(first.words[4] == 5u &&
              ids == std::vector<std::uint32_t>{0u, 1u, 2u, 3u, 4u} &&
              memory.OutstandingAllocations() == 7u,
          "five behavior classes form an ordered native tree");
    Check(!pal4::MonsterBehaviorTree::Insert(first, 2, 0u, runtime).inserted &&
              first.words[4] == 5u,
          "duplicate behavior class retains its original factory");
    runtime.RemoveNode(first_address + 4u, final_node);
    ids.clear();
    Walk(runtime.ReadDword(first.words[2] + 4u), runtime.SharedNil(),
         runtime, ids);
    Check(first.words[4] == 4u &&
              ids == std::vector<std::uint32_t>{0u, 1u, 2u, 3u} &&
              memory.OutstandingAllocations() == 6u,
          "native behavior node deletion updates links and count");

    (void)pal4::MonsterBehaviorTree::Initialize(
        second, second_address, 0u, 0u, 0u, runtime);
    Check(runtime.SharedReferenceCount() == 2u &&
              memory.OutstandingAllocations() == 7u,
          "second behavior tree shares the nil allocation");
    (void)pal4::MonsterBehaviorTree::Destroy(first, first_address, runtime);
    Check(runtime.SharedReferenceCount() == 1u &&
              memory.OutstandingAllocations() == 2u &&
              second.words[2] != 0u,
          "populated tree releases nodes while another header retains nil");
    (void)pal4::MonsterBehaviorTree::Destroy(second, second_address, runtime);
    Check(runtime.SharedReferenceCount() == 0u &&
              runtime.SharedNil() == 0u &&
              memory.OutstandingAllocations() == 0u,
          "last behavior tree releases shared nil and header");
    return EXIT_SUCCESS;
}
