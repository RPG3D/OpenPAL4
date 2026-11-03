#include "pal4/native_combat_state_owner.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_pal_game_iv_state_list.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
int main() {
    try {
        pal4::NativeBaseObjectMemory memory;
        pal4::NativeEventDocumentRuntime docs(memory);
        pal4::NativeCombatStateOwner owner(memory, docs);
        Check(!owner.Address(), "Combat state starts unconstructed");
        bool rejected = false;
        try { (void)owner.Document(); } catch (const std::logic_error&) { rejected = true; }
        Check(rejected, "cold state cannot expose a document");
        const auto address = owner.Construct();
        Check(memory.ReadDword(address) == 0x849480u && owner.Document() == address + 12u &&
                  docs.DocumentCount() == 1u && docs.NodeName(owner.Document()).empty() &&
                  memory.ReadDword(owner.Document() + 4u) == 0u &&
                  memory.ReadDword(owner.Document() + 8u) == 0u &&
                  memory.ReadDword(owner.Document() + 12u) == 0u,
              "shared original constructor creates embedded XML at Combat+12");
        rejected = false;
        try { (void)owner.Construct(); } catch (const std::logic_error&) { rejected = true; }
        Check(rejected && docs.DocumentCount() == 1u, "duplicate construction preserves live document");
        std::array<std::uint32_t, 128> game{};
        pal4::NativePalGameIVStateList states(reinterpret_cast<std::uint32_t>(game.data()));
        Check(states.Register(address, 3u, [&] { owner.Destroy(); }) &&
                  states.Find(3u) == address && states.StateId(address) == 3u,
              "actual native state registry resolves persistent Combat state three");
        memory.WriteDword(address + 4u, 71u);
        memory.WriteDword(address + 8u, 72u);
        for (unsigned cycle = 0; cycle < 3; ++cycle) {
            const auto source = docs.Allocate(60u);
            (void)docs.InitializeDocument(source, "CombatParam", 0u);
            docs.WriteStringAttribute(source, "CombatScene", "CombatWorld");
            docs.WriteStringAttribute(source, "CombatArea", cycle == 1u ? "M02" : "M01");
            for (unsigned index = 0; index < 3; ++index) {
                const auto monster = docs.CreateChild(source, "CbMonster");
                docs.WriteIntAttribute(monster, "MonsterId", index == 0u ? 2579 : 2580);
                docs.WriteIntAttribute(monster, "CombatPosition", index == 0u ? 1 : index == 1u ? 0 : 2);
            }
            owner.AssignRequest(source);
            Check(docs.ChildAt(source, 0u) != docs.ChildAt(owner.Document(), 0u),
                  "Combat state owns independent child nodes");
            docs.DestroyDocument(source);
            (void)docs.Free(source);
            owner.AssignRequest(owner.Document());
            Check(docs.DocumentCount() == 4u && docs.ChildCount(owner.Document()) == 3u &&
                      docs.NodeName(owner.Document()) == "CombatParam" &&
                      docs.AttributeToString(docs.FindNumericAttribute(owner.Document(), "CombatArea")) ==
                          (cycle == 1u ? "M02" : "M01") &&
                      memory.ReadDword(address + 4u) == 71u && memory.ReadDword(address + 8u) == 72u,
                  "temporary root retirement and repeated replacement preserve owned XML and saved flags");
        }
        states.Clear();
        owner.Destroy();
        Check(!owner.Address() && states.Count() == 0u && !states.Find(3u) &&
                  states.OutstandingAllocations() == 0u && docs.DocumentCount() == 0u &&
                  memory.OutstandingAllocations() == 0u,
              "registry teardown retires embedded XML and all native allocations exactly once");
        (void)owner.Construct();
        owner.Destroy();
        Check(docs.DocumentCount() == 0u && memory.OutstandingAllocations() == 0u,
              "state reconstruction after teardown balances native XML globals");
        std::cout << "Native Combat owner: original embedded XML, real registry, independent request lifetime, "
                     "replacement, prefix preservation and zero final allocations verified\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
