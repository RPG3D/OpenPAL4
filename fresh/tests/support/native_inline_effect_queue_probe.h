#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/native_effect_object_queues.h"
#include "pal4/native_effect_tree_storage.h"
#include "pal4/effect_system_tree_update.h"
#include <array>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void VerifyNativeInlineEffectQueues() {
    const auto check=[](bool value) {
        if(!value) throw std::runtime_error("native inline effect queue assertion");
    };
    NativeBaseObjectMemory memory;
    std::array<std::uint32_t,65> group{};
    const auto g=reinterpret_cast<std::uint32_t>(group.data());
    NativeEffectTreeStorage trees(memory);
    trees.InitializeInline(g+212u);
    struct Classification final : INativeEffectObjectClassification {
        std::string_view ObjectTypeName(std::uint32_t) override{return "EffectObject";}
        bool UseFallbackQueue(std::uint32_t) override{return false;}
        bool UseCameraQueue(std::uint32_t) override{return false;}
    } classification;
    {
        NativeEffectObjectQueues queues(memory,g,classification);
        const auto baseline=memory.OutstandingAllocations();
        const auto head=group[54];
        // These are opaque keys, never dereferenced as payloads by this probe.
        constexpr std::array<std::uint32_t,7> keys{
            0x80000001u,0x100u,0xfffffff0u,0x400u,0x200u,0x700u,0x500u};
        queues.BindInlineTree(trees);
        queues.BindInlineTree(trees);
        queues.QueueEffect(g,keys[0]); // disabled group must not insert
        check(!group[56] && trees.FindInline(g+212u,keys[0])==head);
        memory.WriteByte(g+4u,1u);
        for(const auto key:keys) queues.QueueEffect(g,key);
        check(group[56]==keys.size() && memory.OutstandingAllocations()==baseline+keys.size());
        for(const auto key:keys) {
            const auto node=trees.FindInline(g+212u,key);
            check(node!=head && memory.ReadDword(node+12u)==key && memory.ReadByte(node+16u)==1u);
        }
        queues.QueueEffect(g,keys[0]);
        check(group[56]==keys.size()); // unique-key tree, unlike ordinary lists
        queues.RemoveEffect(g,keys[1]);
        queues.RemoveEffect(g,0x123456u); // missing key leaves storage untouched
        const auto removed=trees.FindInline(g+212u,keys[1]);
        check(removed!=head && !memory.ReadByte(removed+16u) && group[56]==keys.size());
        queues.QueueEffect(g,keys[1]);
        check(!memory.ReadByte(removed+16u)); // duplicate does not reactivate pending removal
        struct Update final : IEffectSystemTreeUpdateRuntime {
            NativeBaseObjectMemory& memory;
            NativeEffectTreeStorage& trees;
            std::vector<std::uint32_t> visited;
            Update(NativeBaseObjectMemory& m,NativeEffectTreeStorage& t):memory(m),trees(t){}
            std::uint32_t ReadWord(std::uint32_t a) override{return memory.ReadDword(a);}
            std::uint8_t ReadByte(std::uint32_t a) override{return memory.ReadByte(a);}
            std::uint32_t SharedNil(std::uint32_t) override{return trees.InlineNil();}
            std::uint32_t RemoveInactiveInline(std::uint32_t t,std::uint32_t n) override{return trees.EraseInline(t,n);}
            void UpdateXmlObject(std::uint32_t,std::uint32_t,std::uint32_t) override {
                throw std::logic_error("inline probe must not dispatch XML objects");
            }
            void UpdateInlineObject(std::uint32_t key,std::uint32_t first,std::uint32_t second) override {
                if(first!=0x1234u || second!=0x5678u) throw std::runtime_error("inline frame arguments changed");
                visited.push_back(key);
            }
        } update(memory,trees);
        EffectSystem::UpdateInlineEffects(g,0x1234u,0x5678u,update);
        check(update.visited==std::vector<std::uint32_t>{0x200u,0x400u,0x500u,0x700u,0x80000001u,0xfffffff0u});
        check(group[56]==6u && trees.FindInline(g+212u,keys[1])==head);
        queues.QueueEffect(g,keys[1]);
        check(group[56]==7u && memory.ReadByte(trees.FindInline(g+212u,keys[1])+16u)==1u);
        for(const auto key:keys) queues.RemoveEffect(g,key);
        memory.WriteByte(g+4u,0u);
        EffectSystem::UpdateInlineEffects(g,0x1234u,0x5678u,update);
        check(group[56]==7u); // disabled cleanup must not free pending nodes
        // Explicit pruning is also used during shutdown after disabling the group.
        // IDA631880 has no enabled guard, unlike the normal update entry.
        (void)EffectSystem::ProcessEffectList(g,update);
        check(!group[56] && memory.OutstandingAllocations()==baseline &&
              memory.ReadDword(head)==head && memory.ReadDword(head+8u)==head);
    }
    trees.CleanupInline(g+212u);
    check(!trees.InlineNil() && !memory.OutstandingAllocations());
}
} // namespace pal4::test
