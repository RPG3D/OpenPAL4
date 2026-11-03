#include "pal4/portable/package.h"
#include "pal4/package_tea.h"
#include <minilzo.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace pal4;
using namespace pal4::portable;
namespace {
void Check(bool value){if(!value)throw std::runtime_error("package contract failed");}
void Put(std::string& bytes,std::size_t offset,std::uint32_t value) {
    for(unsigned b=0;b<4;++b)bytes[offset+b]=static_cast<char>(value>>(b*8));
}
// Fixture writer only: inverse of the shared block TEA reader.
void Encrypt(std::string& bytes,std::size_t offset,std::size_t count=1024) {
    std::vector<std::uint32_t> w(count);
    for(std::size_t i=0;i<w.size();++i)for(unsigned b=0;b<4;++b)
        w[i]|=std::uint32_t(static_cast<unsigned char>(bytes[offset+i*4+b]))<<(b*8);
    auto key=PackageTea::PackageKeyWords();std::uint32_t sum=0,z=w.back();
    for(unsigned round=0;round<6+52/count;++round) {
        sum+=0x9e3779b9u;const auto e=(sum>>2)&3;
        for(std::size_t p=0;p<w.size();++p) {
            const auto y=w[(p+1)%w.size()];
            z=w[p]+=(((z>>5)^(y<<2))+((y>>3)^(z<<4)))^((sum^y)+(key[(p&3)^e]^z));
        }
    }
    for(std::size_t i=0;i<w.size();++i)Put(bytes,offset+i*4,w[i]);
}
std::string Fixture(unsigned codec=1,bool encrypted=false,bool broken=false,std::string payload="ID3test!") {
    constexpr std::uint32_t table=0x100000,offset=128+table;
    const auto decoded=static_cast<std::uint32_t>(payload.size());
    if(encrypted)Encrypt(payload,0,2);
    if(codec==2) {
        std::vector<lzo_byte> packed(128),work(LZO1X_1_MEM_COMPRESS);lzo_uint size=packed.size();
        Check(lzo1x_1_compress(reinterpret_cast<const lzo_byte*>(payload.data()),payload.size(),packed.data(),&size,work.data())==LZO_E_OK);
        payload.assign(packed.begin(),packed.begin()+size);
        if(broken)payload[0]=0;
    }
    std::string b(offset+payload.size()+8,'\0');Put(b,0,0x1a545352);Put(b,16,32768);Put(b,32,1);
    PackageNameCrc crc;crc.InitializeTable();Put(b,128,crc.Compute(reinterpret_cast<const std::uint8_t*>("p23.mp3")));
    Put(b,132,(codec<<16)|1);Put(b,140,offset);Put(b,144,static_cast<std::uint32_t>(payload.size()));Put(b,148,decoded);
    Put(b,152,8);Put(b,156,encrypted?1:0);
    b.replace(offset,payload.size(),payload);b.replace(offset+payload.size(),7,"p23.mp3");
    Encrypt(b,128);Encrypt(b,128+table-4097);return b;
}
template<class F> void Reject(F action) {bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}Check(rejected);}
}
int main(int argc,char** argv) {
    try {
        if(argc==4 && std::string_view(argv[2])=="--find") {
            std::ifstream input(argv[1],std::ios::binary);PackageReader reader(input);
            const auto info=reader.Find(argv[3]);
            std::cout<<"member="<<argv[3]<<" found="<<(info?"true":"false")<<'\n';
            return 0;
        }
        if(argc==3 && std::string_view(argv[2])=="--list") {
            std::ifstream input(argv[1],std::ios::binary);PackageReader reader(input);
            for(const auto& name:reader.ListFiles())std::cout<<name<<'\n';return 0;
        }
        if(argc==4) {
            std::ifstream input(argv[1],std::ios::binary);PackageReader reader(input);
            const auto info=reader.Find(argv[2]);if(!info)throw std::runtime_error("member not found");
            std::ofstream output(argv[3],std::ios::binary);
            const auto count=reader.CopyTo(argv[2],output,512u*1024u*1024u);
            if(!output)throw std::runtime_error("output write failed");
            std::cout<<"codec="<<info->codec<<" encrypted="<<info->encryption<<" offset="<<info->offset<<" bytes="<<count<<'\n';return 0;
        }
        const auto fixture=Fixture();std::istringstream stream(fixture);PackageReader reader(stream);
        const auto data=reader.Read("P23.MP3");Check(std::string(data.begin(),data.end())=="ID3test!");
        Check(!reader.Find("missing.mp3"));Check(reader.Read("p23.mp3")==data);
        Check(!reader.ReadIfPresent("missing.mp3"));Check(reader.ReadIfPresent("P23.MP3")==data);
        Reject([&]{reader.ReadIfPresent("p23.mp3",7);});
        Check(reader.ListFiles()==std::vector<std::string>{"p23.mp3"});
        Check(lzo_init()==LZO_E_OK);
        for(std::size_t count:{2u,3u,13u,1024u})for(std::size_t tail=0;tail<4;++tail){
            std::string original(count*4+tail,'\0');for(std::size_t i=0;i<original.size();++i)original[i]=static_cast<char>((i*37+11)&255);
            auto encrypted=original;Encrypt(encrypted,0,count);
            std::vector<std::uint8_t> bytes(encrypted.begin(),encrypted.end());
            Check(PackageTea::DecryptBytes(bytes,PackageTea::PackageKeyWords())==0);
            Check(std::equal(bytes.begin(),bytes.end(),original.begin(),[](std::uint8_t a,char b){return a==static_cast<unsigned char>(b);}));
        }
        {std::array<std::uint8_t,7> short_data{};Check(PackageTea::DecryptBytes(short_data,PackageTea::PackageKeyWords())==-1);}
        for(unsigned codec:{1u,2u})for(bool encrypted:{false,true}) {
            std::istringstream s(Fixture(codec,encrypted));PackageReader r(s);Check(r.Read("p23.mp3")==data);Check(r.ReadIfPresent("p23.mp3")==data);std::ostringstream copied;Check(r.CopyTo("p23.mp3",copied)==data.size() && copied.str()=="ID3test!");
            PackageMemberStream movie(r,"p23.mp3",128);Check(movie.InMemory()==(codec!=1 || encrypted) && movie.Size()==8);
            std::array<std::uint8_t,4> part{};Check(movie.Read(part)==4 && std::string(part.begin(),part.end())=="ID3t");movie.Seek(6);
            Check(movie.Read(part)==2 && part[0]=='t' && part[1]=='!' && movie.Read(part)==0);Reject([&]{movie.Seek(9);});movie.Seek(0);Check(movie.Read(part)==4 && part[0]=='I');
            if(codec!=1 || encrypted)Reject([&]{PackageMemberStream limited(r,"p23.mp3",7);});
        }
        Reject([&]{PackageMemberStream missing(reader,"missing.mp3");});
        {
            std::string payload(196613,'x');payload[65536]='\0';payload.back()='z';
            std::istringstream input(Fixture(1,false,false,payload));PackageReader raw(input);std::ostringstream output;
            Check(raw.CopyTo("p23.mp3",output)==payload.size() && output.str()==payload);
            std::array<std::uint8_t,16> chunk{};Check(raw.ReadRaw("p23.mp3",65530,chunk)==16 && chunk[6]==0);
            Check(raw.ReadRaw("p23.mp3",payload.size()-1,chunk)==1 && chunk[0]=='z');
            Check(raw.ReadRaw("p23.mp3",payload.size(),chunk)==0);
            Reject([&]{raw.ReadRaw("p23.mp3",payload.size()+1,chunk);});
            PackageMemberStream first(raw,"p23.mp3"),second(raw,"p23.mp3");
            first.Seek(65530);second.Seek(payload.size()-1);
            Check(first.Read(chunk)==16 && chunk[6]==0 && first.Position()==65546);
            Check(second.Read(chunk)==1 && chunk[0]=='z' && second.Position()==payload.size());
            Check(first.Read(chunk)==16 && chunk[0]=='x' && first.Position()==65562);
            first.Seek(payload.size());Check(first.Read(chunk)==0);
            std::istringstream packedInput(Fixture(2));PackageReader packed(packedInput);Reject([&]{packed.ReadRaw("p23.mp3",0,chunk);});
            std::istringstream encryptedInput(Fixture(1,true));PackageReader encrypted(encryptedInput);Reject([&]{encrypted.ReadRaw("p23.mp3",0,chunk);});
        }
        Reject([&]{std::istringstream s(Fixture(2,false,true));PackageReader r(s);r.Read("p23.mp3");});
        Reject([&]{std::istringstream s(Fixture(3));PackageReader r(s);r.Read("p23.mp3");});
        Reject([&]{std::ostringstream output;reader.CopyTo("p23.mp3",output,7);});
        Reject([&]{std::ostringstream output;output.setstate(std::ios::badbit);reader.CopyTo("p23.mp3",output);});
        Reject([&]{reader.Read("p23.mp3",7);});Reject([&]{reader.Read("missing.mp3");});
        Reject([&]{reader.Find(std::string_view("p23.mp3\0x",9));});
        for(const auto size:{0u,127u,128u,1048703u})Reject([&]{std::istringstream s(fixture.substr(0,size));PackageReader r(s);});
        auto truncated=fixture.substr(0,128+0x100000+7);std::istringstream shortStream(truncated);PackageReader shortReader(shortStream);
        Reject([&]{shortReader.Read("p23.mp3");});
        auto bad=fixture;Put(bad,16,32769);Reject([&]{std::istringstream s(bad);PackageReader r(s);});
        bad=fixture;Put(bad,32,32769);Reject([&]{std::istringstream s(bad);PackageReader r(s);});
        std::cout<<"package shared TEA/index, raw/LZO, encryption, limits and truncation contracts passed\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
