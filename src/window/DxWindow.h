#pragma once

#include "dx/Dx.h"
#include "dx/DxDescriptor.h"
#include "rendering/RenderUtils.h"
#include "window.h"

struct dx_window : win32_window
{
	dx_window() = default;
	dx_window(dx_window&) = delete;
	dx_window(dx_window&&) = default;

	virtual ~dx_window();

	bool initialize(const TCHAR* name, uint32 requestedClientWidth, uint32 requestedClientHeight, color_depth colorDepth = color_depth_8, bool exclusiveFullscreen = false);

	virtual void shutdown();

	virtual void swapBuffers();
	virtual void toggleFullscreen();
	void toggleVSync();

	virtual void onResize();
	virtual void onMove();
	virtual void onWindowDisplayChange();


	Dx12Resource backBuffers[NUM_BUFFERED_FRAMES];
	DxRtvDescriptorHandle backBufferRTVs[NUM_BUFFERED_FRAMES];
	uint32 currentBackbufferIndex;

	color_depth colorDepth;

private:
	void updateRenderTargetViews();

	Dx12Swapchain swapchain;
	com<ID3D12DescriptorHeap> rtvDescriptorHeap;


	bool tearingSupported;
	bool exclusiveFullscreen;
	bool hdrSupport;
	bool vSync = false;
	bool initialized = false;
};
