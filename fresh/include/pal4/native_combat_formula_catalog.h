#pragma once
#include "pal4/game_data_database_header.h"
#include <string>
#include <vector>

namespace pal4 {
class NativeGameDatabase;
// Copies original CombatFormulaConfig through the shared32-key loader.
class NativeCombatFormulaCatalog final {
public:
    explicit NativeCombatFormulaCatalog(const NativeGameDatabase&);
    [[nodiscard]] float FloatAt(std::int32_t index) const;
    [[nodiscard]] const std::vector<std::string>& Warnings() const { return warnings_; }
private:
    GameDataDatabaseHeader settings_;
    std::vector<std::string> warnings_;
};
}
