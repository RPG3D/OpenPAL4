#pragma once
#include <cstdint>
#include <vector>
#include <iostream>
#include <stdexcept>
namespace pal4::test {
template<class Manager,class Clear> void VerifyNativeCombatFirstRoster(Manager& manager,Clear&& clear,const char* label){
    const auto check=[](bool b,const char* why){if(!b)throw std::runtime_error(why);};
    const auto reject=[&](auto&& f,const char* why){bool denied{};try{f();}catch(const std::logic_error&){denied=true;}check(denied,why);};
    check(!manager.ActiveIterators() && manager.Count()>manager.StageCount(),"first-roster fixture includes actual offstage owners");
    const auto first=manager.CreateFirstIterator();
    struct Cursor {Manager& m;std::uint32_t token;~Cursor(){if(token)m.DeleteFirstIterator(token);}} cursor{manager,first};
    check(manager.ActiveIterators()==1u,"first roster participates in manager lifetime guard");
    reject([&]{manager.DeleteStageIterator(first);},"stage API rejects first-roster cursor");
    const auto stage=manager.CreateStageIterator();
    reject([&]{manager.FirstIteratorAtEnd(stage);},"first API rejects stage cursor");manager.DeleteStageIterator(stage);
    std::vector<std::uint32_t> seen;
    while(!manager.FirstIteratorAtEnd(first)){seen.push_back(manager.FirstIteratorActor(first));manager.NextFirstIterator(first);}
    check(seen.size()==manager.Count(),"first cursor includes offstage actual actors");
    for(std::size_t i=0;i<seen.size();++i)check(seen[i]==manager.ActorAt(i),"first roster preserves original creation order");
    manager.NextFirstIterator(first);check(manager.FirstIteratorAtEnd(first),"first cursor next at end remains bounded");
    reject([&]{manager.FirstIteratorActor(first);},"first cursor does not dereference end");
    reject([&]{clear();},"actual clear rejects while only first cursor is held");
    const auto again=manager.CreateFirstIterator();manager.DeleteFirstIterator(again);
    check(manager.Count()==seen.size() && manager.ActiveIterators()==1u,"blocked clear leaves manager reusable and roster intact");
    manager.DeleteFirstIterator(first);cursor.token=0u;check(!manager.ActiveIterators(),"first cursor retirement balances lifetime guard");
    reject([&]{manager.FirstIteratorAtEnd(first);},"retired first cursor rejected");
    std::cout<<"Native combat first roster: "<<label<<" actual offstage/order, separate cursor families, clear guard and cleanup pass\n";
}
}
