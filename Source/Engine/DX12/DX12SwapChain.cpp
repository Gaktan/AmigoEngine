#include "Engine.h"
#include "DX12/DX12SwapChain.h"

#include "DX12/DX12DescriptorHeap.h"
#include "DX12/DX12Device.h"

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
		result = factory4->QueryInterface(IID_PPV_ARGS(&factory5));

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

DX12SwapChain::DX12SwapChain(HWND inHandle, const DX12CommandQueue& inCommandQueue, uint32 inWidth, uint32 inHeight)
{
	m_TearingSupported = CheckTearingSupport();

	IDXGIFactory4* dxgi_factory4;
	uint32 create_factory_flags = 0;
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

	ThrowIfFailed(swap_chain1->QueryInterface(IID_PPV_ARGS(&m_D3DSwapChain)));

	swap_chain1->Release();
	dxgi_factory4->Release();

	m_CurrentBackBufferIndex = m_D3DSwapChain->GetCurrentBackBufferIndex();

	for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		m_BackBuffers[i] = new DX12RenderTarget();

	bool first_call = true;
	UpdateRenderTargetViews(inWidth, inHeight, first_call);
}

DX12SwapChain::~DX12SwapChain()
{
	for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		delete m_BackBuffers[i];

	m_D3DSwapChain->Release();
}

void DX12SwapChain::UpdateRenderTargetViews(uint32 inWidth, uint32 inHeight, bool inFirstCall/* = false*/)
{
	if (!inFirstCall)
	{
		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		g_RenderingDevice.Flush();

		for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		{
			// Any references to the back buffers must be released before the swap chain can be resized.
			m_BackBuffers[i]->ReleaseResources();
			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swap_chain_desc = {};
		ThrowIfFailed(m_D3DSwapChain->GetDesc(&swap_chain_desc));
		ThrowIfFailed(m_D3DSwapChain->ResizeBuffers(NUM_BUFFERED_FRAMES, inWidth, inHeight, swap_chain_desc.BufferDesc.Format, swap_chain_desc.Flags));

		m_CurrentBackBufferIndex = m_D3DSwapChain->GetCurrentBackBufferIndex();
	}

	const Vec4 clear_color = { 0.4f, 0.6f, 0.9f, 1.0f };
	for (uint32 i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		ID3D12Resource* back_buffer;
		ThrowIfFailed(m_D3DSwapChain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)));

		m_BackBuffers[i]->InitFromResource(back_buffer, DXGI_FORMAT_R8G8B8A8_UNORM, clear_color);
	}
}

void DX12SwapChain::ClearBackBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	auto back_buffer = m_BackBuffers[m_CurrentBackBufferIndex];

	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			back_buffer->GetResource(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		inCommandList->ResourceBarrier(1, &barrier);

		back_buffer->ClearBuffer(inCommandList);
	}
}

void DX12SwapChain::Present(ID3D12GraphicsCommandList2* inCommandList, DX12CommandQueue* inCommandQueue)
{
	auto back_buffer = m_BackBuffers[m_CurrentBackBufferIndex];

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(back_buffer->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	inCommandList->ResourceBarrier(1, &barrier);

	m_FrameFenceValues[m_CurrentBackBufferIndex] = inCommandQueue->ExecuteCommandList(inCommandList);

	uint32 sync_interval = m_VSync ? 1 : 0;
	uint32 present_flags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_D3DSwapChain->Present(sync_interval, present_flags));

	m_CurrentBackBufferIndex = m_D3DSwapChain->GetCurrentBackBufferIndex();

	inCommandQueue->WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
}

void DX12SwapChain::SetRenderTarget(ID3D12GraphicsCommandList2* inCommandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_BackBuffers[m_CurrentBackBufferIndex]->GetCPUDescriptorHandle();
	inCommandList->OMSetRenderTargets(1, &rtv, false, nullptr);

	// TODO: Allow to set custom viewports and scissor rects
	D3D12_RESOURCE_DESC desc = m_BackBuffers[m_CurrentBackBufferIndex]->GetResource()->GetDesc();

	D3D12_RECT		scissor_rect	= CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	D3D12_VIEWPORT	viewport		= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(desc.Width), static_cast<float>(desc.Height));

	inCommandList->RSSetViewports(1, &viewport);
	inCommandList->RSSetScissorRects(1, &scissor_rect);
}

uint32 DX12SwapChain::GetCurrentBackBufferIndex()
{
	return m_CurrentBackBufferIndex;
}