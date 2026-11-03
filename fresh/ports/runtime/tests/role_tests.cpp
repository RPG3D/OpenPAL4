#include "pal4/portable/role.h"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
using namespace pal4::portable;
void Check(bool value) { if (!value) throw std::runtime_error("Role contract failed"); }
void Word(std::string& bytes, std::uint32_t value) {
    for (unsigned shift=0; shift<32; shift+=8) bytes += static_cast<char>(value >> shift);
}
void Text(std::string& bytes, const std::string& value) { Word(bytes, static_cast<std::uint32_t>(value.size())); bytes+=value; }
std::string Fixture() {
    std::string bytes="P4RL"; Word(bytes, 1); Word(bytes, 4); Word(bytes, 50);
    for (unsigned index=0; index<4; ++index) {
        Word(bytes, index);
        Text(bytes, "ROLE_ID"); Word(bytes, 1); Word(bytes, 1001+index);
        for (const auto* name : {"RA_NAME", "RA_ICON", "RA_MODEL", "RA_TEXTURE", "SOUND_WOUNDED1", "SOUND_WOUNDED2", "SOUND_WOUNDED3"}) {
            Text(bytes, name); Word(bytes, 2); Text(bytes, std::string(name)=="RA_MODEL"?"101":"\xe4\xba\x91");
        }
        for (unsigned i=8; i<50; ++i) {
            Text(bytes, "field_"+std::to_string(i)); Word(bytes, i==8?3:1); Word(bytes, i==8?0x80000000u:0xffffffffu);
        }
    }
    return bytes;
}
RoleCatalog Parse(const std::string& bytes) { std::istringstream stream(bytes); return ReadRoles(stream); }
void Reject(const std::string& bytes) {
    bool failed=false; try { (void)Parse(bytes); } catch (const std::exception&) { failed=true; } Check(failed);
}
}
int main(int argc, char** argv) {
    try {
        if (argc==2) {
            std::ifstream stream(argv[1], std::ios::binary);
            WriteRoleReport(std::cout, ReadRoles(stream)); return 0;
        }
        const auto bytes=Fixture(); const auto roles=Parse(bytes);
        Check(roles[3].Number("ROLE_ID")==1004 && roles[0].Text("RA_NAME")=="\xe4\xba\x91");
        Check(roles[0].Number("field_9")==0xffffffffu);
        Check(std::bit_cast<std::uint32_t>(roles[0].Real("field_8"))==0x80000000u);
        for (std::size_t size=0; size<bytes.size(); ++size) Reject(bytes.substr(0, size));
        Reject(bytes+"x");
        auto bad=bytes; bad[4]=2; Reject(bad);
        bad=bytes; bad[8]=5; Reject(bad);
        bad=bytes; bad[16]=1; Reject(bad);
        bad=bytes; const auto field=bad.find("field_8"); bad[field+7]=4; Reject(bad);
        bad=bytes; bad[field+11]=0; bad[field+12]=0; bad[field+13]=char(0x80); bad[field+14]=char(0x7f); Reject(bad);
        bad=bytes; const auto chinese=bad.find("\xe4\xba\x91"); bad[chinese]=char(0xc0); Reject(bad);
        bad=bytes; bad.replace(bad.find("field_9"), 7, "field_8"); Reject(bad);
        bad=bytes; const auto second=bad.find("field_8", field+7); bad[second+6]='X'; Reject(bad);
        bool wrong_type=false; try { (void)roles[0].Number("RA_NAME"); } catch (const std::bad_variant_access&) { wrong_type=true; } Check(wrong_type);
        std::ostringstream report; WriteRoleReport(report, roles);
        Check(report.str().find("e4ba91")!=std::string::npos);
        std::cout << "Role resource checks passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
