#include "Engine.h"
#include "DX12/DX12Device.h"

#include "DX12/DX12CommandQueue.h"
#include "DX12/DX12DescriptorHeap.h"

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

	delete m_RTVDescriptorHeap;
	delete m_DSVDescriptorHeap;
	delete m_SRVDescriptorHeap;

#if defined(_DEBUG)
	ID3D12DebugDevice* debug_device = nullptr;
	ThrowIfFailed(m_Device->QueryInterface(__uuidof(ID3D12DebugDevice), reinterpret_cast<void**>(&debug_device)));

	HRESULT result = debug_device->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
	ThrowIfFailed(result);

	debug_device->Release();
#endif

	m_Device->Release();
}

void DX12Device::Init(HWND inWindowHandle, uint32 inWidth, uint32 inHeight)
{
	EnableDebugLayer();
	EnableGPUBasedValidation();

	{
		IDXGIAdapter4* dxgi_adapter4 = GetAdapter(m_UseWarp);
		m_Device = CreateDevice(dxgi_adapter4);
		dxgi_adapter4->Release();
	}

	m_RTVDescriptorHeap = new DX12DescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_BUFFERED_FRAMES);
	m_DSVDescriptorHeap = new DX12DescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, NUM_BUFFERED_FRAMES);
	m_SRVDescriptorHeap = new DX12DescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

	m_DirectCommandQueue = new DX12CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_ComputeCommandQueue = new DX12CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CopyCommandQueue = new DX12CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_COPY);

	m_SwapChain = new DX12SwapChain(*this, inWindowHandle, *m_DirectCommandQueue, inWidth, inHeight);
}

void DX12Device::Flush()
{
	m_DirectCommandQueue->Flush();
	m_ComputeCommandQueue->Flush();
	m_CopyCommandQueue->Flush();
}

void DX12Device::Present(ID3D12GraphicsCommandList2* inCommandList)
{
	m_SwapChain->Present(inCommandList, m_DirectCommandQueue);
}

ID3D12Device2* DX12Device::CreateDevice(IDXGIAdapter4* inAdapter)
{
	ID3D12Device2* d3d12_device2;
	ThrowIfFailed(D3D12CreateDevice(inAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12_device2)));

	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ID3D12InfoQueue* info_queue;
	if (SUCCEEDED(d3d12_device2->QueryInterface(__uuidof(ID3D12InfoQueue), (void **) &info_queue)))
	{
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID deny_IDs[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,	// I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,							// This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,						// This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER new_filter = {};
		//new_filter.DenyList.NumCategories	= _countof(Categories);
		//new_filter.DenyList.pCategoryList	= Categories;
		new_filter.DenyList.NumSeverities	= _countof(severities);
		new_filter.DenyList.pSeverityList	= severities;
		new_filter.DenyList.NumIDs			= _countof(deny_IDs);
		new_filter.DenyList.pIDList			= deny_IDs;

		ThrowIfFailed(info_queue->PushStorageFilter(&new_filter));

		info_queue->Release();
	}
#endif

	return d3d12_device2;
}

void DX12Device::EnableGPUBasedValidation()
{
#if defined(_DEBUG)
	ID3D12Debug*	debug_controller0 = nullptr;
	ID3D12Debug1*	debug_controller1 = nullptr;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller0)));
	ThrowIfFailed(debug_controller0->QueryInterface(IID_PPV_ARGS(&debug_controller1)));
	debug_controller1->SetEnableGPUBasedValidation(true);

	debug_controller1->Release();
	debug_controller0->Release();
#endif
}

void DX12Device::EnableDebugLayer()
{
#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ID3D12Debug* debug_interface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
	debug_interface->EnableDebugLayer();

	debug_interface->Release();
#endif
}

IDXGIAdapter4* DX12Device::GetAdapter(bool inUseWarp)
{
	IDXGIFactory4* dxgi_factory = nullptr;
	UINT create_factory_flags = 0;
#if defined(_DEBUG)
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

	IDXGIAdapter4* dxgi_adapter4 = nullptr;

	if (inUseWarp)
	{
		IDXGIAdapter1* dxgi_adapter1 = nullptr;
		ThrowIfFailed(dxgi_factory->EnumWarpAdapter(IID_PPV_ARGS(&dxgi_adapter1)));
		ThrowIfFailed(dxgi_adapter1->QueryInterface(__uuidof(IDXGIAdapter4), (void **) &dxgi_adapter4));
		dxgi_adapter1->Release();
	}
	else
	{
		IDXGIAdapter1* dxgi_adapter1 = nullptr;
		SIZE_T max_dedicated_video_memory = 0;
		for (UINT i = 0; dxgi_factory->EnumAdapters1(i, &dxgi_adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 desc;
			dxgi_adapter1->GetDesc1(&desc);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgi_adapter1, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				desc.DedicatedVideoMemory > max_dedicated_video_memory)
			{
				max_dedicated_video_memory = desc.DedicatedVideoMemory;
				ThrowIfFailed(dxgi_adapter1->QueryInterface(__uuidof(IDXGIAdapter4), (void **) &dxgi_adapter4));
			}

			dxgi_adapter1->Release();
		}
	}

	dxgi_factory->Release();

	return dxgi_adapter4;
}

DX12CommandQueue* DX12Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
	DX12CommandQueue* command_queue = nullptr;
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		command_queue = m_DirectCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		command_queue = m_ComputeCommandQueue;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		command_queue = m_CopyCommandQueue;
		break;
	default:
		Assert(false, "Invalid command queue type.");
	}

	return command_queue;
}

DX12DescriptorHeap* DX12Device::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE inType) const
{
	DX12DescriptorHeap* descriptor_heap = nullptr;
	switch (inType)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		descriptor_heap = m_RTVDescriptorHeap;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		descriptor_heap = m_DSVDescriptorHeap;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		descriptor_heap = m_SRVDescriptorHeap;
		break;
	default:
		Assert(false, "Unsuppported D3D12_DESCRIPTOR_HEAP_TYPE");
		break;
	}

	return descriptor_heap;
}