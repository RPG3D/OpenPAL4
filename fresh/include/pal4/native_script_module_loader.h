#pragma once
#include <filesystem>
#include <string_view>
namespace pal4 {
class NativeAngelScriptEngine;
// Accept script.cpk member names or original gamedata\EditData\script\ paths.
// The module name remains the caller's SDK context/cache identity.
void LoadNativeScriptModule(NativeAngelScriptEngine&, const std::filesystem::path&,
                            std::string_view member, std::string_view module);
}  // namespace pal4
