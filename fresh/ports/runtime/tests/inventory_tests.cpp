#include "pal4/portable/inventory.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace pal4::portable;
void Check(bool ok){if(!ok)throw std::runtime_error("inventory contract failed");}
void Word(std::string& out,std::uint32_t value){for(unsigned i=0;i<4;++i)out+=static_cast<char>(value>>(i*8));}
int main(int argc,char** argv){try{
    if(argc==3){std::ifstream source(argv[1],std::ios::binary);const auto catalog=ReadPropertyCatalog(source);Check(catalog.size()==246 && catalog.at(3103).category==4 && catalog.at(3241).category==4);
        Inventory inventory(catalog);inventory.Add(3103,1);inventory.Add(3241,1);Check(inventory.Count(3103)==1 && inventory.Count(3241)==1);
        std::ofstream out(argv[2]);out<<"{\"independent_fixture\":true,\"game_calls\":0,\"inventory\":";inventory.WriteReport(out);out<<'}';return 0;}
    std::string bytes="P4PI";Word(bytes,1);Word(bytes,1);Word(bytes,3103);Word(bytes,4);Word(bytes,4);bytes+="\xb3\xa4\xb9\xad";
    std::istringstream input(bytes);const auto parsed=ReadPropertyCatalog(input);Check(parsed.at(3103).name=="长弓");
    for(std::size_t i=0;i<bytes.size();++i){bool rejected=false;try{std::istringstream in(bytes.substr(0,i));(void)ReadPropertyCatalog(in);}catch(const std::exception&){rejected=true;}Check(rejected);}
    PropertyCatalog catalog;for(unsigned i=1;i<=8;++i)catalog.emplace(i,PropertyDefinition{i,i%5,"item"});Inventory inventory(catalog);
    Check(inventory.Money()==0 && inventory.AdjustMoney(-1)==0);
    Check(inventory.AdjustMoney(25)==25 && inventory.AdjustMoney(-5)==20);
    Check(inventory.AdjustMoney(10000000)==9999999 && inventory.AdjustMoney(2147483647)==0);
    inventory.Add(999,1);Check(inventory.Records().empty() && !inventory.Seen(999));
    inventory.Add(1,0);Check(inventory.Records().size()==1 && inventory.Count(1)==0 && inventory.Seen(1) && inventory.Records()[0].sequence==1);
    Check(inventory.CategoryCount(1)==1 && inventory.AtReverse(1,0)->count==0 && !inventory.AtReverse(1,1));
    Check(!inventory.Featured(1));inventory.SetFeatured(1,1);Check(inventory.Featured(1)->id==1);
    Check(inventory.FindSequence(1,99)->id==1 && !inventory.FindSequence(1,2));
    inventory.Add(1,120);Check(inventory.Count(1)==99 && inventory.Records()[0].sequence==1 && inventory.Recent().size()==1);
    inventory.Add(1,-9);Check(inventory.Count(1)==90);inventory.Remove(1,-50);Check(inventory.Count(1)==99);
    inventory.Remove(1,100);Check(inventory.Count(1)==0 && inventory.Records().empty() && inventory.Recent().empty() && inventory.Seen(1));
    Check(!inventory.Featured(1) && !inventory.FindSequence(1));
    for(unsigned i=1;i<=8;++i)inventory.Add(i,1);Check(inventory.Records().size()==8 && inventory.Recent().size()==6 && inventory.Recent().front()==4 && inventory.Recent().back()==9);
    inventory.Add(3,1);Check(inventory.Recent().size()==6 && inventory.Recent().back()==4 && inventory.Count(3)==2);
    inventory.Remove(3,2);Check(inventory.Recent().size()==5 && inventory.Count(3)==0);
    inventory.Add(1,99);inventory.Add(1,2147483647);Check(inventory.Count(1)==0x80000062u); // 原32位相加后作有符号上限比较。
    inventory.Remove(1,1);Check(inventory.Count(1)==0);
    std::cout<<"inventory categories, original counts, sequence, recent and cache contracts passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
