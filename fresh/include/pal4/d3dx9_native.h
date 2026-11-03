#pragma once

// The original SDK owns the D3DX interfaces and private implementations.
#include <d3dx9.h>

#ifdef PAL4_D3DX9_EXPORTS
#define PAL4_D3DX9_API __declspec(dllexport)
#else
#define PAL4_D3DX9_API __declspec(dllimport)
#endif

namespace pal4 {

class PAL4_D3DX9_API D3DX9 final {
public:
    // Public boundaries identified in the ownership ledger. Returned COM
    // objects belong to D3DX; callers release them through their interfaces.
    static HRESULT CreateFont(IDirect3DDevice9* device, INT height, UINT width,
        UINT weight, UINT mip_levels, BOOL italic, DWORD charset,
        DWORD precision, DWORD quality, DWORD pitch, LPCSTR face, ID3DXFont** font);
    static HRESULT CreateFontIndirect(IDirect3DDevice9* device,
        const D3DXFONT_DESCA* description, ID3DXFont** font);
    static HRESULT CreateSprite(IDirect3DDevice9* device, ID3DXSprite** sprite);
    static HRESULT CreateEffect(IDirect3DDevice9* device, LPCVOID source, UINT size,
        const D3DXMACRO* defines, ID3DXInclude* include, DWORD flags,
        ID3DXEffectPool* pool, ID3DXEffect** effect, ID3DXBuffer** errors);
    static HRESULT CreateEffectCompiler(LPCSTR source, UINT size,
        const D3DXMACRO* defines, ID3DXInclude* include, DWORD flags,
        ID3DXEffectCompiler** compiler, ID3DXBuffer** errors);
    static HRESULT CreateTextureFromFileInMemory(IDirect3DDevice9* device,
        LPCVOID data, UINT size, IDirect3DTexture9** texture);
    static HRESULT AssembleShader(LPCSTR source, UINT size,
        const D3DXMACRO* defines, ID3DXInclude* include, DWORD flags,
        ID3DXBuffer** shader, ID3DXBuffer** errors);
    static D3DXVECTOR3* TransformCoord(D3DXVECTOR3* output,
        const D3DXVECTOR3* input, const D3DXMATRIX* matrix);
    static D3DXVECTOR3* TransformNormal(D3DXVECTOR3* output,
        const D3DXVECTOR3* input, const D3DXMATRIX* matrix);
    static D3DXMATRIX* Multiply(D3DXMATRIX* output,
        const D3DXMATRIX* left, const D3DXMATRIX* right);
    static D3DXMATRIX* Transpose(D3DXMATRIX* output, const D3DXMATRIX* input);
    static D3DXMATRIX* MultiplyTranspose(D3DXMATRIX* output,
        const D3DXMATRIX* left, const D3DXMATRIX* right);
};

} // namespace pal4
