#include "pal4/native_pal_game_object_manager_owner.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::NativeBaseObjectMemory memory;
    const auto before = memory.OutstandingAllocations();
    {
        pal4::NativePalGameObjectManagerOwner manager(memory);
        const auto address = manager.Address();
        Check(address != 0u &&
                  memory.ReadDword(address + 0x10u) ==
                      pal4::PalGameObjectManager::kVtable &&
                  memory.ReadDword(address + 4u) != 0u &&
                  memory.ReadDword(address + 0x18u) != 0u &&
                  manager.ObjectCount() == 0u && manager.FactoryCount() == 0u,
              "original GOB manager owns both embedded trees");
        manager.RegisterOriginalFactories();
        Check(manager.FactoryCount() == 11u &&
                  memory.ReadDword(address + 0x20u) == 11u &&
                  manager.FactoryForType(0) == 0x5a42e0u &&
                  manager.FactoryForType(8) == 0x59c460u &&
                  manager.FactoryForType(11) == 0u,
              "original GOB factory tree resolves normal and effect types");
        Check(manager.InsertObject("MO001", 0x1234u) &&
                  manager.InsertObject("effect001", 0x5678u) &&
                  !manager.InsertObject("MO001", 0x9999u) &&
                  manager.ObjectCount() == 2u &&
                  memory.ReadDword(address + 12u) == 2u &&
                  manager.FindObject("MO001") == 0x1234u &&
                  manager.FindObject("effect001") == 0x5678u &&
                  manager.FindObject("absent") == 0u,
              "original GOB object tree owns named objects");
        const auto header=memory.ReadDword(address+4u);
        const auto factory_header=memory.ReadDword(address+0x18u);
        bool missing_rejected=false;
        try { manager.CleanupScene({},{}); }
        catch(const std::invalid_argument&) { missing_rejected=true; }
        Check(missing_rejected && manager.ObjectCount()==2u,
              "populated GOB cleanup rejects missing services before mutation");
        std::vector<std::uint32_t> calls;
        manager.CleanupScene([&](std::uint32_t object) {
            bool recursive_rejected=false, insertion_rejected=false;
            try { manager.CleanupScene({},{}); }
            catch(const std::logic_error&) { recursive_rejected=true; }
            try { (void)manager.InsertObject("during-cleanup",0x9999u); }
            catch(const std::logic_error&) { insertion_rejected=true; }
            Check(recursive_rejected && insertion_rejected && manager.ObjectCount()==2u,
                  "GOB notification preserves tree traversal and rejects reentrancy");
            calls.push_back(object);
        },[&](std::uint32_t object,std::uint32_t mode) {
            Check(mode==1u && calls.back()==object,"GOB notification precedes deleting destructor mode1");
            calls.push_back(object+1u);
        });
        Check(calls==std::vector<std::uint32_t>{0x1234u,0x1235u,0x5678u,0x5679u} &&
                  manager.Address()==address && memory.ReadDword(address+4u)==header &&
                  memory.ReadDword(address+0x18u)==factory_header && manager.ObjectCount()==0u &&
                  manager.FactoryCount()==11u && manager.FactoryForType(8)==0x59c460u &&
                  manager.FindObject("MO001")==0u,
              "GOB scene cleanup retains manager, empty header and original factories");
        const auto retained_allocations=memory.OutstandingAllocations();
        manager.CleanupScene({},{});
        unsigned visited=0u;
        manager.VisitObjects([&](std::uint32_t) { ++visited; return true; });
        Check(!visited && memory.OutstandingAllocations()==retained_allocations &&
                  manager.InsertObject("MO001",0x9999u) && manager.FindObject("MO001")==0x9999u,
              "cleaned GOB manager supports empty Scene activation and subsequent same-name registration");
    }
    Check(memory.OutstandingAllocations() == before,
          "GOB manager releases both trees, nodes and keys");
}
