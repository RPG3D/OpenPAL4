#pragma once
#include "pal4/native_scene_gob_assets.h"
#include "pal4/native_event_document_runtime.h"
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeGobRetainedConfig(NativeSceneGobAssets& objects) {
    const auto check=[](bool ok) {if(!ok)throw std::runtime_error("retained native GOB config consumer assertion");};
    unsigned count{};bool base_checked{},machine_checked{};
    for(const auto address:objects.ObjectAddresses()) {
        auto* object=objects.FindMutableObjectByAddress(address);check(object!=nullptr);
        auto& docs=object->ConfigurationDocuments();const auto root=object->ConfigurationAddress();
        check(root && *reinterpret_cast<const std::uint32_t*>(address+0x1c0u)==root &&
            docs.NodeName(root)=="PAL4-GameObject");++count;
        const auto* record=objects.FindRecord(reinterpret_cast<const char*>(*reinterpret_cast<const std::uint32_t*>(address+4u)));
        check(record!=nullptr);
        if(!base_checked && object->Type()==0u) {
            const auto attr=docs.FindNumericAttribute(root,"PAL4-GameObject-object-hint");check(attr!=0u);
            const auto old=docs.AttributeToInt(attr,-1);const auto applied=*reinterpret_cast<const std::int32_t*>(address+0x1bcu);
            const auto source=record->config.Root().FindAttribute("PAL4-GameObject-object-hint");check(source!=nullptr);
            docs.SetAttributeIntValue(attr,12345);object->ReloadConfiguration(*record);
            check(object->ConfigurationAddress()==root && *reinterpret_cast<const std::int32_t*>(address+0x1bcu)==12345 &&
                source->ToInt(0u)==static_cast<std::uint32_t>(old));
            docs.SetAttributeIntValue(attr,old);object->ReloadConfiguration(*record);
            check(*reinterpret_cast<const std::int32_t*>(address+0x1bcu)==applied);base_checked=true;
        }
        if(!machine_checked && object->Type()==5u) {
            const auto parameters=docs.FindChild(root,"PAL4-GameObject-Parameters");
            const auto machine=docs.FindChild(parameters,"PAL4_GameObject-machine");check(machine!=0u);
            const auto attr=docs.FindNumericAttribute(machine,"PAL4_GameObject-machine-repeat");check(attr!=0u);
            const auto old=docs.AttributeToInt(attr,0);
            docs.SetAttributeIntValue(attr,old^1);object->ReloadConfiguration(*record);
            check(*reinterpret_cast<const std::int32_t*>(address+0x280u)==(old^1));
            docs.SetAttributeIntValue(attr,old);object->ReloadConfiguration(*record);
            check(*reinterpret_cast<const std::int32_t*>(address+0x280u)==old);machine_checked=true;
        }
    }
    check(count==objects.Size() && base_checked && machine_checked);
    std::cout<<"Retained GOB XML: "<<count<<" live native slots; base hint and Machine repeat reset consume mutated owned XML while catalog remains unchanged\n";
}
}
