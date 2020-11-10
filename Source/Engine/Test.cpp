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

#include "Gfx/DrawableObject.h"
#include "Gfx/DrawUtils.h"
#include "Gfx/GBuffer.h"
#include "Gfx/Mesh.h"
#include "Gfx/MeshLoader.h"
#include "Gfx/ShaderObject.h"
#include "Gfx/TextureLoader.h"

#include "Utils/Mouse.h"

#include "Shaders/Include/ConstantBuffers.h"
#include "Shaders/Include/Shaders.h"

// Constant Buffer for the vertex shader
DX12ConstantBuffer* m_ConstantBuffer = nullptr;

GBuffer* m_GBuffer = nullptr;

DX12Texture* m_DummyTexture = nullptr;

std::map<std::string, ShaderObject*> m_AllShaderObjects;
RenderBuckets m_RenderBuckets;

float	m_FOV;
Mat4	m_ModelMatrix;
Mat4	m_ViewMatrix;
Mat4	m_ProjectionMatrix;

Vec4	m_SavedPosition;

bool	m_ContentLoaded;

void ResizeBuffers(int inNewWidth, int inNewHeight)
{
	// Recreate swapchain buffers (causes a flush)
	g_RenderingDevice.GetSwapChain()->UpdateRenderTargetViews(inNewWidth, inNewHeight);

	//Recreate GBuffer
	m_GBuffer->ReleaseResources();
	m_GBuffer->AllocateResources(inNewWidth, inNewHeight);
}

bool LoadContent(uint32 inWidth, uint32 inHeight)
{
	{
		m_FOV				= 45.0f;
		m_ContentLoaded		= false;

		const Vec4 eye_position(10.0f, 3.5f, 0.0f, 0);
		float eye_distance = 10.0f;
		m_SavedPosition		= Vec4::Normalize(eye_position) * eye_distance;
	}

	MeshLoader::Init();
	TextureLoader::Init();

	auto* command_queue	= g_RenderingDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT); // Don't use COPY for this.
	auto* command_list	= command_queue->GetCommandList();

	m_GBuffer = new GBuffer;

	// Buffers
	ResizeBuffers(inWidth, inHeight);

	// Create shader objects
	{
		m_AllShaderObjects["OpaqueGeometry"]	= new ShaderObject(RenderPass::OpaqueGeometry, InlineShaders::DefaultVS, InlineShaders::DefaultPS);
		m_AllShaderObjects["Transparent"]		= new ShaderObject(RenderPass::Transparent, InlineShaders::DefaultVS, InlineShaders::TransparentShader);
	}

	DrawUtils::Init(command_list);

	// Create drawable objects
	{
		MeshLoader mesh_loader;
		mesh_loader.LoadFromFile("Data\\Cornell_fake_box.obj");
		mesh_loader.Finalize(command_list, m_AllShaderObjects, m_RenderBuckets);
	}
	{
		MeshLoader mesh_loader;
		mesh_loader.LoadFromFile("Data\\LightBulb.obj");
		mesh_loader.Finalize(command_list, m_AllShaderObjects, m_RenderBuckets);
	}

	// Load texture
	TextureLoader texture_loader;
	texture_loader.LoadFromFile("Data\\render_1024.png");
	m_DummyTexture = texture_loader.CreateTexture(command_list);

	// Create Constant Buffer View
	m_ConstantBuffer = new DX12ConstantBuffer();
	m_ConstantBuffer->InitAsConstantBuffer(sizeof(ConstantBuffers::ModelViewProjection));

	auto fence_value = command_queue->ExecuteCommandList(command_list);
	command_queue->WaitForFenceValue(fence_value);

	m_ContentLoaded = true;

	return true;
}

void UnloadContent()
{
	// Make sure the command queue has finished all commands before closing.
	g_RenderingDevice.Flush();

	// Delete all drawable objects
	for (RenderBucket& bucket : m_RenderBuckets)
	{
		for (DrawableObject* d : bucket)
		{
			delete d;
		}
		bucket.clear();
	}

	// Delete all materials
	for (auto pair : m_AllShaderObjects)
	{
		delete pair.second;
	}

	DrawUtils::Destroy();

	delete m_ConstantBuffer;

	delete m_GBuffer;

	delete m_DummyTexture;

	m_ContentLoaded = false;
}

void OnResize(uint32 inWidth, uint32 inHeight)
{
	ResizeBuffers(inWidth, inHeight);
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

void SetupBindings(ID3D12GraphicsCommandList2* inCommandList)
{
	// TODO: This is whack
	auto& descriptor_heap = g_RenderingDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetDescriptorHeap();
	uint32 descriptor_index = descriptor_heap.Allocate();
	g_RenderingDevice.GetD3DDevice()->CopyDescriptorsSimple(1, descriptor_heap.GetCPUHandle(descriptor_index), m_DummyTexture->GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Set slot 0 of our root signature to point to our descriptor heap with the texture SRV
	inCommandList->SetGraphicsRootDescriptorTable(0, descriptor_heap.GetGPUHandle(descriptor_index));
}

void RenderGeometry(ID3D12GraphicsCommandList2* inCommandList)
{
	// TODO: Pre multiply matrix
	ConstantBuffers::ModelViewProjection mvp;
	mvp.Model		= m_ModelMatrix;
	mvp.View		= m_ViewMatrix;
	mvp.Projection	= m_ProjectionMatrix;

	for (DrawableObject* d : m_RenderBuckets[(int) RenderPass::OpaqueGeometry])
	{
		d->SetupBindings(inCommandList);

		SetupBindings(inCommandList);

		// Upload Constant Buffer to GPU
		m_ConstantBuffer->UpdateBufferResource(inCommandList, sizeof(ConstantBuffers::ModelViewProjection), &mvp);
		m_ConstantBuffer->SetConstantBuffer(inCommandList, 1);

		d->Render(inCommandList);
	}
}

void RenderTransparent(ID3D12GraphicsCommandList2* inCommandList)
{
	// TODO: Pre multiply matrix
	ConstantBuffers::ModelViewProjection mvp;
	mvp.Model		= m_ModelMatrix;
	mvp.View		= m_ViewMatrix;
	mvp.Projection	= m_ProjectionMatrix;

	for (DrawableObject* d : m_RenderBuckets[(int) RenderPass::Transparent])
	{
		d->SetupBindings(inCommandList);

		SetupBindings(inCommandList);

		// Upload Constant Buffer to GPU
		m_ConstantBuffer->UpdateBufferResource(inCommandList, sizeof(ConstantBuffers::ModelViewProjection), &mvp);
		m_ConstantBuffer->SetConstantBuffer(inCommandList, 1);

		d->Render(inCommandList);
	}
}

void CopyToBackBuffer(ID3D12GraphicsCommandList2* inCommandList)
{
	auto* swap_chain = g_RenderingDevice.GetSwapChain();

	// Clear will trigger a resource transition
	swap_chain->ClearBackBuffer(inCommandList);
	
	swap_chain->SetRenderTarget(inCommandList);

	DrawUtils::DrawFullScreenTriangle(inCommandList, m_GBuffer->GetRenderTarget(0));
}

void OnRender()
{
	auto* command_queue		= g_RenderingDevice.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto* command_list		= command_queue->GetCommandList();

	// Set the descriptor heap containing all textures
	ID3D12DescriptorHeap* heaps[] = { command_queue->GetDescriptorHeap().GetD3DDescriptorHeap() };
	command_list->SetDescriptorHeaps(1, heaps);

	// Clear the GBuffer targets.
	m_GBuffer->ClearDepthBuffer(command_list);
	m_GBuffer->ClearRenderTargets(command_list);

	m_GBuffer->Set(command_list);

	RenderGeometry(command_list);
	RenderTransparent(command_list);

	CopyToBackBuffer(command_list);

	// Present
	g_RenderingDevice.Present(command_list);
}
