#include "Engine.h"
#include "Test.h"

#include <iostream>

// This file will be used to prototype.
// Once a feature is complete, we can create additional files
// based on this one

#include "Math/Math.h"
#include "Math/Vec4.h"
#include "Math/Mat4.h"

#include "DX12/DX12Device.h"
#include "DX12/DX12DescriptorHeap.h"
#include "DX12/DX12Resource.h"
#include "DX12/DX12RenderTarget.h"
#include "DX12/DX12CommandQueue.h"
#include "DX12/DX12Texture.h"

#include "Gfx/Mesh.h"
#include "Gfx/MeshLoader.h"
#include "Gfx/TextureLoader.h"
#include "Gfx/DrawableObject.h"

#include "Utils/Mouse.h"

#include "Shaders/Include/ConstantBuffers.h"

// Constant Buffer for the vertex shader
DX12ConstantBuffer* m_ConstantBuffer = nullptr;

// Depth buffer.
DX12DepthRenderTarget* m_DepthBuffer = nullptr;

DX12Texture* m_DummyTexture = nullptr;

std::vector<DrawableObject*> m_DrawableObjects;

D3D12_VIEWPORT	m_Viewport;
D3D12_RECT		m_ScissorRect;

float	m_FOV;
Mat4	m_ModelMatrix;
Mat4	m_ViewMatrix;
Mat4	m_ProjectionMatrix;

Vec4	m_SavedPosition;

bool	m_ContentLoaded;

void ResizeBuffers(DX12Device& inDevice, int inNewWidth, int inNewHeight)
{
	inDevice.ResestDescriptorHeaps();

	// Recreate swapchain buffers (causes a flush)
	inDevice.GetSwapChain()->UpdateRenderTargetViews(inDevice, inNewWidth, inNewHeight);

	if (m_DepthBuffer)
	{
		delete m_DepthBuffer;
	}
	m_DepthBuffer = new DX12DepthRenderTarget();

	DX12DescriptorHeap* descriptor_heap = inDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	uint32 heap_index = descriptor_heap->GetFreeIndex();

	m_DepthBuffer->InitAsDepthStencilBuffer(inDevice, descriptor_heap->GetCPUHandle(heap_index), inNewWidth, inNewHeight);
}

bool LoadContent(DX12Device& inDevice, uint32 inWidth, uint32 inHeight)
{
	{
		m_ScissorRect		= CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
		m_Viewport			= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight));
		m_FOV				= 45.0f;
		m_ContentLoaded		= false;

		const Vec4 eye_position(10.0f, 3.5f, 0.0f, 0);
		float eye_distance = 10.0f;
		m_SavedPosition		= Vec4::Normalize(eye_position) * eye_distance;
	}

	MeshLoader::Init();
	TextureLoader::Init();

	auto* command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); // Don't use COPY for this.
	auto* command_list	= command_queue->GetCommandList(inDevice);

	// Create the depth buffer
	ResizeBuffers(inDevice, inWidth, inHeight);

	// Create drawable objects
	{
		MeshLoader mesh_loader;
		mesh_loader.LoadFromFile("Data\\Cornell_fake_box.obj");
		Mesh* bulb_mesh = mesh_loader.CreateMeshObject(inDevice, command_list);
		m_DrawableObjects.push_back(DrawableObject::CreateDrawableObject(inDevice, bulb_mesh));
	}
	{
		Trace("=======\n\n\n=======");
		MeshLoader mesh_loader;
		mesh_loader.LoadFromFile("Data\\LightBulb.obj");
		Mesh* bulb_mesh = mesh_loader.CreateMeshObject(inDevice, command_list);
		m_DrawableObjects.push_back(DrawableObject::CreateDrawableObject(inDevice, bulb_mesh));
	}

	// Load texture
	TextureLoader texture_loader;
	texture_loader.LoadFromFile("Data\\render_1024.png");
	m_DummyTexture = texture_loader.CreateTexture(inDevice, command_list);

	// Create Constant Buffer View
	m_ConstantBuffer = new DX12ConstantBuffer();
	m_ConstantBuffer->InitAsConstantBuffer(inDevice, sizeof(ConstantBuffer::ModelViewProjection));

	auto fence_value = command_queue->ExecuteCommandList(command_list);
	command_queue->WaitForFenceValue(fence_value);

	m_ContentLoaded = true;

	return true;
}

void UnloadContent(DX12Device& inDevice)
{
	// Make sure the command queue has finished all commands before closing.
	inDevice.Flush();

	// Delete all drawable objects
	for (DrawableObject* d : m_DrawableObjects)
	{
		delete d;
	}
	m_DrawableObjects.clear();

	delete m_ConstantBuffer;

	delete m_DepthBuffer;

	delete m_DummyTexture;

	m_ContentLoaded = false;
}

void OnResize(DX12Device& inDevice, uint32 inWidth, uint32 inHeight)
{
	inWidth		= Math::Max(256u, inWidth);
	inHeight	= Math::Max(256u, inHeight);
	m_Viewport	= CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(inWidth), static_cast<float>(inHeight));

	ResizeBuffers(inDevice, inWidth, inHeight);
}

float TTT = 0.0f;

void OnUpdate(uint32 inWidth, uint32 inHeight, float inDeltaT)
{
	Mouse::GetInstance().UpdateWindowSize(inWidth, inHeight);
	Mouse::GetInstance().Update();

	TTT += inDeltaT;

	// Update the model matrix.
	const Vec4 rotation_axis(0, 1, 0, 0);
	m_ModelMatrix = Mat4::CreateRotationMatrix(rotation_axis, Math::ToRadians(180.0f));

	Vec4 eye_position = m_SavedPosition;
	const Vec4 focus_point(0, 3.5f, 0, 0);
	const Vec4 up_direction(0, 1, 0, 0);

	Mouse& mouse = Mouse::GetInstance();
	if (mouse.IsButtonDown(MouseButton::Middle) || mouse.WasJustReleased(MouseButton::Middle))
	{
		MousePos click_pos		= mouse.GetNormalizedClickPos(MouseButton::Middle);
		MousePos mouse_pos		= mouse.GetNormalizedPos();
		MousePos mouse_movement	= { click_pos.x - mouse_pos.x, click_pos.y - mouse_pos.y };

		Vec4 local_point(eye_position - focus_point);
		float x = local_point.X();
		float y = local_point.Y();
		float z = local_point.Z();
		float r = local_point.Length();

		float phi	= Math::Atan2(z, x);
		float theta	= Math::Atan2(Math::Sqrt(x*x + z*z), y);

		float camera_movement_speed = 1.0f;
		phi		+= mouse_movement.x * camera_movement_speed * Math::Pi;
		theta	-= mouse_movement.y * camera_movement_speed * Math::Pi;

		// Restrain Theta to avoid flipping camera at the poles
		// TODO: Use Quaternions instead
		const float limit = 0.1f;
		theta = Math::Clamp(theta, limit, Math::Pi - limit);

		local_point.X() = r * Math::Sin(theta) * Math::Cos(phi);
		local_point.Y() = r * Math::Cos(theta);
		local_point.Z() = r * Math::Sin(theta) * Math::Sin(phi);

		// Preserve the correct distance
		eye_position = Vec4::Normalize(local_point + focus_point) * m_SavedPosition.Length();
	}

	// Update the view matrix.
	m_ViewMatrix = Mat4::CreateLookAtMatrix(eye_position, focus_point, up_direction);

	// Release middle click means we need to save the new position
	if (mouse.WasJustReleased(MouseButton::Middle))
	{
		m_SavedPosition		= eye_position;
	}

	//Trace("%f", m_SavedPosition.Length());

	// Update the projection matrix.
	float aspect_ratio = inWidth / static_cast<float>(inHeight);
	m_ProjectionMatrix = Mat4::CreatePerspectiveMatrix(Math::ToRadians(m_FOV), aspect_ratio, 0.1f, 100.0f);
}

void OnRender(DX12Device& inDevice)
{
	auto* command_queue	= inDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto* command_list	= command_queue->GetCommandList(inDevice);
	auto* swap_chain	= inDevice.GetSwapChain();

	// Clear the render targets.
	{
		swap_chain->ClearBackBuffer(command_list);
		m_DepthBuffer->ClearBuffer(command_list);
	}

	command_list->RSSetViewports(1, &m_Viewport);
	command_list->RSSetScissorRects(1, &m_ScissorRect);

	swap_chain->SetRenderTarget(command_list, m_DepthBuffer);

	// TODO: Pre multiply matrix
	ConstantBuffer::ModelViewProjection mvp;
	mvp.Model		= m_ModelMatrix;
	mvp.View		= m_ViewMatrix;
	mvp.Projection	= m_ProjectionMatrix;

	for (DrawableObject* d : m_DrawableObjects)
	{
		d->SetupBindings(command_list);

		// Set texture
		{
			// Set the descriptor heap containing the texture srv
			ID3D12DescriptorHeap* heaps[] = { inDevice.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetD3DDescriptorHeap() };
			command_list->SetDescriptorHeaps(1, heaps);

			// Set slot 0 of our root signature to point to our descriptor heap with the texture SRV
			command_list->SetGraphicsRootDescriptorTable(0, m_DummyTexture->GetGPUHandle());
		}

		// Upload Constant Buffer to GPU
		m_ConstantBuffer->UpdateBufferResource(inDevice, command_list, sizeof(ConstantBuffer::ModelViewProjection), &mvp);
		m_ConstantBuffer->SetConstantBuffer(command_list, 1);

		d->Render(command_list);
	}

	// Present
	inDevice.Present(command_list);
}
