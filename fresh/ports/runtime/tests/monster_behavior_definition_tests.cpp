#include "pal4/portable/monster_behavior_definition.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("monster behavior definition contract failed");}
void Integer(BinaryXmlNode& node,std::string name,int value){node.attributes.push_back({std::move(name),1,std::int32_t(value)});}
void Real(BinaryXmlNode& node,std::string name,float value){node.attributes.push_back({std::move(name),2,value});}
BinaryXmlNode Choice(int type,int proportion=10,int wait_mode=1){
    BinaryXmlNode node;node.name="choice";
    Integer(node,"MSTINFO_Behavior_ObjectType",type);
    Integer(node,"MSTINFO_Behavior_ObjectProportion",proportion);
    Integer(node,"MSTINFO_Behavior_ObjectWaitMode",wait_mode);return node;
}
BinaryXmlConfig Document(std::vector<BinaryXmlNode> groups){
    BinaryXmlNode root;root.name="MSTINFO_Behavior";root.children=std::move(groups);
    const auto bytes=BinaryXmlConfig::Serialize(root);Check(bytes.has_value());
    BinaryXmlConfig result;Check(result.Parse(*bytes));return result;
}
}
int main(){try{
    Check(!ReadMonsterBehaviorDefinition(Document({})).fuzzy);
    BinaryXmlNode group;group.name="group";group.children={Choice(0)};
    Check(!ReadMonsterBehaviorDefinition(Document({group})).fuzzy);
    Integer(group,"MSTINFO_Behavior_GroupMode",3);
    Check(!ReadMonsterBehaviorDefinition(Document({group})).fuzzy);
    group.attributes.clear();Integer(group,"MSTINFO_Behavior_GroupMode",0);
    group.children={Choice(0),Choice(1,20,2),Choice(2),Choice(3),Choice(7)};
    Real(group.children[1],"MSTINFO_Behavior_ObjectWaitTime",2.5f);
    Real(group.children[1],"MSTINFO_Behavior_ObjectExpectTime",4.f);
    const auto source=Document({group});const auto before=source.Serialize();
    const auto definition=ReadMonsterBehaviorDefinition(source);
    Check(definition.fuzzy && definition.fuzzy->mode==0 && definition.fuzzy->choices.size()==5);
    for(int i=0;i<4;++i)Check(definition.fuzzy->choices[i].class_id==i);
    Check(definition.fuzzy->choices[4].class_id==-1);
    Check(definition.fuzzy->choices[0].wait_time==-1 && definition.fuzzy->choices[0].expect_time==-1);
    Check(definition.fuzzy->choices[1].proportion==20 && definition.fuzzy->choices[1].wait_mode==2 &&
        definition.fuzzy->choices[1].wait_time==2.5f && definition.fuzzy->choices[1].expect_time==4);
    Check(source.Serialize()==before);
    // 原函数保留无效子项之前加入的记录，但不会写组模式。
    for(auto invalid:{Choice(-1),Choice(1,-1),Choice(1,10,0)}){
        auto partial=group;partial.children={Choice(2),invalid,Choice(3)};
        const auto result=ReadMonsterBehaviorDefinition(Document({partial}));
        Check(result.fuzzy && !result.fuzzy->mode && result.fuzzy->choices.size()==1 && result.fuzzy->choices[0].class_id==2);
    }
    auto empty=group;empty.children.clear();const auto empty_result=ReadMonsterBehaviorDefinition(Document({empty}));
    Check(empty_result.fuzzy && empty_result.fuzzy->choices.empty() && empty_result.fuzzy->mode==0);
    // 只读取文档首个子节点，后续组不合并。
    auto second=group;second.children={Choice(3)};
    const auto two=ReadMonsterBehaviorDefinition(Document({empty,second}));Check(two.fuzzy && two.fuzzy->choices.empty());
    auto text=group;text.children={Choice(0)};text.children[0].attributes.back().value=std::string("2");text.children[0].attributes.back().type=3;
    const auto text_result=ReadMonsterBehaviorDefinition(Document({text}));Check(text_result.fuzzy && text_result.fuzzy->choices[0].wait_mode==2);
    std::ostringstream report;WriteMonsterBehaviorDefinition(report,definition);Check(report.str().find("\"class_id\":-1")!=std::string::npos);
    std::cout<<"original fuzzy definition defaults, partial groups, class IDs and XML ownership contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
