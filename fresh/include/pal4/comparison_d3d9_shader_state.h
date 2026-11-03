#pragma once
#include <d3d9.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <vector>
#include <xmmintrin.h>

namespace pal4 {
// Read-only D3D9 diagnostic state. Raw words preserve NaNs, signed zero and
// exact constants; unused registers are not evidence of an active input mismatch.
struct ComparisonD3D9ShaderState {
    bool read_failed{};
    unsigned short x87_control{};
    unsigned mxcsr{};
    std::vector<DWORD> vertex_code,pixel_code,vertex_float,pixel_float;
    std::array<DWORD,64> vertex_integer{},pixel_integer{};
    std::array<BOOL,16> vertex_bool{},pixel_bool{};
    std::array<DWORD,48> transforms{};
    std::array<DWORD,32> texture_transforms{};
    std::array<DWORD,2> texture_transform_flags{},texture_coordinate_index{};
    template<class Shader> bool ReadCode(Shader* shader,std::vector<DWORD>& code) {
        if(!shader)return true;
        UINT bytes{};
        if(FAILED(shader->GetFunction(nullptr,&bytes)) || !bytes || bytes%4u || bytes>65536u)return false;
        code.resize(bytes/4u);
        return SUCCEEDED(shader->GetFunction(code.data(),&bytes));
    }
    void Read(IDirect3DDevice9* device,IDirect3DVertexShader9* vs,IDirect3DPixelShader9* ps,UINT max_vertex_constants) noexcept {
        try {
            unsigned short control{};
            __asm fnstcw control
            x87_control=control;mxcsr=_mm_getcsr();
            const auto check=[&](HRESULT result){read_failed=FAILED(result)||read_failed;};
            if(!ReadCode(vs,vertex_code) || !ReadCode(ps,pixel_code))read_failed=true;
            if(vs && !vertex_code.empty()){
                const auto requested=D3DSHADER_VERSION_MAJOR(vertex_code[0])<2u?96u:256u;
                const auto count=requested<max_vertex_constants?requested:max_vertex_constants;
                vertex_float.resize(count*4u);
                check(device->GetVertexShaderConstantF(0u,reinterpret_cast<float*>(vertex_float.data()),count));
                check(device->GetVertexShaderConstantI(0u,reinterpret_cast<int*>(vertex_integer.data()),16u));
                check(device->GetVertexShaderConstantB(0u,vertex_bool.data(),16u));
            }
            if(ps && !pixel_code.empty()){
                const auto major=D3DSHADER_VERSION_MAJOR(pixel_code[0]);
                const auto count=major>=3u?224u:major>=2u?32u:8u;
                pixel_float.resize(count*4u);
                check(device->GetPixelShaderConstantF(0u,reinterpret_cast<float*>(pixel_float.data()),count));
                check(device->GetPixelShaderConstantI(0u,reinterpret_cast<int*>(pixel_integer.data()),16u));
                check(device->GetPixelShaderConstantB(0u,pixel_bool.data(),16u));
            }
            const std::array<D3DTRANSFORMSTATETYPE,3> types{D3DTS_WORLD,D3DTS_VIEW,D3DTS_PROJECTION};
            for(unsigned i=0;i<types.size();++i){D3DMATRIX matrix{};
                check(device->GetTransform(types[i],&matrix));
                std::memcpy(transforms.data()+16u*i,&matrix,sizeof(matrix));}
            for(unsigned i=0;i<2;++i){D3DMATRIX matrix{};
                check(device->GetTransform(static_cast<D3DTRANSFORMSTATETYPE>(D3DTS_TEXTURE0+i),&matrix));
                std::memcpy(texture_transforms.data()+16u*i,&matrix,sizeof(matrix));
                check(device->GetTextureStageState(i,D3DTSS_TEXTURETRANSFORMFLAGS,&texture_transform_flags[i]));
                check(device->GetTextureStageState(i,D3DTSS_TEXCOORDINDEX,&texture_coordinate_index[i]));}
        }catch(...){read_failed=true;}
    }
    template<class Range>static void WriteArray(std::ostream& out,const Range& words){
        out<<'[';bool first=true;for(const auto word:words){if(!first)out<<',';first=false;out<<word;}out<<']';
    }
    void Write(std::ostream& out)const {
        out<<"{\"read_failed\":"<<read_failed<<",\"x87_control\":"<<x87_control<<",\"mxcsr\":"<<mxcsr<<",\"vs_code\":";WriteArray(out,vertex_code);
        out<<",\"ps_code\":";WriteArray(out,pixel_code);
        out<<",\"vs_float\":";WriteArray(out,vertex_float);
        out<<",\"ps_float\":";WriteArray(out,pixel_float);
        out<<",\"vs_integer\":";WriteArray(out,vertex_integer);
        out<<",\"ps_integer\":";WriteArray(out,pixel_integer);
        out<<",\"vs_bool\":";WriteArray(out,vertex_bool);
        out<<",\"ps_bool\":";WriteArray(out,pixel_bool);
        out<<",\"transform_bits\":";WriteArray(out,transforms);
        out<<",\"texture_transform_bits\":";WriteArray(out,texture_transforms);
        out<<",\"texture_transform_flags\":";WriteArray(out,texture_transform_flags);
        out<<",\"texture_coordinate_index\":";WriteArray(out,texture_coordinate_index);out<<'}';
    }
};
}
