#include "D3DXVolumeTextureSaver.h"
#include <d3dx9.h>
#include <cassert>

D3DXVolumeTextureSaver::D3DXVolumeTextureSaver()
	: m_Device(nullptr)
{
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = 1;
	d3dpp.BackBufferHeight           = 1;
	d3dpp.BackBufferFormat           = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount            = 1;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality         = 0;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow              = nullptr;
	d3dpp.Windowed                   = true;
	d3dpp.EnableAutoDepthStencil     = false;
	d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	IDirect3D9* pD3DObject = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3DObject)
	{
		throw "Unable to create Direct3D9 object! Something is seriously wrong!";
	}

	HRESULT hr = pD3DObject->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		nullptr,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp,
		&m_Device);

	pD3DObject->Release();

	if (FAILED(hr))
	{
		throw "Unable to create Direct3D9 device!";
	}
}

D3DXVolumeTextureSaver::~D3DXVolumeTextureSaver()
{
	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
}

bool D3DXVolumeTextureSaver::SaveToVolumeTexture(
	const std::vector<float>& r,
	const std::vector<float>& g,
	const std::vector<float>& b,
	const wchar_t* outputFile)
{
	if (r.size() != g.size() || g.size() != b.size())
	{
		assert(!"All channels must be of the same dimension!");
		return false;
	}

	const size_t cubeSize = r.size();

	IDirect3DVolumeTexture9* pVolumeTex = nullptr;
	HRESULT hr = D3DXCreateVolumeTexture(m_Device, cubeSize, cubeSize, cubeSize, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &pVolumeTex);
	if (FAILED(hr))
	{
		assert(!"Unable to create volume texture!");
		return false;
	}

	D3DLOCKED_BOX box;
	hr = pVolumeTex->LockBox(0, &box, nullptr, 0);
	if (FAILED(hr))
	{
		assert(!"Unable to lock texture for writing!");
		return false;
	}

	char* pData = (char*)box.pBits;

	for (size_t slice = 0; slice < cubeSize; ++slice)
	{
		char* pSlice = &pData[slice * box.SlicePitch];

		for (size_t row = 0; row < cubeSize; ++row)
		{
			char* pRow = &pSlice[row * box.RowPitch];

			for (size_t col = 0; col < cubeSize; ++col)
			{
				int* color = (int*)pRow;
				
				float cr = r[col];
				float cg = g[row];
				float cb = b[slice];

				color[col] = D3DXCOLOR(cr, cg, cb, 1.0f);
			}
		}
	}

	hr = pVolumeTex->UnlockBox(0);

	hr = D3DXSaveTextureToFileW(outputFile, D3DXIFF_DDS, pVolumeTex, nullptr);
	if (FAILED(hr))
	{
		assert(!"Unable to save texture to file!");
		return false;
	}

	return true;
}