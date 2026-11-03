#include "pal4/portable/inventory_services.h"
#include "pal4/script_inventory_change.h"
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool ok,const char* error){if(!ok)throw std::logic_error(error);}
}
struct InventoryServices::Impl final:pal4::IScriptInventoryChangeRuntime {
    ::pal4::portable::Inventory& inventory;const PropertyCatalog& catalog;TextLookup text;Hint hint;Sound sound;std::string buffer;bool buffer_active{};
    Impl(::pal4::portable::Inventory& i,const PropertyCatalog& c,TextLookup t,Hint h,Sound s):inventory(i),catalog(c),text(std::move(t)),hint(std::move(h)),sound(std::move(s)){Require(bool(text) && bool(hint) && bool(sound),"inventory presentation services are missing");}
    std::uint32_t UiFrameManager() override{return 1;}
    std::uint32_t CreateHintWindow(std::uint32_t frames) override{Require(frames==1,"invalid inventory hint manager");return 2;}
    std::uint32_t GameDatabase() override{return 0x100;}
    std::uint8_t HasProperty(std::uint32_t db,std::uint32_t id) override{Require(db==0x100,"invalid inventory database");return catalog.contains(id);}
    std::uint32_t FindProperty(std::uint32_t db,std::uint32_t id) override{Require(db==0x100 && catalog.contains(id),"invalid inventory property lookup");return id;}
    std::uint32_t Inventory() override{return 3;}
    void AddItem(std::uint32_t owner,std::uint32_t id,std::int32_t count) override{Require(owner==3,"invalid inventory owner");inventory.Add(id,count);}
    void RemoveItem(std::uint32_t owner,std::uint32_t id,std::int32_t count) override{Require(owner==3,"invalid inventory owner");inventory.Remove(id,count);}
    void InitializeTextBuffer() override{Require(!buffer_active,"inventory text buffer reentry");buffer.clear();buffer_active=true;}
    std::string_view PropertyName(std::uint32_t id) override{return catalog.at(id).name;}
    std::string format;
    std::string_view GameString(std::uint32_t table,std::uint32_t id) override{Require(table==0x134 && (id==251 || id==252),"invalid inventory GameString request");const auto value=text(id);Require(value.has_value(),"original inventory message is missing");format=*value;return format;}
    void FormatText(std::string_view pattern,std::string_view name,std::int32_t count) override{
        Require(buffer_active && pattern.size()<=65536,"invalid inventory message format");unsigned names{},counts{};
        for(std::size_t p=0;p<pattern.size();++p){if(pattern[p]!='%'){buffer+=pattern[p];continue;}
            Require(++p<pattern.size(),"truncated inventory format token");
            if(pattern[p]=='%')buffer+='%';else if(pattern[p]=='s'){buffer+=name;++names;}else if(pattern[p]=='d'){buffer+=std::to_string(count);++counts;}
            else throw std::runtime_error("unsupported inventory message format token");
        }
        Require(names==1 && counts==1,"inventory message substitutions differ from original");
    }
    std::string_view TextBuffer() override{Require(buffer_active,"inventory text buffer is inactive");return buffer;}
    void ShowHint(std::uint32_t window,std::string_view value,float x,float y,float duration,std::uint32_t mode,std::uint32_t enabled) override{Require(window==2 && mode==0 && enabled==1,"invalid inventory hint binding");hint(value,x,y,duration);}
    std::uint32_t AudioManager() override{return 4;}
    void PlaySound(std::uint32_t audio,std::uint8_t flag,std::string_view path,std::uint32_t mode) override{Require(audio==4 && flag==1 && mode==1,"invalid inventory sound binding");
        if(path=="gamedata\\PALSound\\WA06.wav")sound("WA06",mode);else if(path=="gamedata\\PALSound\\WA07.wav")sound("WA07",mode);else throw std::logic_error("invalid inventory sound resource");}
    std::uint8_t CleanupTextBuffer() noexcept override{buffer.clear();buffer_active=false;return 1;}
};
InventoryServices::InventoryServices(Inventory& i,const PropertyCatalog& c,TextLookup t,Hint h,Sound s):impl_(std::make_unique<Impl>(i,c,std::move(t),std::move(h),std::move(s))){}
InventoryServices::~InventoryServices()=default;
void InventoryServices::Add(std::uint32_t id,std::int32_t count,bool hint){(void)pal4::ScriptInventoryChange::Add(id,count,hint,*impl_);}
void InventoryServices::Remove(std::uint32_t id,std::int32_t count,bool hint){(void)pal4::ScriptInventoryChange::Remove(id,count,hint,*impl_);}
}
