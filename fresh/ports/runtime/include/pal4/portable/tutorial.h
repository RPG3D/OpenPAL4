#pragma once
#include "pal4/portable/script_stack.h"
#include "pal4/portable/text_catalog.h"
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
namespace pal4::portable {
// P4TU transports original CP936 text, including every rich-text/image tag.
using TutorialCatalog=TextCatalog;
TutorialCatalog ReadTutorialCatalog(std::istream&);
struct TutorialView {
    std::uint64_t generation{},id{},revision{};
    std::uint32_t key{};
    bool visible{},waiting{};
    std::string source;
};
// UI owns presentation only. Closing a matching window retires the original
// ScriptTutorialWait on the next game frame; it never resumes bytecode directly.
class TutorialServices final:public IScriptContextServices {
public:
    using SaveFlag=std::function<void(std::uint32_t)>;
    TutorialServices(IScriptContextServices& next,const TutorialCatalog& catalog,
                     std::uint64_t generation,bool enabled,SaveFlag save);
    ~TutorialServices();
    void OnCall(const ScriptRequest& request) override;
    ScriptResponse Invoke(const ScriptRequest&) override;
    TutorialView View() const;
    // Same original tutorial gate as giShowTutorial, without creating a CSB wait.
    bool ShowFromUi(std::uint32_t key);
    bool Close(std::uint64_t generation,std::uint64_t id);
    std::optional<ScriptToken> Advance();
    void Cancel();
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
