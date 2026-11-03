#pragma once

#include "pal4/native_dialog_stop_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_script_object_runtime.h"
#include "pal4/native_effect_handle_content_runtime.h"
#include "pal4/native_player_transform_runtime.h"
#include "pal4/native_dialog_reset_runtime.h"
#include "pal4/native_effect_tree_storage.h"
#include "pal4/native_effect_owner_registry.h"
#include "pal4/native_effect_object_pool.h"
#include "native_dialog_preparation_fixture.h"

#include <rwcore.h>
#include <array>
#include <bit>
#include <memory>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeDialogStop() {
    const auto require = [](bool ok) {
        if (!ok) throw std::runtime_error("native dialog stop regression");
    };
    const auto address = [](const void* p) {
        return reinterpret_cast<std::uint32_t>(p);
    };
    NativeBaseObjectMemory memory;
    NativeScriptObjectRuntime tasks(memory);
    struct Task final : INativeScriptTaskLifecycle {
        NativeBaseObjectMemory& memory;
        NativeScriptObjectRuntime& tasks;
        unsigned deletes{};
        Task(NativeBaseObjectMemory& m, NativeScriptObjectRuntime& t)
            : memory(m), tasks(t) {}
        void Delete(std::uint32_t task, std::uint32_t flags) override {
            if (flags != 1u) throw std::runtime_error("dialog task delete flags");
            GomTaskBase base(tasks);
            (void)base.Destroy(*reinterpret_cast<GomTaskBaseState*>(task));
            (void)memory.Free(task);
            ++deletes;
        }
        void Begin(std::uint32_t) override { throw std::logic_error("unexpected task begin"); }
        void Tick(std::uint32_t, float) override { throw std::logic_error("unexpected task tick"); }
        bool IsDurationComplete(std::uint32_t) override { return false; }
        void Finish(std::uint32_t) override { throw std::logic_error("unexpected task finish"); }
    };
    struct Effects final : INativeDialogStopEffects, INativeDialogResetEffects {
        std::array<double, 2> values{2.25, 3.75};
        unsigned initialized{}, modes{}, measured{};
        unsigned resets{}, references{}, events{};
        bool EffectsEnabled() override { return true; }
        bool EffectIsActive(std::uint32_t) override { return false; }
        void ReleaseEffectReferences(std::uint32_t) override { ++references; }
        void ProcessEffectEvent(std::uint32_t, std::uint32_t) override { ++events; }
        void InitializeEffectSystem() override { ++initialized; }
        void SetEffectMode(std::uint32_t effect, std::uint32_t mode) override {
            if (mode == 0u) { ++resets; return; }
            if (mode != 1u ||
                (effect != reinterpret_cast<std::uint32_t>(&values[0]) &&
                 effect != reinterpret_cast<std::uint32_t>(&values[1])))
                throw std::runtime_error("dialog effect mode");
            ++modes;
        }
        double EffectValue(std::uint32_t effect) override {
            ++measured;
            return *reinterpret_cast<const double*>(effect);
        }
    } effects;
    NativeDialogStopRuntime runtime(memory, tasks, effects);
    NativeDialogResetRuntime reset_runtime(memory, tasks, effects);
    std::array<DialogObjectState, 2> dialogs{};
    std::array<std::shared_ptr<Task>, 2> owners;
    for (unsigned index = 0; index < dialogs.size(); ++index) {
        const auto container = memory.Allocate(sizeof(GomTaskContainerState));
        tasks.InitializeTaskContainer(container);
        dialogs[index].words[2] = container;
        const auto task = memory.Allocate(sizeof(GomTaskBaseState));
        GomTaskBase base(tasks);
        (void)base.Construct(*reinterpret_cast<GomTaskBaseState*>(task));
        owners[index] = std::make_shared<Task>(memory, tasks);
        tasks.RegisterTask(task, owners[index]);
        tasks.AddTask(container, task, "dialog-stop-task", {});
    }
    auto* parent = RwFrameCreate();
    auto* frame = RwFrameCreate();
    require(parent && frame && RwFrameAddChild(parent, frame));
    dialogs[0].words[43] = address(frame);
    std::array<std::uint32_t, 4> slots{11u, 123u, 22u, 456u};
    dialogs[0].words[24] = address(slots.data());
    dialogs[0].words[25] = address(slots.data() + slots.size());
    std::array<std::uint32_t, 15> records{};
    records[4] = address(&effects.values[0]);
    records[14] = address(&effects.values[1]);
    dialogs[0].words[28] = address(records.data());
    dialogs[0].words[29] = address(records.data() + records.size());
    const auto before = memory.OutstandingAllocations();
    require(DialogObject::StopPlayback(dialogs[0], true, runtime) == 3.75);
    require(dialogs[0].words[2] == 0u && dialogs[0].words[43] == address(frame) &&
            !RwFrameGetParent(frame) && tasks.RegisteredTasks() == 1u &&
            owners[0]->deletes == 1u && owners[1]->deletes == 0u &&
            memory.OutstandingAllocations() < before &&
            slots == std::array<std::uint32_t,4>{11u, 0u, 22u, 0u} &&
            effects.initialized == 2u && effects.modes == 2u && effects.measured == 2u);
    const auto remaining = memory.OutstandingAllocations();
    require(DialogObject::StopPlayback(dialogs[0], true, runtime) == 0.0 &&
            memory.OutstandingAllocations() == remaining && effects.measured == 2u);
    require(RwFrameAddChild(parent, frame) != nullptr);
    dialogs[1].words[43] = address(frame);
    require(DialogObject::StopPlayback(dialogs[1], false, runtime) == 0.0 &&
            RwFrameGetParent(frame) == parent && tasks.RegisteredTasks() == 0u &&
            owners[1]->deletes == 1u && memory.OutstandingAllocations() == 0u &&
            effects.measured == 2u);
    require(RwFrameRemoveChild(frame) && RwFrameDestroy(frame) && RwFrameDestroy(parent));

    // Real native tree images exercise the complete shared handle-retirement
    // path, with a real native owner registry and queue storage. Effect creation,
    // effect dispatch and owner notifications remain separate responsibilities.
    std::array<std::uint32_t,42> system{};
    std::array<std::uint32_t,64> owner{};
    const auto system_address = address(system.data());
    const auto owner_address = address(owner.data());
    system[0] = 1u;
    auto registry = std::make_unique<NativeEffectOwnerRegistry>(memory,system_address);
    require(system[40] == 0u && !registry->Contains(owner_address));
    require(registry->Register(owner_address) && !registry->Register(owner_address) &&
            registry->Contains(owner_address) && system[40] == 1u);
    // Unsigned ordering crosses the sign boundary; keys are borrowed numeric
    // addresses and must never be dereferenced by registration/teardown.
    for (auto key : {0u,0xffffffffu,0x80000000u,0x7fffffffu,12u,9u,15u})
        require(registry->Register(key) && registry->Contains(key));
    for (auto key : {12u,0x80000000u,0u,15u,0xffffffffu,9u,0x7fffffffu}) {
        registry->Unregister(key);
        require(!registry->Contains(key) && registry->Contains(owner_address));
    }
    registry->Unregister(owner_address+4u);
    require(system[40] == 1u);
    const auto registry_allocations = memory.OutstandingAllocations();
    require(registry_allocations == 3u);
    NativeEffectTreeStorage tree_storage(memory);
    tree_storage.InitializeActive(owner_address+12u);
    tree_storage.InitializePending(owner_address+28u);
    const auto active_nil = tree_storage.ActiveNil();
    const auto pending_nil = tree_storage.PendingNil();
    const auto active_head = owner[4];
    const auto pending_head = owner[8];
    require(active_nil != pending_nil && memory.ReadDword(active_nil+24u) == 1u &&
            memory.ReadDword(pending_nil+20u) == 1u);
    std::array<std::uint32_t,12> other_owner{};
    const auto other_address = address(other_owner.data());
    tree_storage.InitializeActive(other_address+12u);
    tree_storage.InitializePending(other_address+28u);
    require(memory.ReadDword(other_owner[4]+4u) == active_nil &&
            memory.ReadDword(other_owner[8]+4u) == pending_nil &&
            memory.OutstandingAllocations() == registry_allocations+6u);
    struct OwnerServices final : INativeEffectHandleOwnerServices {
        std::uint32_t owner{};
        std::uint32_t pool{};
        NativeDialogResetRuntime* reset{};
        NativeEffectObjectPool* pool_runtime{};
        unsigned released{};
        std::vector<std::uint32_t> returned;
        std::vector<std::uint32_t> reset_dialogs;
        std::string ReadNameAttribute(std::uint32_t) override {
            throw std::logic_error("retirement does not read effect names");
        }
        void ResetDialogRuntimeState(std::uint32_t dialog) override {
            (void)DialogObject::ResetRuntimeState(*reinterpret_cast<DialogObjectState*>(dialog),*reset);
            reset_dialogs.push_back(dialog);
        }
        void DispatchOwnerRelease(std::uint32_t target, EffectSystemHandleWords handle,
            std::uint32_t mode) override {
            if (target != owner || handle.system != owner || handle.id != 12u || mode != 0u)
                throw std::runtime_error("native handle owner notification");
            ++released;
        }
        void ReturnEffect(std::uint32_t target, std::uint32_t dialog, std::uint32_t mode) override {
            if (target != pool || mode != 0u || reset_dialogs.empty() || reset_dialogs.back() != dialog)
                throw std::runtime_error("pending Dialog resets before returning to its pool");
            if (!pool_runtime) throw std::logic_error("native pending object pool is required");
            pool_runtime->Return(dialog,static_cast<std::uint8_t>(mode));
            returned.push_back(dialog);
        }
        void ProcessEffectList(std::uint32_t) override {
            throw std::logic_error("content retirement queues the dialog directly");
        }
    } host;
    host.owner = owner_address;
    host.pool = system_address+84u;
    host.reset = &reset_runtime;
    NativePlayerTransformRuntime transforms;
    NativeEffectHandleContentRuntime content(memory,system_address,registry->Nil(),
        active_nil,pending_nil,runtime,transforms,host);
    require(content.ContainsOwner(system_address,owner_address) &&
            !content.ContainsOwner(system_address,owner_address+4u));
    std::array<DialogObjectState,3> pending_dialogs{};
    struct PoolObjects final : INativeEffectPoolObjects {
        std::array<DialogObjectState,3>& dialogs;
        NativeDialogPreparationFixture preparation;
        unsigned destroyed{};
        PoolObjects(std::array<DialogObjectState,3>& d, NativeBaseObjectMemory& memory)
            : dialogs(d), preparation(memory) {
            for (auto& dialog : dialogs) preparation.Initialize(dialog);
        }
        void ClearDialogFacets(std::uint32_t effect) override { preparation.ClearFacets(effect); }
        std::string ReadEffectName(std::uint32_t document) override {
            for (unsigned i=0;i<dialogs.size();++i)
                if (document==reinterpret_cast<std::uint32_t>(&dialogs[i])+20u)
                    return std::to_string(i);
            throw std::logic_error("pending pool unknown Dialog document");
        }
        std::uint32_t LoadEffect(std::uint32_t, std::string_view name) override {
            for (unsigned i=0;i<dialogs.size();++i)
                if (name==std::to_string(i)) return reinterpret_cast<std::uint32_t>(&dialogs[i]);
            throw std::logic_error("pending pool unknown effect name");
        }
        // Explicit payload doubles: these Dialog images are owned by this
        // fixture. The pool owns real cache/name nodes and performs SDK detach.
        void DestroyEffect(std::uint32_t effect) override {
            if (reinterpret_cast<DialogObjectState*>(effect)->words[2]!=0u)
                throw std::runtime_error("pending pool receives reset Dialog");
            preparation.Destroy(*reinterpret_cast<DialogObjectState*>(effect));
            ++destroyed;
        }
    } pool_objects(pending_dialogs,memory);
    auto native_pool=std::make_unique<NativeEffectObjectPool>(memory,system_address,pool_objects,pool_objects.preparation.documents);
    native_pool->Enable(32u);
    host.pool_runtime=native_pool.get();
    for (unsigned i=0;i<pending_dialogs.size();++i)
        require(native_pool->Acquire(std::to_string(i))==address(&pending_dialogs[i]));
    content.InsertPending(owner_address+28u,pending_nil,pending_head,
        address(&pending_dialogs[0]),1.25f);
    content.InsertPending(owner_address+28u,pending_nil,memory.ReadDword(pending_head+4u),
        address(&pending_dialogs[2]),9.25f);
    auto& middle = pending_dialogs[1];
    middle.words[2] = memory.Allocate(sizeof(GomTaskContainerState));
    tasks.InitializeTaskContainer(middle.words[2]);
    const auto task = memory.Allocate(sizeof(GomTaskBaseState));
    GomTaskBase base(tasks);
    (void)base.Construct(*reinterpret_cast<GomTaskBaseState*>(task));
    tasks.RegisterTask(task,owners[0]);
    tasks.AddTask(middle.words[2],task,"handle-retirement",{});
    middle.words[24] = address(slots.data());
    middle.words[25] = address(slots.data()+slots.size());
    middle.words[28] = address(records.data());
    middle.words[29] = address(records.data()+records.size());
    parent = RwFrameCreate(); frame = RwFrameCreate();
    require(parent && frame && RwFrameAddChild(parent,frame));
    RwV3d axis{0,0,1}, translation{100,200,300}, local{1,2,3};
    require(RwFrameRotate(parent,&axis,45.0f,rwCOMBINEREPLACE) &&
            RwFrameTranslate(parent,&translation,rwCOMBINEPOSTCONCAT) &&
            RwFrameTranslate(frame,&local,rwCOMBINEREPLACE));
    middle.words[43] = address(frame);
    const auto world_before = content.FrameTransform(address(frame));
    const auto activate = [&] {
        const auto node = memory.Allocate(28u);
        memory.WriteDword(node,active_nil); memory.WriteDword(node+4u,active_head);
        memory.WriteDword(node+8u,active_nil); memory.WriteDword(node+12u,12u);
        memory.WriteDword(node+16u,address(&middle)); memory.WriteDword(node+20u,1u);
        memory.WriteDword(node+24u,1u);
        memory.WriteDword(active_head,node); memory.WriteDword(active_head+4u,node);
        memory.WriteDword(active_head+8u,node); memory.WriteDword(owner_address+24u,1u);
    };
    activate();
    EffectSystemHandleWords handle{12u,owner_address};
    require(EffectHandle::DialogNeedsUpdate(handle,content));
    EffectHandle::UpdateContent(handle,content);
    require(handle.id == 0xffffffffu && handle.system == 0u && host.released == 1u &&
            middle.words[2] == 0u && !RwFrameGetParent(frame) &&
            tasks.RegisteredTasks() == 0u && owners[0]->deletes == 2u &&
            owner[6] == 0u && owner[10] == 3u);
    const auto world_after = content.FrameTransform(address(frame));
    for (unsigned index=0; index<world_before.size(); ++index)
        if (index%4u != 3u)
            require(std::fabs(world_before[index]-world_after[index]) < 0.0001f);
    const auto middle_node = [&] {
        auto node = memory.ReadDword(pending_head+4u);
        while (node != pending_nil) {
            const auto key = memory.ReadDword(node+12u);
            if (key == address(&middle)) return node;
            node = memory.ReadDword(node+(address(&middle)<key ? 0u : 8u));
        }
        return pending_nil;
    }();
    require(middle_node != pending_nil &&
            std::bit_cast<float>(memory.ReadDword(middle_node+16u)) == 3.75f);
    activate();
    handle = {12u,owner_address};
    EffectHandle::UpdateContent(handle,content);
    require(owner[6] == 0u && owner[10] == 3u && host.released == 2u &&
            std::bit_cast<float>(memory.ReadDword(middle_node+16u)) == 3.75f);
    system[0] = 0u;
    registry->Unregister(owner_address);
    require(!registry->Contains(owner_address) && !registry->Register(owner_address+4u) &&
            system[40] == 1u && memory.OutstandingAllocations() > registry_allocations);
    handle = {12u,owner_address};
    EffectHandle::UpdateContent(handle,content);
    require(handle.system == owner_address && host.released == 2u);
    system[0] = 1u;
    require(registry->Contains(owner_address));
    registry->Unregister(owner_address);
    require(!EffectHandle::DialogNeedsUpdate(handle,content));
    require(registry->Register(owner_address));
    content.UpdatePending(owner_address,1.25);
    require(owner[10] == 3u && host.returned.empty() &&
            std::bit_cast<float>(memory.ReadDword(middle_node+16u)) == 3.75f);
    owner[1] = 1u;
    content.UpdatePending(owner_address,1.25);
    require(owner[10] == 3u && host.returned.empty() &&
            std::bit_cast<float>(memory.ReadDword(middle_node+16u)) == 2.5f);
    content.UpdatePending(owner_address,2.5);
    require(owner[10] == 2u && host.returned == std::vector<std::uint32_t>{address(&pending_dialogs[0])} &&
            std::bit_cast<float>(memory.ReadDword(middle_node+16u)) == 0.0f);
    content.UpdatePending(owner_address,6.0);
    require(owner[10] == 1u && host.returned == (std::vector<std::uint32_t>{
                address(&pending_dialogs[0]),address(&pending_dialogs[1])}) &&
            effects.resets == 2u && effects.references == 2u && effects.events == 2u);
    content.UpdatePending(owner_address,0.0);
    require(owner[10] == 0u && memory.ReadDword(pending_head) == pending_head &&
            memory.ReadDword(pending_head+4u) == pending_nil &&
            memory.ReadDword(pending_head+8u) == pending_head &&
            host.returned == (std::vector<std::uint32_t>{address(&pending_dialogs[0]),
                address(&pending_dialogs[1]),address(&pending_dialogs[2])}) &&
            host.reset_dialogs == host.returned);
    content.UpdatePending(owner_address,0.25);
    require(host.returned.size() == 3u);

    // Runtime reset also destroys a live native container when called directly.
    auto& live_reset = pending_dialogs[0];
    live_reset.words[2] = memory.Allocate(sizeof(GomTaskContainerState));
    tasks.InitializeTaskContainer(live_reset.words[2]);
    const auto reset_task = memory.Allocate(sizeof(GomTaskBaseState));
    (void)base.Construct(*reinterpret_cast<GomTaskBaseState*>(reset_task));
    tasks.RegisterTask(reset_task,owners[0]);
    tasks.AddTask(live_reset.words[2],reset_task,"dialog-reset",{});
    live_reset.words[44] = 123u;
    live_reset.words[45] = 0u;
    (void)DialogObject::ResetRuntimeState(live_reset,reset_runtime);
    require(live_reset.words[2] == 0u && live_reset.words[44] == 0u &&
            live_reset.words[45] == std::bit_cast<std::uint32_t>(1.0f) &&
            tasks.RegisteredTasks() == 0u && owners[0]->deletes == 3u);
    require(system[26]==3u && system[30]==3u && system[34]==3u && pool_objects.destroyed==0u);
    native_pool.reset();
    pool_objects.preparation.Release();
    host.pool_runtime=nullptr;
    require(pool_objects.destroyed==3u && system[26]==0u && system[30]==0u && system[34]==0u);
    // Destroy a populated active tree without destroying its borrowed Dialog.
    activate();
    tree_storage.CleanupActive(owner_address+12u);
    tree_storage.CleanupPending(owner_address+28u);
    require(owner[4] == 0u && owner[8] == 0u && owner[6] == 0u && owner[10] == 0u &&
            tree_storage.ActiveNil() == active_nil && tree_storage.PendingNil() == pending_nil &&
            memory.ReadDword(other_owner[4]+4u) == active_nil &&
            memory.ReadDword(other_owner[8]+4u) == pending_nil &&
            memory.OutstandingAllocations() == registry_allocations+4u && host.returned.size() == 3u);
    tree_storage.CleanupActive(other_address+12u);
    require(tree_storage.ActiveNil() == 0u && tree_storage.PendingNil() == pending_nil &&
            memory.OutstandingAllocations() == registry_allocations+2u);
    content.InsertPending(other_address+28u,pending_nil,other_owner[8],
        address(&pending_dialogs[0]),5.0f);
    require(other_owner[10] == 1u && memory.OutstandingAllocations() == registry_allocations+3u);
    tree_storage.CleanupPending(other_address+28u);
    require(tree_storage.PendingNil() == 0u && other_owner[8] == 0u &&
            other_owner[10] == 0u && host.returned.size() == 3u &&
            owners[0]->deletes == 3u);
    // Disabled singleton teardown still releases a populated registry and leaves
    // the borrowed owner image alive, without calling its lifecycle callbacks.
    system[0] = 0u;
    registry.reset();
    require(system[38] == 0u && system[40] == 0u && owners[0]->deletes == 3u);
    require(RwFrameDestroy(frame) && RwFrameDestroy(parent) &&
            memory.OutstandingAllocations() == 0u);
}
}  // namespace pal4::test
