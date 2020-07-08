// dllmain.cpp : Defines the entry point for the DLL application.

/* Detours Include and Libraries */
#include <detours.h>
#pragma comment (lib, "detours.lib")

/* Standard Includes */
#include <Windows.h>
#include <iostream>

/* DirectX9 Includes and Libraries */
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

/* DearImGui Includes */
#include "ImGui Files/imgui.h"
#include "ImGui Files/imgui_impl_dx9.h"
#include "ImGui Files/imgui_impl_win32.h"

/* Function declarations*/
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(__stdcall* f_EndScene)(IDirect3DDevice9* pDevice);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
f_EndScene oEndScene;
WNDPROC oWndProc;

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

/* We detour EndScene to this function for rendering to screen*/
HRESULT __stdcall ourHookedFunct(IDirect3DDevice9* pDevice)
{

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(FindWindowA(NULL, ("Counter-Strike: Global Offensive")));
	ImGui_ImplDX9_Init(pDevice);

	/* Create a NewFrame*/
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	/* We render our menu in here*/
	ImGui::Begin("CoristatInternal Menu");

	ImGui::End();

	/* EndFrame and Render*/
	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	/* Return EndScene or else game will crash*/
	return oEndScene(pDevice);
}

/* In here we Detour Endscene(42 in vTable) with our ImGui Menu*/
DWORD WINAPI HookingThread(HMODULE hModule)
{	
	/* Create d3d and check if it failed*/
	IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION); 
	if (!d3d)
		return 1;

	D3DPRESENT_PARAMETERS d3dparam{ 0 };
	HWND window_handle = FindWindowA(0, ("Counter-Strike: Global Offensive"));
	oWndProc = (WNDPROC)SetWindowLongPtr(window_handle, GWLP_WNDPROC, (LONG_PTR)WndProc);

	/* D3D parameters*/
	d3dparam.hDeviceWindow = window_handle, d3dparam.SwapEffect = D3DSWAPEFFECT_DISCARD, d3dparam.Windowed = true;
	IDirect3DDevice9* d3dDevice;

	/* Check if the D3DDevice creation failed if not get the vTable*/
	if (FAILED(d3d->CreateDevice(0, D3DDEVTYPE_HAL, d3dparam.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dparam, &d3dDevice))){
		d3d->Release();
		return 1;
	}
		
	/* Get the vTable*/
	void** vTable = *reinterpret_cast<void***>(d3dDevice);

	/* We can give nullptr and release, because we dont need d3dDevice anymore*/
	if (d3dDevice)
		d3dDevice->Release(), d3dDevice = nullptr;

	/* We use MS Detours, to Detour EndScene to our ImGui Menu*/
	oEndScene = (f_EndScene)DetourFunction((PBYTE)vTable[42], (PBYTE)ourHookedFunct);
	
	return 0;
}

/* This is the entry point of the application (DllMain)*/
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved){                     
    switch (ul_reason_for_call){
    case DLL_PROCESS_ATTACH:
		/* We CreateThread when the dll_process_attach (when injected)*/
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)HookingThread, 0, 0, 0);
    }
    
    return TRUE;
}

