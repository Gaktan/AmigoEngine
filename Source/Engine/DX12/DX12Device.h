#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

class DX12CommandQueue;
class DX12SwapChain;

class DX12Device
{
protected:
public: //Temp hack
	ID3D12Device2*			m_Device;
	DX12SwapChain*			m_SwapChain;

protected:
	// Use WARP adapter
	bool					m_UseWarp = false;

	DX12CommandQueue*		m_DirectCommandQueue;
	DX12CommandQueue*		m_ComputeCommandQueue;
	DX12CommandQueue*		m_CopyCommandQueue;

public:
	DX12Device();
	~DX12Device();
	void Init(HWND inWindowHandle, ui32 inClientWidth, ui32 inClientHeight);
	void Flush();
	void Present(ID3D12GraphicsCommandList2* inCommandList);

	DX12CommandQueue* GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

protected:
	ID3D12Device2* CreateDevice(IDXGIAdapter4* inAdapter);

	void EnableGPUBasedValidation();
	void EnableDebugLayer();

	IDXGIAdapter4* GetAdapter(bool inUseWarp);
};