#include "pal4/game_manager_components.h"
#include "pal4/game_manager_component_callback.h"

#include <algorithm>

namespace pal4 {
namespace {

struct BehaviorFactory {
    std::uint32_t getter;
    std::uint32_t factory;
};

constexpr std::array<BehaviorFactory, 5u> kBehaviorFactories{{
    {0x7e0e40u, 0x429c70u},
    {0x7e0fc0u, 0x429a30u},
    {0x7de4b0u, 0x42a130u},
    {0x5a6980u, 0x42a360u},
    {0x59b770u, 0x429ec0u},
}};

void Write(GameManagerComponentState& state,
           std::uint32_t offset, std::uint32_t value) {
    state.words[offset / 4u] = value;
}

struct CallbackStateWriter final : IGameManagerComponentCallbackRuntime {
    GameManagerComponentState& state;
    std::uint32_t base;
    CallbackStateWriter(GameManagerComponentState& state,
                        std::uint32_t base)
        : state(state), base(base) {}
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        state.words[(address - base) / 4u] = value;
    }
};

void WriteByte(GameManagerComponentState& state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = (offset & 3u) * 8u;
    const auto mask = 0xffu << shift;
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

MonsterBehaviorTreeState LoadBehaviorTree(
    const GameManagerComponentState& state) {
    MonsterBehaviorTreeState tree{};
    constexpr auto first_word = 0x698u / 4u;
    std::copy_n(state.words.begin() + first_word, tree.words.size(),
                tree.words.begin());
    return tree;
}

void StoreBehaviorTree(GameManagerComponentState& state,
                       const MonsterBehaviorTreeState& tree) {
    constexpr auto first_word = 0x698u / 4u;
    std::copy(tree.words.begin(), tree.words.end(),
              state.words.begin() + first_word);
}

}  // namespace

void GameManagerComponentRuntimeStub::ConstructBase(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::DestroyBase(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::InitializeEventProcessor(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::CleanupEventProcessor(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::InitializeScriptObject(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::CleanupScriptObject(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::InitializeBehaviorTree(
    GameManagerComponentState&, std::uint32_t) {}
void GameManagerComponentRuntimeStub::DestroyBehaviorTree(
    GameManagerComponentState&, std::uint32_t) {}
IMonsterBehaviorTreeRuntime& GameManagerComponentRuntimeStub::TreeRuntime() {
    return tree_;
}
void GameManagerComponentRuntimeStub::DestroyAuxiliary(
    GameManagerComponentState&, std::uint32_t) {}
std::uint32_t GameManagerComponentRuntimeStub::ClassIdFromGetter(
    std::uint32_t getter_address) {
    for (std::uint32_t i = 0u; i < kBehaviorFactories.size(); ++i)
        if (kBehaviorFactories[i].getter == getter_address) return i;
    return 0u;
}
GameManagerBehaviorInsertResult GameManagerComponentRuntimeStub::InsertBehavior(
    std::uint32_t, std::uint32_t, std::uint32_t) { return {}; }
void GameManagerComponentRuntimeStub::CopyInsertionResult(
    const GameManagerBehaviorInsertResult&) {}
void GameManagerComponentRuntimeStub::InsertFinalBehavior(
    std::uint32_t, std::uint32_t, std::uint32_t) {}

std::uint32_t GameManagerComponents::Initialize(
    GameManagerComponentState& state, std::uint32_t address,
    std::uint8_t incoming_stack_byte,
    IGameManagerComponentRuntime& runtime) {
    int stage = -1;
    try {
        runtime.ConstructBase(state, address);
        stage = 0;
        runtime.InitializeEventProcessor(state, address + kEventProcessorOffset);
        stage = 1;
        runtime.InitializeScriptObject(state, address + kScriptObjectOffset);
        stage = 2;
        WriteByte(state, kTreeOffset, incoming_stack_byte);
        WriteByte(state, kTreeOffset + 1u, incoming_stack_byte);
        WriteByte(state, kTreeOffset + 8u, 0u);
        runtime.InitializeBehaviorTree(state, address + kTreeOffset);

        Write(state, 0x698u, 0x842d64u);
        Write(state, 0x6b0u, 0x842d58u);
        Write(state, 0x6b8u, 0x842d4cu);
        Write(state, 0x6c0u, 0x842d3cu);
        Write(state, 0x6c8u, 0x842d2cu);
        Write(state, 0x6d0u, 0x842e58u);
        stage = 3;
        Write(state, 0x6acu, 0x842e54u);
        Write(state, 0x6b0u, 0x842e48u);
        Write(state, 0x6b8u, 0x842e3cu);
        Write(state, 0x6c0u, 0x842e2cu);
        Write(state, 0x6c8u, 0x842e1cu);
        Write(state, 0x6d0u, 0x842e10u);
        CallbackStateWriter callback_writer(state, address);
        (void)GameManagerComponentCallback::Construct(
            address + kAuxiliaryOffset, callback_writer);
        stage = 4;

        Write(state, 0u, 0x842dc0u);
        Write(state, 0x170u, 0x842db0u);
        Write(state, 0x1ecu, 0x842da4u);
        Write(state, 0x204u, 0u);
        Write(state, 0x654u, 0u);
        Write(state, 0x6e4u, 0u);
        Write(state, 0x6e8u, 0u);
        Write(state, 0x6d4u, address);

        const auto tree = address + kTreeOffset;
        for (std::uint32_t i = 0u; i < 4u; ++i) {
            const auto class_id = runtime.ClassIdFromGetter(kBehaviorFactories[i].getter);
            const auto inserted = runtime.InsertBehavior(
                tree, class_id, kBehaviorFactories[i].factory);
            runtime.CopyInsertionResult(inserted);
        }
        const auto class_id = runtime.ClassIdFromGetter(kBehaviorFactories[4].getter);
        runtime.InsertFinalBehavior(tree, class_id, kBehaviorFactories[4].factory);
        return address;
    } catch (...) {
        if (stage >= 4)
            runtime.DestroyAuxiliary(state, address + kAuxiliaryOffset);
        if (stage >= 3)
            runtime.DestroyBehaviorTree(state, address + kTreeOffset);
        if (stage >= 2)
            runtime.CleanupScriptObject(state, address + kScriptObjectOffset);
        if (stage >= 1)
            runtime.CleanupEventProcessor(state, address + kEventProcessorOffset);
        if (stage >= 0)
            runtime.DestroyBase(state, address);
        throw;
    }
}

void GameManagerComponents::Cleanup(
    GameManagerComponentState& state, std::uint32_t address,
    IGameManagerComponentRuntime& runtime) {
    Write(state, 0u, 0x842dc0u);
    Write(state, 0x170u, 0x842db0u);
    Write(state, 0x1ecu, 0x842da4u);
    runtime.DestroyAuxiliary(state, address + kAuxiliaryOffset);

    auto tree = LoadBehaviorTree(state);
    (void)MonsterBehaviorTree::Destroy(
        tree, address + 0x698u, runtime.TreeRuntime());
    StoreBehaviorTree(state, tree);

    runtime.CleanupScriptObject(state, address + kScriptObjectOffset);
    runtime.CleanupEventProcessor(state, address + kEventProcessorOffset);
    runtime.DestroyBase(state, address);
}

GameManagerBehaviorInsertResult& GameManagerComponents::InsertBehaviorNode(
    GameManagerComponentState& state, std::int32_t class_id,
    std::uint32_t factory, GameManagerBehaviorInsertResult& output,
    IGameManagerComponentRuntime& runtime) {
    auto tree = LoadBehaviorTree(state);
    const auto result = MonsterBehaviorTree::Insert(
        tree, class_id, factory, runtime.TreeRuntime());
    StoreBehaviorTree(state, tree);
    output.node = result.node;
    output.inserted = result.inserted;
    return output;
}

}  // namespace pal4
