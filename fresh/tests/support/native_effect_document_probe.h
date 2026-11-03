#pragma once
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_package_archive.h"
#include "pal4/native_effect_text_manager.h"
#include "pal4/binary_xml_config.h"
#include <array>
#include <bit>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativeEffectDocument(const std::filesystem::path& game_root) {
    const auto check=[](bool v) {if(!v) throw std::runtime_error("native effect XML document regression");};
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> storage{};
    const auto root=reinterpret_cast<std::uint32_t>(storage.data());
    (void)documents.InitializeDocument(root,"before",0u);
    const auto text=[&](std::uint32_t record) {
        const auto p=memory.ReadDword(record+4u),n=memory.ReadDword(record+8u);
        return std::string_view(p?reinterpret_cast<const char*>(p):"",n);
    };
    std::function<void(std::uint32_t,const BinaryXmlNode&)> compare;
    unsigned nodes{},attributes{};
    compare=[&](std::uint32_t node,const BinaryXmlNode& expected) {
        ++nodes;
        check(text(node)==expected.name && memory.ReadDword(node+52u)==expected.attributes.size() &&
              memory.ReadDword(node+56u)==expected.children.size());
        for(const auto& a:expected.attributes) {
            ++attributes;
            const auto attr=documents.FindNumericAttribute(node,a.name);
            check(attr && memory.ReadDword(attr+16u)==a.type);
            const auto value=memory.ReadDword(attr+24u);
            if(a.type==0u) check(static_cast<std::int8_t>(value)==std::get<std::int8_t>(a.value));
            else if(a.type==1u) check(value==static_cast<std::uint32_t>(std::get<std::int32_t>(a.value)));
            else if(a.type==2u) check(value==std::bit_cast<std::uint32_t>(std::get<float>(a.value)));
            else if(a.type==3u) check(text(value)==std::get<std::string>(a.value));
            else if(a.type==4u) {
                const auto& bytes=std::get<std::vector<std::uint8_t>>(a.value);
                check(memory.ReadDword(attr+20u)==bytes.size());
                if(!bytes.empty()) check(std::memcmp(reinterpret_cast<const void*>(value),bytes.data(),bytes.size())==0);
            }
        }
        const auto children=memory.ReadDword(node+40u);
        for(unsigned i=0;i<expected.children.size();++i) {
            const auto child=memory.ReadDword(children+i*4u);
            check(memory.ReadDword(child+16u)==node);
            compare(child,expected.children[i]);
        }
    };
    BinaryXmlNode generated{"Effect",{
        {"char",0u,std::int8_t{-3}}, {"integer",1u,std::int32_t{-99}},
        {"float",2u,-1.25f}, {"text",3u,std::string("abc")},
        {"raw",4u,std::vector<std::uint8_t>{0u,1u,255u}}},
        {{"empty",{{"empty-text",3u,std::string{}},{"empty-raw",4u,std::vector<std::uint8_t>{}}},{}}}};
    const auto encoded=BinaryXmlConfig::Serialize(generated);
    check(encoded && documents.LoadBinaryDocument(root,*encoded));
    compare(root,generated);
    check(documents.AttributeToString(documents.FindNumericAttribute(root,"char"))==std::string(1,static_cast<char>(-3)) &&
          documents.AttributeToString(documents.FindNumericAttribute(root,"integer"))=="-99" &&
          documents.AttributeToString(documents.FindNumericAttribute(root,"float"))=="-1.250000" &&
          documents.AttributeToString(documents.FindNumericAttribute(root,"text"))=="abc" &&
          documents.AttributeToString(0u).empty());
    bool raw_rejected=false;
    const auto before_raw=memory.OutstandingAllocations();
    try { (void)documents.AttributeToString(documents.FindNumericAttribute(root,"raw")); }
    catch(const std::logic_error&) { raw_rejected=true; }
    check(raw_rejected && memory.OutstandingAllocations()==before_raw);
    const auto allocations=memory.OutstandingAllocations();
    auto truncated=*encoded; truncated.pop_back();
    check(!documents.LoadBinaryDocument(root,truncated) && memory.OutstandingAllocations()==allocations);
    compare(root,generated);
    auto trailing=*encoded; trailing.push_back(1u);
    check(!documents.LoadBinaryDocument(root,trailing));
    auto bad_version=*encoded; bad_version[0]=2u;
    check(!documents.LoadBinaryDocument(root,bad_version));
    const auto verify_independent_clone=[&](const BinaryXmlNode& expected) {
        const auto clone=documents.CloneDocument(root);
        check(clone && clone!=root && memory.ReadDword(clone+16u)==0u);
        documents.CleanupDocument(root);
        compare(clone,expected);
        documents.DestroyDocument(clone);
        (void)documents.Free(clone);
        check(documents.DocumentCount()==1u);
    };
    verify_independent_clone(generated);
    NativePackageArchive archive;
    const auto path=(game_root/"gamedata"/"Effect.cpk").string();
    check(archive.Open(path.c_str()));
    for(const auto name:{"foot_2","H_212","Y_004"}) {
        const auto bytes=archive.ReadMember((std::string("script\\")+name+".eft").c_str());
        BinaryXmlConfig expected;
        check(bytes && expected.Parse(*bytes) && documents.LoadBinaryDocument(root,*bytes));
        compare(root,expected.Root());
        verify_independent_clone(expected.Root());
    }
    {
        NativeEffectTextManager templates(memory,documents);
        for(const auto name:{"foot_2","H_212","Y_004"}) {
            const auto bytes=archive.ReadMember((std::string("script\\")+name+".eft").c_str());
            BinaryXmlConfig expected;
            check(bytes && expected.Parse(*bytes) && documents.LoadBinaryDocument(root,*bytes));
            const auto oxn=documents.FindChild(root,"OxnList");
            check(oxn && templates.Load(oxn));
            const BinaryXmlNode* expected_oxn=nullptr;
            for(const auto& child:expected.Root().children)
                if(child.name=="OxnList") expected_oxn=&child;
            check(expected_oxn && documents.ChildCount(oxn)==expected_oxn->children.size());
            // The template manager owns clones after the loader releases XML.
            documents.CleanupDocument(root);
            for(const auto& child:expected_oxn->children) {
                std::string label;
                for(const auto& attr:child.attributes)
                    if(attr.name=="Attr_Label") label=std::get<std::string>(attr.value);
                const auto property=templates.Find(label);
                check(property && documents.NodeName(property)==child.name);
                compare(property,child);
                const auto before_duplicate=memory.OutstandingAllocations();
                check(!templates.Add(label,property) && memory.OutstandingAllocations()==before_duplicate);
            }
            check(!templates.Find("not-an-effect-template"));
        }
        // AddEntry inserts or updates Attr_Label on the clone, preserving source.
        BinaryXmlNode unlabeled{"OxnList",{},{{"child",{},{}},{"other",{{"Attr_Label",3u,std::string("old")}}, {}}}};
        const auto data=BinaryXmlConfig::Serialize(unlabeled);
        check(data && documents.LoadBinaryDocument(root,*data));
        check(documents.ChildAt(root,2u)==0u && !documents.NextChild(root,documents.ChildAt(root,1u)));
        templates.Clear();
        const auto source=documents.ChildAt(root,0u),other=documents.ChildAt(root,1u);
        check(templates.Add("new-label",source) && !documents.FindNumericAttribute(source,"Attr_Label"));
        check(templates.Add("renamed",other) &&
              documents.AttributeToString(documents.FindNumericAttribute(other,"Attr_Label"))=="old");
        check(documents.AttributeToString(documents.FindNumericAttribute(templates.Find("new-label"),"Attr_Label"))=="new-label" &&
              documents.AttributeToString(documents.FindNumericAttribute(templates.Find("renamed"),"Attr_Label"))=="renamed");
        // Original LoadFromXml clears previous entries even for invalid input.
        check(!templates.Load(0u) && !templates.Find("new-label") && !templates.Find("renamed"));
        check(!templates.Load(source));
        BinaryXmlNode duplicate{"OxnList",{},{{"a",{{"Attr_Label",3u,std::string("same")}},{}},
            {"b",{{"Attr_Label",3u,std::string("same")}}, {}}}};
        const auto duplicate_data=BinaryXmlConfig::Serialize(duplicate);
        check(duplicate_data && documents.LoadBinaryDocument(root,*duplicate_data));
        check(!templates.Load(root) && documents.NodeName(templates.Find("same"))=="a");
        templates.Clear();
        check(!templates.Find("same"));
    }
    documents.CleanupDocument(root);
    check(nodes>20u && attributes>50u && documents.DocumentCount()==1u);
    documents.DestroyDocument(root);
    check(documents.DocumentCount()==0u && !documents.SharedNil() && memory.OutstandingAllocations()==0u);
}
}  // namespace pal4::test
