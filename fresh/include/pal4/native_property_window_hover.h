#pragma once
#include "pal4/native_inventory_property_reader.h"
#include "pal4/property_window_lifecycle.h"
#include <windows.h>
namespace pal4 {
// Resolve actual DLL hover ID through the live inventory, without retaining a
// borrowed ListboxItem or dereferencing its former inventory entry.
std::optional<NativeInventoryProperty> ResolveNativePropertyHover(
    HMODULE,const PropertyWindowState&,NativeInventoryPropertyReader&);
}
