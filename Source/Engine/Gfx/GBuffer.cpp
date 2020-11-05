#include "Engine.h"
#include "Gfx/GBuffer.h"

#include "DX12/DX12RenderTarget.h"
#include "DX12/DX12DescriptorHeap.h"

GBuffer::GBuffer()
{
	// TODO: Hardoding to 1 for now
	m_NumRenderTargets = 1;

	m_DepthBuffer = new DX12DepthBuffer;

	uint32 rt_index = 0;
	for (; rt_index < m_NumRenderTargets; ++rt_index)
		m_RenderTargets[rt_index] = new DX12RenderTarget;

	for (; rt_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++rt_index)
		m_RenderTargets[rt_index] = nullptr;
}

GBuffer::~GBuffer()
{
	if (m_DepthBuffer)
		delete m_DepthBuffer;

	for (uint32 i = 0; i < m_NumRenderTargets; ++i)
	{
		delete m_RenderTargets[i];
		m_RenderTargets[i] = nullptr;
	}
}

void GBuffer::ReleaseResources()
{
	m_DepthBuffer->ReleaseResources();

	for (uint32 i = 0; i < m_NumRenderTargets; ++i)
		m_RenderTargets[i]->ReleaseResources();
}

void GBuffer::AllocateResources(DX12Device& inDevice, uint32 inTargetWidth, uint32 inTargetHeight)
{
	// All the resources inside the GBuffer share the same dimensions
	m_Width		= inTargetWidth;
	m_Height	= inTargetHeight;

	// Allocate depth buffer
	m_DepthBuffer->InitAsDepthStencilBuffer(inDevice, m_Width, m_Height);

	// Allocate render targets
	for (uint32 i = 0; i < m_NumRenderTargets; ++i)
		m_RenderTargets[i]->InitAsRenderTarget(inDevice, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void GBuffer::Set(ID3D12GraphicsCommandList2* inCommandList) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handles[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
	for (uint32 i = 0; i < m_NumRenderTargets; ++i)
		rtv_handles[i] = m_RenderTargets[i]->GetCPUDescriptorHandle();

	D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = m_DepthBuffer->GetCPUDescriptorHandle();

	// True means the handle passed in is the pointer to a contiguous range of NumRenderTargetDescriptors descriptors. 
	bool rt_single_handle_to_descriptor_range = false;
	inCommandList->OMSetRenderTargets(m_NumRenderTargets, &rtv_handles[0], rt_single_handle_to_descriptor_range, &dsv_handle);

	// TODO: Allow to set custom viewports and scissor rects
	D3D12_RECT		scissor_rect	= CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	D3D12_VIEWPORT	viewport		= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

	inCommandList->RSSetViewports(1, &viewport);
	inCommandList->RSSetScissorRects(1, &scissor_rect);
}

void GBuffer::ClearDepthBuffer(ID3D12GraphicsCommandList2* inCommandList) const
{
	m_DepthBuffer->ClearBuffer(inCommandList);
}

void GBuffer::ClearRenderTargets(ID3D12GraphicsCommandList2* inCommandList) const
{
	// TODO: Can we clear all targets at once?
	for (uint32 i = 0; i < m_NumRenderTargets; ++i)
		m_RenderTargets[i]->ClearBuffer(inCommandList);
}
