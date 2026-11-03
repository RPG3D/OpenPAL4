#pragma once
#include "pal4/native_dialog_config_runtime.h"
#include "pal4/native_effect_text_manager.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "pal4/native_package_archive.h"
#include "pal4/binary_xml_config.h"
#include <bit>
#include <functional>
#include <map>
#include <source_location>

namespace pal4::test {
inline void VerifyNativeDialogConfig(const std::filesystem::path& game_root) {
    const auto check=[](bool ok,const std::source_location at=std::source_location::current()) {
        if(!ok) throw std::runtime_error("native Dialog config line "+std::to_string(at.line()));
    };
    const auto address=[](const void* p){return reinterpret_cast<std::uint32_t>(p);};
    NativeBaseObjectMemory memory;
    NativeEventDocumentRuntime documents(memory);
    std::array<std::uint32_t,15> storage{};
    const auto root=address(storage.data());
    (void)documents.InitializeDocument(root,"source",0u);
    NativePackageArchive archive;
    const auto path=(game_root/"gamedata"/"Effect.cpk").string();
    check(archive.Open(path.c_str()));
    {
        NativeEffectTextManager templates(memory,documents);
        const auto nil=memory.Allocate(44u),head=memory.Allocate(44u);
        std::memset(reinterpret_cast<void*>(nil),0,44u);
        std::memset(reinterpret_cast<void*>(head),0,44u);
        memory.WriteDword(nil+40u,1u);
        memory.WriteDword(head,head);memory.WriteDword(head+4u,nil);memory.WriteDword(head+8u,head);
        DialogObjectState state;
        state.words[4]=templates.Address();state.words[40]=head;
        NativeDialogConfigRuntime config(memory,documents,templates,nil);
        const auto text=[&](std::uint32_t record) {
            const auto p=memory.ReadDword(record+4u);
            return std::string(p?reinterpret_cast<const char*>(p):"",memory.ReadDword(record+8u));
        };
        const auto child=[](const BinaryXmlNode& n,std::string_view name)->const BinaryXmlNode* {
            for(const auto& c:n.children) if(c.name==name) return &c;
            return nullptr;
        };
        const auto attr=[](const BinaryXmlNode& n,std::string_view name)->const BinaryXmlAttribute* {
            for(const auto& a:n.attributes) if(a.name==name) return &a;
            return nullptr;
        };
        for(const auto name:{"foot_2","H_212","Y_004"}) {
            const auto data=archive.ReadMember((std::string("script\\")+name+".eft").c_str());
            BinaryXmlConfig expected;
            check(data && expected.Parse(*data) && documents.LoadBinaryDocument(root,*data));
            check(config.Load(state,root));
            const auto& node=expected.Root();
            const auto objects=child(node,"ObjectList"),tasks=child(node,"TaskList"),params=child(node,"TaskParamList");
            check(objects && tasks && params &&
                state.words[29]-state.words[28]==objects->children.size()*20u &&
                state.words[25]-state.words[24]==tasks->children.size()*8u &&
                state.words[33]-state.words[32]==params->children.size()*4u);
            documents.CleanupDocument(root);
            for(unsigned i=0;i<objects->children.size();++i) {
                const auto a=attr(objects->children[i],"ObjectId");
                check(a && text(state.words[28]+i*20u)==std::get<std::string>(a->value) &&
                    !memory.ReadDword(state.words[28]+i*20u+16u));
            }
            for(unsigned i=0;i<tasks->children.size();++i) {
                const auto a=attr(tasks->children[i],"TaskId");
                check(a && memory.ReadDword(state.words[24]+i*8u)==static_cast<std::uint32_t>(std::get<std::int32_t>(a->value)) &&
                    !memory.ReadDword(state.words[24]+i*8u+4u));
            }
            for(unsigned i=0;i<params->children.size();++i) {
                const auto p=memory.ReadDword(state.words[32]+i*4u);
                const auto a=attr(params->children[i],"Attr_Class_Name");
                check(p && documents.NodeName(p)==params->children[i].name && a &&
                    documents.AttributeToString(documents.FindNumericAttribute(p,"Attr_Class_Name"))==std::get<std::string>(a->value));
            }
            const auto oxn=child(node,"OxnList");check(oxn);
            for(const auto& p:oxn->children) {
                const auto a=attr(p,"Attr_Label");check(a);
                const auto stored=templates.Find(std::get<std::string>(a->value));
                check(stored && documents.NodeName(stored)==p.name);
            }
            std::map<std::string,std::array<float,3>> wanted,actual;
            if(const auto targets=child(node,"TargetList")) for(const auto& p:targets->children) {
                const auto a=attr(p,"TargetName");check(a);
                std::array<float,3> position{};
                unsigned i=0;
                for(const auto key:{"TargetPosX","TargetPosY","TargetPosZ"}) {
                    if(const auto value=attr(p,key)) position[i]=std::get<float>(value->value);
                    ++i;
                }
                wanted.try_emplace(std::get<std::string>(a->value),position);
            }
            std::function<void(std::uint32_t)> visit=[&](std::uint32_t p) {
                if(p==nil) return;
                visit(memory.ReadDword(p));
                std::array<float,3> pos{};
                for(unsigned i=0;i<3u;++i) pos[i]=std::bit_cast<float>(memory.ReadDword(p+28u+i*4u));
                actual.emplace(text(p+12u),pos);
                visit(memory.ReadDword(p+8u));
            };
            visit(memory.ReadDword(head+4u));
            check(actual==wanted && state.words[42]==wanted.size());
            const auto distance=attr(node,"DistanceClip"),camera=attr(node,"CameraStart");
            check((state.words[20]>>24u)==(distance?std::get<std::int32_t>(distance->value)>0:1) &&
                std::bit_cast<float>(state.words[21])==(camera?std::max(0.0f,std::get<float>(camera->value)):1.0f));
            state.words[2]=1u;bool busy_rejected=false;
            try {config.Clear(state);} catch(const std::logic_error&) {busy_rejected=true;}
            check(busy_rejected);state.words[2]=0u;
            if(state.words[29]!=state.words[28]) {
                memory.WriteDword(state.words[28]+16u,1u);busy_rejected=false;
                try {(void)config.Load(state,root);} catch(const std::logic_error&) {busy_rejected=true;}
                check(busy_rejected);memory.WriteDword(state.words[28]+16u,0u);
            }
            config.Clear(state);
            check(state.words[25]==state.words[24] && state.words[29]==state.words[28] &&
                state.words[33]==state.words[32] && !state.words[42] &&
                memory.ReadDword(head+4u)==nil && documents.DocumentCount()==1u);
        }
        const BinaryXmlNode generated{"Effect",{{"CameraStart",2u,-2.0f}},{{"OxnList",{},{}},
            {"TargetList",{},{{"target",{{"TargetName",3u,std::string("duplicate")},{"TargetPosX",2u,1.0f}},{}},
                {"target",{{"TargetName",3u,std::string("duplicate")},{"TargetPosX",2u,2.0f}}, {}}}}}};
        const auto generated_data=BinaryXmlConfig::Serialize(generated);
        check(generated_data && documents.LoadBinaryDocument(root,*generated_data) && config.Load(state,root));
        check((state.words[20]>>24u)==1u && !state.words[21] && state.words[42]==1u &&
            std::bit_cast<float>(memory.ReadDword(memory.ReadDword(head+4u)+28u))==1.0f);
        config.Clear(state);
        const BinaryXmlNode missing_oxn{"Effect",{},{{"TaskParamList",{},{{"unloaded",{}, {}}}}}};
        const auto missing_data=BinaryXmlConfig::Serialize(missing_oxn);
        check(missing_data && documents.LoadBinaryDocument(root,*missing_data) && !config.Load(state,root) &&
            state.words[33]-state.words[32]==4u);
        config.Clear(state);
        const BinaryXmlNode invalid{"WrongRoot",{}, {}};
        const auto invalid_data=BinaryXmlConfig::Serialize(invalid);
        check(invalid_data && documents.LoadBinaryDocument(root,*invalid_data));
        const auto allocations=memory.OutstandingAllocations();
        check(!config.Load(state,root) && memory.OutstandingAllocations()==allocations && !config.Load(state,0u));
        for(const auto index:{24u,28u,32u}) (void)memory.Free(state.words[index]);
        (void)memory.Free(head);(void)memory.Free(nil);
    }
    documents.DestroyDocument(root);
    check(!documents.DocumentCount() && !documents.SharedNil() && !memory.OutstandingAllocations());
}
}  // namespace pal4::test
