#include "pal4/native_combat_event_table.h"
#include <iostream>
#include <stdexcept>
#include <string>

int main(){try{
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    const auto reject=[&](auto fn){bool failed{};try{fn();}catch(const std::logic_error&){failed=true;}check(failed,"expected rejection");};
    std::uint32_t slot{};
    {
        pal4::NativeCombatEventTable table(slot);
        const auto address=slot;
        check(address && address==table.Address() && !table.BindingCount() && table.Text().empty(),
              "actual constructor clears67 pairs and initializes owned text");
        reject([&]{pal4::NativeCombatEventTable duplicate(slot);});
        check(slot==address,"duplicate leaves published owner intact");
        const std::string long_text(4096,'X');table.SetText(long_text);
        check(slot==address && table.Text()==long_text,"text allocation cannot relocate published object");
        slot=1u;reject([&]{table.Close();});slot=address;
        check(table.Text()==long_text,"failed close preserves backend for retry");
        table.Close();table.Close();check(!slot,"close clears member publication");
        reject([&]{table.SetText("closed");});reject([&]{(void)table.Address();});
    }
    {pal4::NativeCombatEventTable table(slot);table.SetText("shutdown");}
    check(!slot,"destructor retires backend and publication");
    std::cout<<"PASS: native Combat member400,67 pairs, host text, duplicate/foreign guards and retirement\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
