#include "pal4/d3dx9_native.h"

#include <cstring>
#include <stdexcept>

namespace {
void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void CheckFlag(DWORD flags, bool expected_success) {
    constexpr char shader[] = "vs_1_1\nmov oPos, v0\n";
    ID3DXBuffer* assembled = nullptr;
    ID3DXBuffer* errors = nullptr;
    const HRESULT result = pal4::D3DX9::AssembleShader(
        shader, static_cast<UINT>(std::strlen(shader)), nullptr, nullptr,
        flags, &assembled, &errors);
    const bool success = SUCCEEDED(result);
    Require(success == expected_success, "D3DX assembler flag compatibility");
    if (expected_success)
        Require(assembled && assembled->GetBufferSize() > 0, "assembled shader output");
    else
        Require(result == D3DERR_INVALIDCALL && !assembled,
                "unsupported flag rejected before shader assembly");
    if (assembled) assembled->Release();
    if (errors) errors->Release();
}
}

int main() {
    CheckFlag(D3DXSHADER_SKIPVALIDATION, true);
    CheckFlag(D3DXSHADER_SKIPOPTIMIZATION, false);
}
