#include "pal4/portable/script_module_cache.h"
#include <stdexcept>
#include <utility>

namespace pal4::portable {
const ScriptModuleCache::Bytes& ScriptModuleCache::Load(std::string_view module,
    const Reader& reader) {
    if(module.empty() || module.find('\0')!=std::string_view::npos || !reader)
        throw std::invalid_argument("invalid script module cache request");
    if(const auto found=modules_.find(std::string(module));found!=modules_.end())
        return found->second;
    auto bytes=reader(module);
    if(bytes.empty())throw std::runtime_error("empty script module bytes");
    return modules_.emplace(std::string(module),std::move(bytes)).first->second;
}
const ScriptModuleCache::Bytes* ScriptModuleCache::Find(std::string_view module) const {
    const auto found=modules_.find(std::string(module));
    return found==modules_.end()?nullptr:&found->second;
}
void ScriptModuleCache::RetainOnly(std::string_view module) {
    for(auto it=modules_.begin();it!=modules_.end();) {
        if(it->first==module){++it;continue;}
        it=modules_.erase(it);
    }
}
void ScriptModuleCache::Clear() { modules_.clear(); }
std::size_t ScriptModuleCache::Size() const { return modules_.size(); }
} // namespace pal4::portable
