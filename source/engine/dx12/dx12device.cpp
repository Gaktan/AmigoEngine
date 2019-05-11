#include "engine_precomp.h"
#include "dx12device.h"

#include "dx12/dx12commandqueue.h"

#include <d3dcompiler.h>
//#include <DirectXMath.h>
//
#include <D3dx12.h>

DX12Device::DX12Device()
{
}

DX12Device::~DX12Device()
{
	delete m_DirectCommandQueue;
	delete m_ComputeCommandQueue;
	delete m_CopyCommandQueue;

	delete m_SwapChain;

#if defined(_DEBUG)
	ID3D12DebugDevice* debugDevice = nullptr;
	ThrowIfFailed(m_Device->QueryInterface(__uuidof(ID3D12DebugDevice), reinterpret_cast<void**>(&debugDevice)));

	HRESULT result = debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	ThrowIfFailed(result);

	debugDevice->Release();
#endif

	m_Device->Release();
}

void DX12Device::Init(HWND windowHandle, ui32 clientWidth, ui32 clientHeight)
{
	EnableDebugLayer();
	EnableGPUBasedValidation();

	{
		IDXGIAdapter4* dxgiAdapter4 = GetAdapter(m_UseWarp);
		m_Device = CreateDevice(dxgiAdapter4);
		dxgiAdapter4->Release();
	}

	m_DirectCommandQueue = new DX12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = new DX12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = new DX12CommandQueue(this, D3D12_COMMAND_LIST_TYPE_COPY);

	m_SwapChain = new DX12SwapChain(*this, windowHandle, m_DirectCommandQueue, clientWidth, clientHeight);
}

void DX12Device::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

void DX12Device::Present(ID3D12GraphicsCommandList2* commandList)
{
	m_SwapChain->Present(commandList, m_DirectCommandQueue);
}

ID3D12Device2* DX12Device::CreateDevice(IDXGIAdapter4* adapter)
{
	ID3D12Device2* d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ID3D12InfoQueue* pInfoQueue;
	if (SUCCEEDED(d3d12Device2->QueryInterface(__uuidof(ID3D12InfoQueue), (void **) &pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));

		pInfoQueue->Release();
	}
#endif

	return d3d12Device2;
}

void DX12Device::EnableGPUBasedValidation()
{
#if defined(_DEBUG)
	ID3D12Debug* spDebugController0;
	ID3D12Debug1* spDebugController1;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&spDebugController0)));
	ThrowIfFailed(spDebugController0->QueryInterface(IID_PPV_ARGS(&spDebugController1)));
	spDebugController1->SetEnableGPUBasedValidation(true);

	spDebugController1->Release();
	spDebugController0->Release();
#endif
}

void DX12Device::EnableDebugLayer()
{
#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ID3D12Debug* debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();

	debugInterface->Release();
#endif
}

IDXGIAdapter4* DX12Device::GetAdapter(bool useWarp)
{
	IDXGIFactory4* dxgiFactory = nullptr;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	IDXGIAdapter4* dxgiAdapter4 = nullptr;

	if (useWarp)
	{
		IDXGIAdapter1* dxgiAdapter1 = nullptr;
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter4), (void **) &dxgiAdapter4));
		dxgiAdapter1->Release();
	}
	else
	{
		IDXGIAdapter1* dxgiAdapter1 = nullptr;
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter4), (void **) &dxgiAdapter4));
			}

			dxgiAdapter1->Release();
		}
	}

	dxgiFactory->Release();

	return dxgiAdapter4;
}

DX12CommandQueue* DX12Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	DX12CommandQueue* commandQueue;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		commandQueue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		commandQueue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		commandQueue = m_CopyCommandQueue;
		break;
	default:
		commandQueue = nullptr;
		Assert(false && "Invalid command queue type.");
	}

	return commandQueue;
}