#include "pal4/portable/effect_document.h"
#include "pal4/dialog_object.h"
#include "pal4/effect_system_text_manager.h"
#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_map>
namespace pal4::portable {
namespace {
struct PlanReader final:pal4::IDialogObjectConfigRuntime {
    EffectPlan result;std::vector<const BinaryXmlNode*> nodes{nullptr};
    std::unordered_map<const BinaryXmlNode*,unsigned> handles;
    void Register(const BinaryXmlNode& node){
        if(nodes.size()>=std::numeric_limits<unsigned>::max())throw std::length_error("effect XML handle budget exceeded");
        handles.emplace(&node,static_cast<unsigned>(nodes.size()));nodes.push_back(&node);
        for(const auto& c:node.children)Register(c);
    }
    const BinaryXmlNode& Node(unsigned id) const{if(!id || id>=nodes.size())throw std::out_of_range("invalid effect XML handle");return *nodes[id];}
    unsigned Handle(const BinaryXmlNode& node) const{return handles.at(&node);}
    std::string_view NodeName(unsigned n) override{return Node(n).name;}
    unsigned FindChild(unsigned n,std::string_view name) override{
        for(const auto& c:Node(n).children){if(c.name==name)return Handle(c);}
        return 0;
    }
    unsigned NextChild(unsigned n,unsigned current,std::string_view name) override {
        bool found=false;for(const auto& c:Node(n).children){if(found && c.name==name)return Handle(c);if(Handle(c)==current)found=true;}
        if(!found)throw std::out_of_range("effect XML sibling outside parent");
        return 0;
    }
    unsigned ChildCount(unsigned n) override{return static_cast<unsigned>(Node(n).children.size());}
    unsigned ChildAt(unsigned n,unsigned i) override{return Handle(Node(n).children.at(i));}
    std::optional<std::string_view> StringAttribute(unsigned n,std::string_view name) override{const auto* a=Node(n).FindAttribute(name);if(a)if(const auto* s=std::get_if<std::string>(&a->value))return *s;return {};}
    void Numeric(const BinaryXmlAttribute& a){if(!std::holds_alternative<std::int8_t>(a.value) && !std::holds_alternative<std::int32_t>(a.value) && !std::holds_alternative<float>(a.value))throw std::logic_error("effect XML text-number conversion is not bound");}
    std::optional<int> IntAttribute(unsigned n,std::string_view name) override{const auto* a=Node(n).FindAttribute(name);if(!a)return {};Numeric(*a);return std::bit_cast<int>(a->ToInt(0));}
    std::optional<float> FloatAttribute(unsigned n,std::string_view name) override{const auto* a=Node(n).FindAttribute(name);if(!a)return {};Numeric(*a);return a->ToFloat(0);}
    void AppendObjectId(pal4::DialogObjectState&,std::string_view id) override{result.object_ids.emplace_back(id);}
    void AppendTaskId(pal4::DialogObjectState&,int id) override{result.task_ids.push_back(id);}
    unsigned CopyXmlNode(unsigned n) override{(void)Node(n);return n;}
    void AppendTaskParameter(pal4::DialogObjectState&,unsigned n) override{result.task_parameters.push_back(Node(n));}
    struct TextReader final:pal4::IEffectSystemTextManagerXmlRuntime {
        PlanReader& r;explicit TextReader(PlanReader& p):r(p){}
        void ClearElements(unsigned manager) override{if(manager!=1)throw std::logic_error("invalid effect template owner");r.result.templates.clear();}
        std::string_view NodeName(unsigned n) override{return r.NodeName(n);}
        int ChildCount(unsigned n) override{const auto count=r.ChildCount(n);if(count>unsigned(std::numeric_limits<int>::max()))throw std::length_error("effect template count exceeded");return static_cast<int>(count);}
        unsigned ChildAt(unsigned n,unsigned i) override{return r.ChildAt(n,i);}
        std::string ReadAttribute(unsigned n,std::string_view name) override{return std::string(r.StringAttribute(n,name).value_or(""));}
        bool AddEntry(unsigned manager,std::string_view name,unsigned n) override{if(manager!=1)throw std::logic_error("invalid effect template owner");return r.result.templates.emplace(std::string(name),r.Node(n)).second;}
    };
    bool LoadText(unsigned manager,unsigned node) override{TextReader reader(*this);return pal4::EffectSystemTextManager::LoadFromXml(manager,node,reader);}
    void InsertTarget(pal4::DialogObjectState&,std::string_view name,const std::array<float,3>& p) override{result.targets.emplace(std::string(name),p);}
};
}
EffectPlan ReadEffectPlan(const EffectDocument& document){
    PlanReader reader;reader.Register(document.xml.Root());pal4::DialogObjectState state;state.words[4]=1;
    if(!pal4::DialogObject::LoadConfig(state,1,reader))throw std::runtime_error("original effect Dialog configuration is invalid: "+document.member);
    reader.result.distance_clip=(state.words[20]>>24)!=0;reader.result.camera_start=std::bit_cast<float>(state.words[21]);return std::move(reader.result);
}
}
