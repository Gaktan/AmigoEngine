#include "Engine.h"
#include "dx12swapchain.h"

#include "DX12Device.h"
#include "D3dx12.h"

bool CheckTearingSupport()
{
	bool allow_tearing = false;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	IDXGIFactory4* factory4;
	HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));

	if (result == S_OK)
	{
		IDXGIFactory5* factory5;
		result = factory4->QueryInterface(__uuidof(IDXGIFactory5), (void **) &factory5);

		if (result == S_OK)
		{
			result = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow_tearing, sizeof(allow_tearing));
			if (result != S_OK)
			{
				allow_tearing = false;
			}
			factory5->Release();
		}
		factory4->Release();
	}

	return allow_tearing;
}

DX12SwapChain::DX12SwapChain(DX12Device& inDevice, HWND inHandle, const DX12CommandQueue& inCommandQueue, ui32 inWidth, ui32 inHeight)
	: m_VSync(true)
{
	m_TearingSupported = CheckTearingSupport();

	IDXGIFactory4* dxgi_factory4;
	ui32 create_factory_flags = 0;
#if defined(_DEBUG)
	create_factory_flags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(create_factory_flags, IID_PPV_ARGS(&dxgi_factory4)));

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.Width		= inWidth;
	swap_chain_desc.Height		= inHeight;
	swap_chain_desc.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.Stereo		= false;
	swap_chain_desc.SampleDesc	= { 1, 0 };
	swap_chain_desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount	= NUM_BUFFERED_FRAMES;
	swap_chain_desc.Scaling		= DXGI_SCALING_STRETCH;
	swap_chain_desc.SwapEffect	= DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.AlphaMode	= DXGI_ALPHA_MODE_UNSPECIFIED;

	// It is recommended to always allow tearing if tearing support is available.
	swap_chain_desc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* swap_chain1;
	ThrowIfFailed(dxgi_factory4->CreateSwapChainForHwnd(
		inCommandQueue.GetD3D12CommandQueue(),
		inHandle,
		&swap_chain_desc,
		nullptr,
		nullptr,
		&swap_chain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgi_factory4->MakeWindowAssociation(inHandle, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swap_chain1->QueryInterface(__uuidof(IDXGISwapChain4), (void **) &m_SwapChain));

	swap_chain1->Release();
	dxgi_factory4->Release();

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors	= NUM_BUFFERED_FRAMES;
	desc.Type			= D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(inDevice.m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
	m_RTVDescriptorSize = inDevice.m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	bool first_call = true;
	UpdateRenderTargetViews(inDevice, inWidth, inHeight, first_call);
}

DX12SwapChain::~DX12SwapChain()
{
	m_RTVDescriptorHeap->Release();

	for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		m_BackBuffers[i]->Release();
	}

	m_SwapChain->Release();
}

void DX12SwapChain::UpdateRenderTargetViews(DX12Device& inDevice, ui32 inClientWidth, ui32 inClientHeight, bool inFirstCall/* = false*/)
{
	if (!inFirstCall)
	{
		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		inDevice.Flush();

		for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			m_BackBuffers[i]->Release();
			m_BackBuffers[i] = nullptr;
			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swap_chain_desc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(NUM_BUFFERED_FRAMES, inClientWidth, inClientHeight, swap_chain_desc.BufferDesc.Format, swap_chain_desc.Flags));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}

	i32 rtvDescriptorSize = inDevice.m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		ID3D12Resource* back_buffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));

		inDevice.m_Device->CreateRenderTargetView(back_buffer, nullptr, rtv_handle);

		m_BackBuffers[i] = back_buffer;

		rtv_handle.Offset(rtvDescriptorSize);
	}
}

void DX12SwapChain::ClearBackBuffer(ID3D12GraphicsCommandList2* inCommandList)
{
	auto back_buffer = m_BackBuffers[m_CurrentBackBufferIndex];

	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		inCommandList->ResourceBarrier(1, &barrier);

		float clear_color[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);

		inCommandList->ClearRenderTargetView(rtv, clear_color, 0, nullptr);
	}
}

void DX12SwapChain::Present(ID3D12GraphicsCommandList2* inCommandList, DX12CommandQueue* inCommandQueue)
{
	auto backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	inCommandList->ResourceBarrier(1, &barrier);

	m_FrameFenceValues[m_CurrentBackBufferIndex] = inCommandQueue->ExecuteCommandList(inCommandList);

	ui32 sync_interval = m_VSync ? 1 : 0;
	ui32 present_flags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(sync_interval, present_flags));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	inCommandQueue->WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
}

void DX12SwapChain::SetRenderTarget(ID3D12GraphicsCommandList2* inCommandList, const DX12DepthRenderTarget* inDepthBuffer)
{
	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
	const D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor_handle = inDepthBuffer->GetCPUDescriptorHandle();
	inCommandList->OMSetRenderTargets(1, &rtv, false, &cpu_descriptor_handle);
}

ui32 DX12SwapChain::GetCurrentBackBufferIndex()
{
	return m_CurrentBackBufferIndex;
}