#pragma once

#include <vector>
struct IDirect3DDevice9;

class D3DXVolumeTextureSaver
{
public:
	D3DXVolumeTextureSaver();
	~D3DXVolumeTextureSaver();

	bool SaveToVolumeTexture(
		const std::vector<float>& r,
		const std::vector<float>& g,
		const std::vector<float>& b,
		const wchar_t* outputFile
		);

private:
	IDirect3DDevice9* m_Device;
};

