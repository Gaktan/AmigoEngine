#pragma once

#include "D3dx12.h"
#include "DX12/DX12Device.h"

bool LoadContent(DX12Device& inDevice, uint32 inWidth, uint32 inHeight);
void UnloadContent(DX12Device& inDevice);

void OnUpdate(uint32 inWidth, uint32 inHeight, float inDeltaT);
void OnRender(DX12Device& inDevice);


void OnResize(DX12Device& inDevice, uint32 inWidth, uint32 inHeight);