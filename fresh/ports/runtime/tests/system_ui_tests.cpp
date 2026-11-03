#include "pal4/portable/system_ui.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4::portable;
int main() {
    try {
        GameDataEntries data;SystemUiPermissions permissions(data);
        if(data.Get(12))throw std::runtime_error("unknown entry must remain unknown");
        data.InitializeSceneUiDefaults();
        for(unsigned i=0;i<84;++i) {
            const bool initialized=(i>=8 && i<=20)||i==26;
            if(data.Get(i)!=(initialized?std::optional<std::uint32_t>(1):std::nullopt))
                throw std::runtime_error("original UI default range or unknown boundary differs");
        }
        permissions.GrantMagic(false,true);permissions.Grant(4,false);
        if(data.Get(20)!=0 || data.Get(19)!=1 || data.Get(12)!=0 || data.Get(11)!=1)throw std::runtime_error("permission mapping failed");
        std::ostringstream report;permissions.WriteReport(report);
        if(report.str()!="{\"calls\":2,\"writes\":[{\"entry\":20,\"value\":0},{\"entry\":19,\"value\":1},{\"entry\":12,\"value\":0}]}")throw std::runtime_error("permission write order failed");
        data.Set(16,37); // A distinct sentinel detects a mistaken index8 -> entry16 write.
        permissions.Grant(-1,true);permissions.Grant(8,true);
        if(data.Get(7) || data.Get(16)!=37)throw std::runtime_error("invalid permission index wrote entry");
        for(int i=0;i<8;++i) {permissions.Grant(i,true);if(data.Get(static_cast<unsigned>(i)+8)!=1)throw std::runtime_error("valid permission index failed");}
        bool rejected=false;try{data.Set(84,0);}catch(const std::out_of_range&){rejected=true;}
        if(!rejected)throw std::runtime_error("out of range entry accepted");
        std::cout<<"game entries and original UI permission mapping, write order and bounds passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
