#pragma once
#include "pal4/native_scene_gob_assets.h"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace pal4::test {
// Use the actual harbor records: its animated Machine reaches service creation
// after clip binding. A failure there must release borrowers before the image.
inline void VerifyNativeGobStagedLoadFailure(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms,NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema,NativePkFileManagerRuntime& packages,
    INativeGobEffectService& effects) {
    const auto before=memory.AllocationAddresses();
    unsigned failures{};
    for(unsigned attempt=0;attempt<2u;++attempt){
        NativeSceneGobAssets objects;
        objects.BindMachineServices([]()->NativeGobMachineServices{
            throw std::runtime_error("staged harbor Machine service fixture failure");
        });
        try {
            (void)objects.Load(root,"Q04","Q04HY",transforms,memory,schema,packages,effects);
            throw std::logic_error("harbor failed-load fixture did not reach Machine construction");
        } catch(const std::runtime_error& error) {
            if(std::string_view(error.what())!="staged harbor Machine service fixture failure")throw;
            ++failures;
        }
        objects.Clear();
        if(objects.Size()!=0u || objects.AnimationBindingCount()!=0u ||
           memory.AllocationAddresses()!=before)
            throw std::logic_error("staged harbor GOB failure retains owned records or clips");
    }
    if(failures!=2u)throw std::logic_error("harbor load failure was not exercised twice");
    std::cout<<"Native staged GOB load: actual Q04HY animated Machine service failure twice, original exception retained, borrowers retire before object/clump, zero native allocation delta\n";
}
}
