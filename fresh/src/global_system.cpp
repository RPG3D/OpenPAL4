#include "pal4/global_system.h"

namespace pal4 {

void GlobalSystemRuntimeStub::RegisterCleanup() {}
void GlobalSystemRuntimeStub::SetErrorFlag(GlobalSystemText&,
                                           std::uint8_t) {}
std::uint32_t GlobalSystemRuntimeStub::AllocateEntry(std::uint32_t) {
    return 0u;
}
void GlobalSystemRuntimeStub::WriteDword(std::uint32_t,
                                          std::uint32_t) {}
void GlobalSystemRuntimeStub::InitializeEntryText(std::uint32_t,
                                                   std::uint8_t) {}
void GlobalSystemRuntimeStub::SetText(GlobalSystemText&,
                                      std::string_view) {}
void GlobalSystemRuntimeStub::AssignSourceText(GlobalSystemText&,
                                               std::uint32_t) {}
void GlobalSystemRuntimeStub::AppendEmptyErrorString(GlobalSystemText&) {}
void GlobalSystemRuntimeStub::AssignEntryText(
    std::uint32_t, const GlobalSystemText&) {}
void GlobalSystemRuntimeStub::PushBack(GlobalSystemCollection&,
                                       std::uint32_t) {}
std::uint32_t GlobalSystemRuntimeStub::GameDataCollection(
    std::uint32_t) { return 0u; }
std::uint32_t GlobalSystemRuntimeStub::GameDataObjectCollection(
    std::uint32_t) { return 0u; }
std::uint32_t GlobalSystemRuntimeStub::ScriptEventCollection(
    std::uint32_t) { return 0u; }
void GlobalSystemRuntimeStub::ProcessAll(GlobalSystemCollection&,
                                         std::uint32_t) {}
void GlobalSystemRuntimeStub::Sort(GlobalSystemCollection&) {}
void GlobalSystemRuntimeStub::ReleaseText(GlobalSystemText&) {}
void GlobalSystemRuntimeStub::DestroyUnlinkedEntry(std::uint32_t) {}
void GlobalSystemRuntimeStub::CleanupCollection(GlobalSystemCollection&) {}

namespace {

void InitializeBuffer(GlobalSystemBuffer& buffer, std::uint8_t first_byte) {
    buffer.marker = (buffer.marker & 0xffffff00u) | first_byte;
    buffer.begin = 0u;
    buffer.end = 0u;
    buffer.capacity_end = 0u;
}

template <typename PrepareText>
void AddEntry(GlobalSystemCollection& collection,
              GlobalSystemText& text, std::uint32_t id,
              std::uint8_t incoming_byte, IGlobalSystemRuntime& runtime,
              PrepareText prepare_text) {
    const auto entry = runtime.AllocateEntry(0x18u);
    bool linked = false;
    try {
        runtime.WriteDword(entry, GlobalSystem::kBaseVtable);
        runtime.InitializeEntryText(entry + 8u, incoming_byte);
        runtime.WriteDword(entry, GlobalSystem::kEntryVtable);
        prepare_text();
        runtime.AssignEntryText(entry + 8u, text);
        runtime.WriteDword(entry + 4u, id);
        runtime.PushBack(collection, entry);
        linked = true;
    } catch (...) {
        if (!linked && entry != 0u)
            runtime.DestroyUnlinkedEntry(entry);
        throw;
    }
}

}  // namespace

GlobalSystemCollection& GlobalSystem::Initialize(
    std::uint8_t first_stack_byte, std::uint8_t second_stack_byte,
    IGlobalSystemRuntime& runtime) {
    if ((construction_guard_ & 1u) == 0u) {
        construction_guard_ |= 1u;
        InitializeBuffer(collection_.entries, first_stack_byte);
        InitializeBuffer(collection_.processed, first_stack_byte);
        InitializeBuffer(collection_.sorted, second_stack_byte);
        runtime.RegisterCleanup();
    }
    if (entries_guard_ != 0u)
        return collection_;

    entries_guard_ = 1u;
    GlobalSystemText text{};
    text.marker = second_stack_byte;
    runtime.SetErrorFlag(text, 0u);
    try {
        AddEntry(collection_, text, 2824u, second_stack_byte, runtime,
                 [&] {
                     runtime.SetText(text, "InTeam");
                     runtime.AppendEmptyErrorString(text);
                 });
        for (std::uint32_t id = 2796u; id < 2824u; id += 4u) {
            AddEntry(collection_, text, id, second_stack_byte, runtime,
                     [&] {
                         runtime.AssignSourceText(
                             text, kSourceTextArray + (id - 2796u) * 4u);
                         runtime.AppendEmptyErrorString(text);
                     });
        }
        runtime.ProcessAll(collection_, runtime.GameDataCollection(0x9f8u));
        runtime.ProcessAll(collection_,
                           runtime.GameDataObjectCollection(0x654u));
        runtime.ProcessAll(collection_, runtime.ScriptEventCollection(0x920u));
        runtime.Sort(collection_);
    } catch (...) {
        runtime.ReleaseText(text);
        throw;
    }
    runtime.ReleaseText(text);
    return collection_;
}

void GlobalSystem::Cleanup(IGlobalSystemRuntime& runtime) {
    runtime.CleanupCollection(collection_);
}

}  // namespace pal4
