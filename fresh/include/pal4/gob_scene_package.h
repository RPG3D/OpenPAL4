#pragma once

#include <string_view>

namespace pal4 {

template <typename Runtime>
void SelectGobScenePackage(std::string_view path, Runtime& runtime) {
    if (runtime.UsesObjectPackage(path)) {
        runtime.LoadObjectPackage();
    } else if (runtime.UsesWeaponPackage(path)) {
        runtime.LoadWeaponPackage();
    } else {
        const auto package = runtime.CurrentScenePackage();
        if (package != 0u)
            runtime.LoadScenePackage(package);
    }
}

}  // namespace pal4
