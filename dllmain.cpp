#include <Windows.h>
//#include <dwmapi.h>
#include "pch.h"
#include "dllexports.h"

#include "iathook.h"
#include "INIReader/INIReader.h"
#include "logging.h"

#pragma comment(lib, "Detours/lib.X64/detours.lib")
#include "Detours/include/detours.h"

#define DLL_OLEACC
//#define DLL_VERSION
//#define DLL_DEFINITION ""

/* ------------------------------------------------------------------------------------------------------------------ */

// Reverse endianess and shift right 1 byte to match format of COLORREF by RGB(r,g,b)
#define SHRB(X) ((X & 0xffffffff) >> 8)
#define RGBDW(X) SHRB((X & 0x000000ff) << 24 | (X & 0x0000ff00) << 8 | (X & 0x00ff0000) >> 8 | (X & 0xff000000) >> 24)
#define wsizeof(array) (sizeof(array)/sizeof(array[0]))

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
 
struct configuration_s {
    DWORD ColorText = clTxt1;
    DWORD ColorBackground = clBg1;
    bool UseExperimentalDarkmode = true;
    bool EnableLogging = true;

    const LPCWSTR _dllfile = L"oleacc.dll";
    const char* _logfile = "oleacc.log";
    const char* _inifile = "oleacc.ini";
    const wchar_t* _windowClassName = L"wxWindowNR";
    //const wchar_t* _windowTitleStr = L"pyfa v2.46.0 - Python Fitting Assistant";
    const wchar_t* _windowTitleModified = L" [Darkmode]";
    bool _windowMainWindowFixed = false;
};
configuration_s Configuration;

std::ofstream Log::LOG;


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
            return RGBDW(Configuration.ColorBackground);
        case 5:                         // Window background. The associated foreground colors are COLOR_WINDOWTEXT and COLOR_HOTLITE. 
        case 15:                        // Face color for three-dimensional display elements and for dialog box backgrounds. The associated foreground color is COLOR_BTNTEXT. Windows 10 or greater: This value is not supported.
            return RGBDW(Configuration.ColorBackground);
        case 8:                         // Fitting & fit text color
            return RGBDW(Configuration.ColorText);
        case 18:                        // Right side text color?
            return RGBDW(Configuration.ColorText);
        case 13:                        // Fitting selected element background color
            return org_GetSysColor(nIndex);
        case 14:                        // Fitting & fit selected item text color
            return org_GetSysColor(nIndex);
    }
    return org_GetSysColor(nIndex);
}


/*
    HBRUSH GetSysColorBrush(
      [in] int nIndex
    );
*/
using Prototype_GetSysColorBrush = HBRUSH(WINAPI*)(int nIndex);
Prototype_GetSysColorBrush org_GetSysColorBrush = GetSysColorBrush;

HBRUSH _GetSysColorBrush(int nIndex) {
/*
    5       COLOR_WINDOW
    8       COLOR_WINDOWTEXT
    16      COLOR_3DSHADOW / COLOR_BTNSHADOW
    17      COLOR_GRAYTEXT
    18      COLOR_BTNTEXT
*/
    if (nIndex == 5) {
        return CreateSolidBrush(RGB(0xFF,0x0,0x0));
    }
    return org_GetSysColorBrush(nIndex);
}


/*
    HBRUSH CreateSolidBrush(
      [in] COLORREF color
    );
*/
using Prototype_CreateSolidBrush = HBRUSH(WINAPI*)(COLORREF color);
Prototype_CreateSolidBrush org_CreateSolidBrush = CreateSolidBrush;

HBRUSH _CreateSolidBrush(COLORREF color) {
    return org_CreateSolidBrush(color);
}


/*
    int FillRect(
        [in] HDC        hDC,
        [in] const RECT *lprc,
        [in] HBRUSH     hbr
    );
*/
HBRUSH dbgRedBrush = CreateSolidBrush(RGB(0xFF, 0x0, 0x0));

using Prototype_FillRect = int(WINAPI*)(HDC hDC, const RECT* lprc, HBRUSH hbr);
Prototype_FillRect org_FillRect = FillRect;

int _FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr) {
    //return org_FillRect(hDC, lprc, dbgRedBrush);
    return org_FillRect(hDC, lprc, hbr);
}


/*
    COLORREF SetBkColor(
      [in] HDC      hdc,
      [in] COLORREF color
    );
*/
using Prototype_SetBkColor = COLORREF(WINAPI*)(HDC hdc, COLORREF color);
Prototype_SetBkColor org_SetBkColor = SetBkColor;

COLORREF _SetBkColor(HDC hdc, COLORREF color) {
    return org_SetBkColor(hdc, color);
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

    if (!Configuration._windowMainWindowFixed) {
        if (hWND != NULL && lpClassName != NULL && lpWindowName != NULL) {
            if (hWndParent == NULL) {           // Indicates a "main/top" window
                WCHAR Text[1024];
                int len = wsizeof(Text) - (sizeof(Configuration._windowTitleModified)/sizeof(wchar_t)) - 2;
                GetWindowTextW(hWND, Text, len);
                wcscat_s(Text, Configuration._windowTitleModified);
                SetWindowTextW(hWND, Text);

                Log() << "Setting experimental window attribute for window: " << Text;
                // Undocumented behavior w. constant 20 (This will have DWM draw the title bar black, so using the dark color scheme:)
                // https://gist.github.com/valinet/6afb524426635df9dbe2a9035701fcf4
                BOOL value = TRUE;
                DwmSetWindowAttribute(hWND, 20, &value, sizeof(BOOL));

                //    SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);

                Configuration._windowMainWindowFixed = true;
            }
        }
    }

    return hWND;
}


// Load optional .ini file
void LoadConfiguration() {
    INIReader reader(Configuration._inifile);

    if (reader.ParseError() < 0) {
        return;
    }

    if (reader.HasValue("settings", "EnableLogging")) {
        Configuration.EnableLogging = reader.GetBoolean("settings", "EnableLogging", true);
        if (Configuration.EnableLogging) {
            Log().open(Configuration._logfile);
        }
    }

    Log() << "Initialized settings:";
    //Log() << " - EnableLogging:" << std::boolalpha << Configuration.EnableLogging;
    if (reader.HasValue("settings", "ColorBackground")) {
        Configuration.ColorBackground = reader.GetInteger("settings", "ColorBackground", clBg1);
        Log() << " - ColorBackground: 0x" << std::hex << Configuration.ColorBackground;
    }
    if (reader.HasValue("settings", "ColorText")) {
        Configuration.ColorText = reader.GetInteger("settings", "ColorText", clTxt1);
        Log() << " - ColorText: 0x" << std::hex << Configuration.ColorText;
    }
    if (reader.HasValue("settings", "UseExperimentalDarkmode")) {
        Configuration.UseExperimentalDarkmode = reader.GetBoolean("settings", "UseExperimentalDarkmode", true);
        Log() << " - UseExperimentalDarkmode:" << std::boolalpha << Configuration.UseExperimentalDarkmode;
    }

}


bool onAttach(HMODULE hModule) {

    DisableThreadLibraryCalls(hModule);
    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (Configuration.UseExperimentalDarkmode) {
        DetourAttach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    }
    DetourAttach(&(PVOID&)org_GetSysColor, _GetSysColor);
//    DetourAttach(&(PVOID&)org_GetSysColorBrush, _GetSysColorBrush);
//    DetourAttach(&(PVOID&)org_FillRect, _FillRect);
    DetourTransactionCommit();

    return true;
}

void onDetach() {

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (Configuration.UseExperimentalDarkmode) {
        DetourDetach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    }
    DetourDetach(&(PVOID&)org_GetSysColor, _GetSysColor);
//    DetourDetach(&(PVOID&)org_GetSysColorBrush, _GetSysColorBrush);
//    DetourDetach(&(PVOID&)org_FillRect, _FillRect);
    DetourTransactionCommit();

    #ifdef DLL_OLEACC
    FreeLibrary(oleacc::hTARGETDLL);
    #endif

    #ifdef DLL_VERSION
    FreeLibrary(version::hTARGETDLL);
    #endif  
}
/*
    TODO: Fix the conditional definition file linking (see linker section in .vcxproj)
*/
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {

    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            LoadConfiguration();
            
            #ifdef DLL_OLEACC
            if (!oleacc::LoadProcAddresses(Configuration._dllfile))
                break;
            #endif

            #ifdef DLL_VERSION
            if (!version::LoadProcAddresses(Configuration._dllfile))
                break;
            #endif

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

