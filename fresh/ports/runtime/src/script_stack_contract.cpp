#include "pal4/portable/script_stack.h"
#include <map>
#include <stdexcept>

namespace pal4::portable {
ChapterStackContract VerifyChapterStackContract(std::string_view source,
    std::string_view opening_trace,std::string_view destination_trace) {
    struct Recording:IScriptContextServices {
        std::map<std::uint64_t,std::string> traces;
        std::optional<ScriptToken> loading;
        std::uint32_t calls{};
        void OnCall(const ScriptRequest& request) override {
            traces[request.token.context]+=EncodeScriptCall(request.call);++calls;
        }
        ScriptResponse Invoke(const ScriptRequest& request) override {
            const auto& call=request.call;
            if(call.system_id==65) {
                if(loading || std::get<std::string>(call.arguments.at(0))!="Q01" || std::get<std::string>(call.arguments.at(1))!="Q01")
                    throw std::runtime_error("unexpected opening scene request");
                loading=request.token;
            }
            if(call.system_id==133 || call.system_id==287 || call.system_id==288)return {std::int32_t(1),true};
            return {{},true};
        }
    } recording;
    ScriptGlobals globals{};globals[0]=10100;ScriptStack stack(globals);
    stack.Compile("Q01",source);
    const auto opening=stack.Push("Q01","void Q01_N01_init()",recording);
    if(!opening)throw std::runtime_error("missing opening script entry");
    ChapterStackContract result;
    for(unsigned tick=0;tick<100000 && stack.Depth();++tick) {
        const auto state=stack.Tick(.05);result.simulation_seconds+=.05;
        if(state==ScriptState::failed)throw std::runtime_error(stack.Error());
        if(state==ScriptState::finished) {
            const auto id=stack.TopId();
            const auto trace=recording.traces[id]+"final "+std::to_string(globals[0])+"\n";
            if(trace!=(id==*opening?opening_trace:destination_trace))
                throw std::runtime_error("nested opening trace differs from legacy source oracle");
            stack.PopFinished();continue;
        }
        if(recording.loading) {
            const auto token=*recording.loading;recording.loading.reset();
            if(!stack.Complete(token))throw std::runtime_error("scene request lost its caller wait");
            if(!stack.Push("Q01","void Q01_Q01_init()",recording))throw std::runtime_error("destination scene initializer was not queued");
            ++result.scene_initializers;
        } else if(stack.WaitKind()==ScriptWaitKind::external) {
            if(!stack.Complete(*stack.Pending()))throw std::runtime_error("recording service completion failed");
        }
    }
    if(stack.Depth() || result.scene_initializers!=1 || globals[0]!=10201)
        throw std::runtime_error("opening stack did not complete its nested scene route");
    result.calls=recording.calls;result.story=globals[0];return result;
}
} // namespace pal4::portable
