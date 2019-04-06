#pragma once

#include "D3dx12.h"
#include "dx12/dx12device.h"

bool LoadContent(DX12Device& dx12Device, ui32 width, ui32 height);
void UnloadContent();

void OnUpdate(ui32 width, ui32 height);
void OnRender(DX12Device& dx12Device);
void OnKeyPressed();

void OnMouseWheel();


void OnResize(DX12Device& device, ui32 width, ui32 height);