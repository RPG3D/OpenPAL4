#pragma once

#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_gob_clump_asset.h"
#include "pal4/native_gob_animation_binding.h"
#include "pal4/gob_machine.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace pal4::test {

inline void VerifyNativeM20MissingAnimation(const std::filesystem::path& root,
    IGameResourceManagerRuntime& transforms, NativeBaseObjectMemory& memory,
    IRwDictSchemaRuntime& schema) {
    const auto check = [](bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    };
    NativeSceneGobCatalog catalog;
    check(catalog.Load(root, "M20", "1Y"), "M20 night real GOB catalog");
    const auto record = std::find_if(catalog.Records().begin(), catalog.Records().end(),
        [](const auto& entry) { return entry.name == "MO002"; });
    check(record != catalog.Records().end() && record->type == GobMachine::TypeKey(),
          "M20 MO002 is the original Machine5");
    const auto allocated = memory.OutstandingAllocations();
    {
        NativeGobClumpAsset model;
        check(model.Load(root, record->resource_dir, record->model), "M20 MO002 real clump");
        NativeGobObjectImage object(memory, schema);
        (void)object.Construct(record->type);
        object.BindRecord(*record, reinterpret_cast<std::uint32_t>(model.Clump()), transforms);
        NativeGobAnimationBinding binding(object, model, memory);
        check(binding.Load(record->model) && binding.Bound() && binding.LastError().empty() &&
              !binding.AnimationLoadError().empty(), "missing night ANM is nonfatal and diagnostic");
        const auto state = object.Address() + GobMachine::kStateOffset;
        const auto word = [&](unsigned index) { return memory.ReadDword(state + index * 4u); };
        const auto records = word(2);
        check(records && memory.ReadDword(records - 4u) == 1u &&
              !memory.ReadDword(records + 32u) && !word(1) && !word(3) && !word(17) &&
              !word(18) && word(19) == 1u && word(22) - word(21) == 32u,
              "M20 missing ANM retains original partial array, TAB manager and pause/restart flags");
        check(!binding.Select("A") && binding.Length() == 0.f && binding.Time() == 0.f &&
              !binding.SetTime(0.f) && !binding.AdvanceFrame(1.f / 60.f),
              "partial animation safely follows shared playback guards");
        check(model.Clump() && object.Bound() && object.AnimationService() == &binding,
              "M20 missing ANM retains static model and scene object");
        binding.Release();
        check(!binding.Bound() && !object.AnimationService() && !word(2) && !word(21),
              "partial animation release detaches service and clears owned records");
        binding.Release();
    }
    check(memory.OutstandingAllocations() == allocated, "M20 partial animation has no owner leaks");
    std::cout << "Native M20 missing animation: original MO002 night resources, partial records, safe playback, retained model and cleanup pass\n";
}

}  // namespace pal4::test
