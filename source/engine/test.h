#pragma once

#include "D3dx12.h"
#include "dx12/dx12device.h"

bool LoadContent(DX12Device& inDevice, ui32 inWidth, ui32 inHeight);
void UnloadContent(DX12Device& inDevice);

void OnUpdate(ui32 inWidth, ui32 inHeight, float inDeltaT);
void OnRender(DX12Device& inDevice);


void OnResize(DX12Device& inDevice, ui32 inWidth, ui32 inHeight);