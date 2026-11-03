#include "pal4/portable/monster_behavior_definition.h"
#include "pal4/monster_fuzzy_behavior_loader.h"
#include <bit>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace pal4::portable {
namespace {
void Require(bool value,const char* message){if(!value)throw std::logic_error(message);}
struct Reader final:pal4::IMonsterFuzzyBehaviorRuntime {
    std::vector<const BinaryXmlNode*> nodes;
    std::vector<const BinaryXmlAttribute*> attributes;
    MonsterBehaviorDefinition result;
    static constexpr std::uint32_t event=0x10000,records=0x20000;
    explicit Reader(const BinaryXmlConfig& source){
        // 原4318A0只取行为文档第一个子节点，空文档向加载器传0。
        if(!source.Root().children.empty())nodes.push_back(&source.Root().children.front());
    }
    const BinaryXmlNode& Node(std::uint32_t id) const {
        Require(id && id<=nodes.size(),"invalid monster behavior node");return *nodes[id-1];
    }
    std::uint32_t Attribute(std::uint32_t node,const char* name) override {
        if(!node)return 0;
        const auto* value=Node(node).FindAttribute(name);if(!value)return 0;
        Require(attributes.size()<std::numeric_limits<std::uint32_t>::max(),"too many monster behavior attributes");
        attributes.push_back(value);return static_cast<std::uint32_t>(attributes.size());
    }
    const BinaryXmlAttribute& Attr(std::uint32_t id) const {
        Require(id && id<=attributes.size(),"invalid monster behavior attribute");return *attributes[id-1];
    }
    std::int32_t ParseInt(std::uint32_t id,std::int32_t fallback) override {
        return std::bit_cast<std::int32_t>(Attr(id).ToInt(std::bit_cast<std::uint32_t>(fallback)));
    }
    float ParseFloat(std::uint32_t id,float fallback) override {
        const auto value=Attr(id).ToFloat(fallback);Require(std::isfinite(value),"non-finite monster behavior time");return value;
    }
    std::int32_t ChildCount(std::uint32_t node) override {
        const auto count=Node(node).children.size();Require(count<=10000,"too many monster behavior choices");return static_cast<std::int32_t>(count);
    }
    std::uint32_t ChildByIndex(std::uint32_t node,std::uint32_t index) override {
        const auto& children=Node(node).children;if(index>=children.size())return 0;
        nodes.push_back(&children[index]);return static_cast<std::uint32_t>(nodes.size());
    }
    std::uint32_t CreateEvent(std::uint32_t manager,const char* name) override {
        Require(manager==1 && std::string_view(name)=="MST-Fuzzy-Behavior" && !result.fuzzy,"invalid monster behavior event");
        result.fuzzy.emplace();return event;
    }
    std::uint32_t AddClassProportion(std::uint32_t owner,std::int32_t type,std::int32_t proportion) override {
        Require(owner==event && result.fuzzy.has_value(),"invalid monster behavior event owner");
        auto& choices=result.fuzzy->choices;Require(choices.size()<10000,"too many monster behavior records");
        choices.push_back({type,proportion,0,0,0});return records+static_cast<std::uint32_t>(choices.size()-1);
    }
    MonsterBehaviorChoice& Choice(std::uint32_t id){
        Require(result.fuzzy && id>=records && id-records<result.fuzzy->choices.size(),"invalid monster behavior record");return result.fuzzy->choices[id-records];
    }
    void SetIntAttribute(std::uint32_t id,const char* name,std::int32_t value) override {
        Require(std::string_view(name)=="beh-vtm-waitMode","invalid monster behavior integer field");Choice(id).wait_mode=value;
    }
    void SetFloatAttribute(std::uint32_t id,const char* name,float value) override {
        if(std::string_view(name)=="beh-vtm-waitTime")Choice(id).wait_time=value;
        else if(std::string_view(name)=="beh-vtm-expectTime")Choice(id).expect_time=value;
        else throw std::logic_error("invalid monster behavior float field");
    }
    void SetGroupMode(std::uint32_t owner,std::int32_t mode) override {
        Require(owner==event && result.fuzzy.has_value(),"invalid monster behavior group owner");result.fuzzy->mode=mode;
    }
};
}
MonsterBehaviorDefinition ReadMonsterBehaviorDefinition(const BinaryXmlConfig& source){
    Reader runtime(source);pal4::MonsterFuzzyBehaviorLoader::Load(1,runtime.nodes.empty()?0u:1u,runtime);return std::move(runtime.result);
}
void WriteMonsterBehaviorDefinition(std::ostream& out,const MonsterBehaviorDefinition& definition){
    out<<std::setprecision(9)<<"{\"fuzzy_event_defined\":"<<(definition.fuzzy?"true":"false")<<",\"group_mode\":";
    if(definition.fuzzy && definition.fuzzy->mode)out<<*definition.fuzzy->mode;else out<<"null";
    out<<",\"choices\":[";
    if(definition.fuzzy)for(std::size_t i=0;i<definition.fuzzy->choices.size();++i){
        if(i)out<<',';
        const auto& c=definition.fuzzy->choices[i];
        out<<"{\"class_id\":"<<c.class_id<<",\"proportion\":"<<c.proportion<<",\"wait_mode\":"<<c.wait_mode
            <<",\"wait_time\":"<<c.wait_time<<",\"expect_time\":"<<c.expect_time<<'}';
    }
    out<<"]}";Require(bool(out),"cannot write monster behavior definition");
}
} // namespace pal4::portable
