#pragma once

bool LoadContent(uint32 inWidth, uint32 inHeight);
void UnloadContent();

void OnUpdate(uint32 inWidth, uint32 inHeight, float inDeltaT);
void OnRender();


void OnResize(uint32 inWidth, uint32 inHeight);