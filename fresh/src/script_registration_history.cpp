#include "pal4/script_registration_history.h"

namespace pal4 {

int ScriptRegistrationHistory::Initialize(
    std::uint8_t seed, IScriptRegistrationHistoryRuntime& runtime) {
    seed_ = seed;
    entries_.clear();
    return runtime.RegisterAtExit(kCleanupCallback);
}

void ScriptRegistrationHistory::Record(const char* declaration,
                                       const char* description) {
    entries_.emplace_back(declaration, description);
}

void ScriptRegistrationHistory::Cleanup() {
    std::vector<Entry>().swap(entries_);
}

}  // namespace pal4
