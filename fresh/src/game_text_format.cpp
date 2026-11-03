#include "pal4/game_text_format.h"
#include <cstdio>
#include <stdexcept>
#include <vector>
namespace pal4 {
namespace {
std::size_t Validate(std::string_view value,std::string_view types){
    std::size_t conversions{};
    for(std::size_t i=0;i<value.size();++i)if(value[i]=='%'){
        if(++i<value.size() && value[i]=='%')continue;
        while(i<value.size() && std::string_view("-0123456789.").find(value[i])!=std::string_view::npos)++i;
        if(i==value.size() || conversions>=types.size() || value[i]!=types[conversions++])throw std::invalid_argument("game text format contract");
    }
    return conversions;
}
template<class... Args>std::string Render(const std::string& format,Args... args){
    const int count=std::snprintf(nullptr,0u,format.c_str(),args...);
    if(count<0)throw std::runtime_error("game text formatting failed");std::vector<char> bytes(static_cast<std::size_t>(count)+1u);
    if(std::snprintf(bytes.data(),bytes.size(),format.c_str(),args...)!=count)throw std::runtime_error("game text formatting changed size");
    return {bytes.data(),static_cast<std::size_t>(count)};
}
}
std::string FormatGameTextArgument(std::string_view value,std::string_view argument){
    (void)Validate(value,"s");const std::string text(argument);return Render(std::string(value),text.c_str());
}
std::string FormatGameTextQuantity(std::string_view value,std::string_view name,std::int32_t quantity){
    if(Validate(value,"sd")!=2u)throw std::invalid_argument("game material format contract");
    const std::string text(name);return Render(std::string(value),text.c_str(),quantity);
}
std::string FormatGameTextArguments(std::string_view value,std::string_view first,std::string_view second){
    (void)Validate(value,"ss");const std::string a(first),b(second);
    return Render(std::string(value),a.c_str(),b.c_str());
}
std::string FormatGameTextInteger(std::string_view value,std::int32_t amount){
    (void)Validate(value,"d");return Render(std::string(value),amount);
}
}
