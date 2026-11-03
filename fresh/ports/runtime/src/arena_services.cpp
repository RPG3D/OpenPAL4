#include "pal4/portable/arena_services.h"
#include <limits>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
namespace {
std::string Name(const ScriptValue& value,bool optional=false) {
    auto name=std::get<std::string>(value);name.resize(name.find('\0')==std::string::npos?name.size():name.find('\0'));
    if((name.empty() && !optional) || name.size()>64 || name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos)
        throw std::invalid_argument("invalid arena load name");
    for(auto& c:name)if(c>='a' && c<='z')c=static_cast<char>(c-('a'-'A'));
    return name;
}
}
ScriptResponse ArenaServices::Invoke(const ScriptRequest& request) {
    const auto& call=request.call;if(call.system_id!=65)return next_.Invoke(request);
    if(pending_)return {}; // 原ArenaManager非ready时忽略重复装载。
    if(call.arguments.size()!=4 || !std::holds_alternative<bool>(call.arguments[3]))throw std::invalid_argument("invalid arena load arguments");
    auto world=Name(call.arguments[0]);auto scene=Name(call.arguments[1]);auto resource=Name(call.arguments[2],true);
    if(next_generation_==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("arena scene generation exhausted");
    pending_=ArenaLoadCommand{request.token,++next_generation_,std::move(world),std::move(scene),std::move(resource)};
    dispatched_=false;
    // 内部装载屏障，供GL线程在Tick返回后处理；原回调未读取第四个bool。
    return {{},true};
}
std::optional<ArenaLoadCommand> ArenaServices::TakeCommand() {
    if(!pending_ || dispatched_)return {};
    dispatched_=true;return pending_;
}
bool ArenaServices::Finish(ScriptToken token) {
    if(!pending_ || !dispatched_ || pending_->token!=token)return false;
    Cancel();return true;
}
} // namespace pal4::portable
