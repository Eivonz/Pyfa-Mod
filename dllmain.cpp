#include <Windows.h>
#include "pch.h"


#pragma comment(lib, "Detours/lib.X64/detours.lib")
#include "Detours/include/detours.h"

#include "oleacc.dll.h"


// "Theme" colors
const DWORD clDbgR = RGB(0xFF, 0x0, 0x0);
const DWORD clDbgG = RGB(0x0, 0xFF, 0x0);
const DWORD clDbgB = RGB(0x00, 0x0, 0xFF);
const DWORD clTxt1 = 0xBCBCBC;
const DWORD clBg1 = 0x686868;
const DWORD clBg2 = 0x3e3e42;
const DWORD clBg3 = 0x2d2d30;
const DWORD clBg4 = 0x252526;
const DWORD clBg5 = 0x1e1e1e;




extern "C" typedef HRESULT(WINAPI* t_DwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
void DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) {
    HMODULE shell;

    shell = LoadLibrary(L"dwmapi.dll");
    if (shell) {
        t_DwmSetWindowAttribute set_window_attribute = reinterpret_cast<t_DwmSetWindowAttribute>(GetProcAddress(shell, "DwmSetWindowAttribute"));
        set_window_attribute(hwnd, dwAttribute, pvAttribute, cbAttribute);

        FreeLibrary(shell);
    }
}





/*
DWORD GetSysColor(
    [in] int nIndex
);
*/
using Prototype_GetSysColor = DWORD (WINAPI*)(int nIndex);
Prototype_GetSysColor org_GetSysColor = GetSysColor;

int _GetSysColor(int nIndex) {

    switch (nIndex) {
        case 4:                         // Main window frame background or outline
            return clBg1;              // Dark
        case 5:                         // Window background. The associated foreground colors are COLOR_WINDOWTEXT and COLOR_HOTLITE. 
        case 15:                        // Face color for three-dimensional display elements and for dialog box backgrounds. The associated foreground color is COLOR_BTNTEXT. Windows 10 or greater: This value is not supported.
            return clBg2;
        case 8:                         // Fitting & fit text color
            return clTxt1;
        case 18:                        // Right side text color?
            return clTxt1;
        case 13:                        // Fitting selected element background color
            return org_GetSysColor(nIndex);
        case 14:                        // Fitting & fit selected item text color
            return org_GetSysColor(nIndex);
    }
    return org_GetSysColor(nIndex);
}


/*
HWND CreateWindowExW(
  [in]           DWORD     dwExStyle,
  [in, optional] LPCWSTR   lpClassName,
  [in, optional] LPCWSTR   lpWindowName,
  [in]           DWORD     dwStyle,
  [in]           int       X,
  [in]           int       Y,
  [in]           int       nWidth,
  [in]           int       nHeight,
  [in, optional] HWND      hWndParent,
  [in, optional] HMENU     hMenu,
  [in, optional] HINSTANCE hInstance,
  [in, optional] LPVOID    lpParam
);

*/
using Prototype_CreateWindowExW = HWND(WINAPI*)(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
Prototype_CreateWindowExW org_CreateWindowExW = CreateWindowExW;

HWND _CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {

    HWND hWND = org_CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // Undocumented behavior w. constant 20 (This will have DWM draw the title bar black, so using the dark color scheme)
    BOOL value = TRUE;
    DwmSetWindowAttribute(hWND, 20, &value, sizeof(BOOL));

    return hWND;
}


bool onAttach(HMODULE hModule) {

    DisableThreadLibraryCalls(hModule);
    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    DetourAttach(&(PVOID&)org_GetSysColor, _GetSysColor);
    DetourTransactionCommit();

    return true;
}

void onDetach() {

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    DetourDetach(&(PVOID&)org_GetSysColor, _GetSysColor);
    DetourTransactionCommit();

    FreeLibrary(oleacc::hTARGETDLL);
}



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{


    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            oleacc::LoadProcAddresses();
            onAttach(hModule);
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            onDetach();
            break;
    }
    return TRUE;
}

