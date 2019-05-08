#include "engine_precomp.h"
#include "dx12swapchain.h"

#include "dx12device.h"
#include "D3dx12.h"

#include <cassert>
#include <exception>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

bool CheckTearingSupport()
{
	bool allowTearing = false;

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
			result = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
			if (result != S_OK)
			{
				allowTearing = false;
			}
			factory5->Release();
		}
		factory4->Release();
	}

	return allowTearing;
}

DX12SwapChain::DX12SwapChain(DX12Device& device, HWND hWnd, DX12CommandQueue* commandQueue, ui32 width, ui32 height)
	: m_VSync(false)
{
	m_TearingSupported = CheckTearingSupport();

	IDXGIFactory4* dxgiFactory4;
	ui32 createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = NUM_BUFFERED_FRAMES;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	// It is recommended to always allow tearing if tearing support is available.
	swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue->GetD3D12CommandQueue(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1->QueryInterface(__uuidof(IDXGISwapChain4), (void **) &m_SwapChain));
	
	swapChain1->Release();
	dxgiFactory4->Release();

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = NUM_BUFFERED_FRAMES;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(device.m_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_RTVDescriptorHeap)));
	m_RTVDescriptorSize = device.m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews(device, width, height, true);
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

void DX12SwapChain::UpdateRenderTargetViews(DX12Device& device, ui32 clientWidth, ui32 clientHeight, bool firstCall/* = false*/)
{
	if (!firstCall)
	{
		// Flush the GPU queue to make sure the swap chain's back buffers
		// are not being referenced by an in-flight command list.
		device.Flush();

		for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			m_BackBuffers[i]->Release();
			m_BackBuffers[i] = nullptr;
			m_FrameFenceValues[i] = m_FrameFenceValues[m_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_SwapChain->ResizeBuffers(NUM_BUFFERED_FRAMES, clientWidth, clientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();
	}

	auto rtvDescriptorSize = device.m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i)
	{
		ID3D12Resource* backBuffer;
		ThrowIfFailed(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		device.m_Device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

		m_BackBuffers[i] = backBuffer;

		rtvHandle.Offset(rtvDescriptorSize);
	}
}

void DX12SwapChain::ClearBackBuffer(ID3D12GraphicsCommandList2* commandList)
{
	auto backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ResourceBarrier(1, &barrier);

		float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);

		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}
}

void DX12SwapChain::Present(ID3D12GraphicsCommandList2* commandList, DX12CommandQueue* commandQueue)
{
	auto backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrier);

	m_FrameFenceValues[m_CurrentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

	ui32 syncInterval = m_VSync ? 1 : 0;
	ui32 presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(m_SwapChain->Present(syncInterval, presentFlags));

	m_CurrentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

	commandQueue->WaitForFenceValue(m_FrameFenceValues[m_CurrentBackBufferIndex]);
}

void DX12SwapChain::SetRenderTarget(ID3D12GraphicsCommandList2* commandList, DX12DepthRenderTarget* depthBuffer)
{
	const CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_CurrentBackBufferIndex, m_RTVDescriptorSize);
	commandList->OMSetRenderTargets(1, &rtv, false, &depthBuffer->GetCPUDescriptorHandle());
}