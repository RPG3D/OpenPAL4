#include "pal4/native_miles_3d_listener.h"
#include "pal4/native_miles_digital_driver.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void Reject(F f){bool rejected{};try{f();}catch(const std::exception&){rejected=true;}Check(rejected,"expected listener ownership/state rejection");}
}
int main(int argc,char** argv){try{
    Check(argc==2,"provide original PAL4 installation path");
    pal4::NativeMilesDigitalDriver driver(argv[1]);
    std::vector<std::uint32_t> messages;
    pal4::NativeMiles3dListener listener(driver,[&](auto id){messages.push_back(id);});
    Reject([&]{(void)listener.Open();});
    Reject([&]{(void)listener.Position();});
    Check(driver.Open(),"open original Miles driver");driver.SetMasterVolume(0.f);
    Check(listener.Open() && listener.Opened() && listener.Provider()!=0u && !listener.ProviderName().empty(),"open actual original provider and listener");
    const auto provider=listener.Provider();
    Check(listener.Position()==std::array<float,3>{0.f,0.f,0.f},"listener initializes at actual origin");
    listener.SetPosition({4.f,-7.f,2.5f});
    Check(listener.Position()==std::array<float,3>{4.f,-7.f,2.5f},"original DLL reads back all position components");
    Check(listener.Open() && listener.Provider()==provider && messages.size()==1u,"repeated open retains the same live provider/listener");
    pal4::NativeMiles3dListener other(driver);
    Reject([&]{(void)other.Open();});
    listener.SetPosition({0.f,0.f,0.f});
    Check(listener.Position()==std::array<float,3>{0.f,0.f,0.f},"actual5756D0 origin reset is observable through original getter");
    std::cout<<"Original Miles 3D listener provider="<<listener.ProviderName()<<" message="<<messages.front()<<" position/reset/repeat/ownership pass\n";
    listener.Close();listener.Close();
    Check(!listener.Opened() && !listener.Provider(),"close clears native listener/provider publication");
    Reject([&]{listener.SetPosition({});});
    Check(other.Open("PAL4_missing_provider_probe"),"original shared provider search falls back to a usable provider");
    other.Close();
    Check(listener.Open(),"provider and listener can reopen after complete retirement");listener.Close();driver.Close();
    std::cout<<"Original Miles listener fallback/retirement/reopen pass; no samples played\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
