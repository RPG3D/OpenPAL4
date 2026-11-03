#pragma once
#include "pal4/portable/csb.h"
#include "pal4/portable/script.h"
#include <functional>

namespace pal4::portable {
using CsbTextDecoder=std::function<std::string(std::string_view)>;
// Legacy instruction compatibility backend. Original DWORD values remain 32 bit;
// VM references are checked indices rather than truncated native addresses.
class CsbExecution {
public:
    CsbExecution(std::shared_ptr<const CsbModule> module,std::string_view entry,
        ScriptGlobals& globals,IScriptServices& services,CsbTextDecoder decoder={});
    ~CsbExecution();
    CsbExecution(const CsbExecution&)=delete;
    CsbExecution& operator=(const CsbExecution&)=delete;
    ScriptState Execute(std::uint32_t budget=10000,ScriptRunMode mode=ScriptRunMode::until_wait);
    ScriptState State() const;
    ScriptPause PauseReason() const;
    const std::string& Error() const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace pal4::portable
