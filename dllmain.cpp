#include <Windows.h>
#include "pch.h"
#include "dllexports.h"

#include "iathook.h"
#include "INIReader/INIReader.h"
#include "logging.h"

#pragma comment(lib, "Detours/lib.X64/detours.lib")
#include "Detours/include/detours.h"


#define DLL_OLEACC

#define WSC_STATIC 1
#define WSC_LISTVIEW 2
#define WSC_LISTVIEW_HEADER 3
#define WSC_PANEL 4
#define WSC_DIALOG 5
#define WSC_LISTVIEW_SPECIAL 6
#define WSC_CHECKBOX 7
#define WSC_DBG 99

// Reverse endianess and shift right 1 byte to match format of COLORREF by RGB(r,g,b)
#define SHRB(X) ((X & 0xffffffff) >> 8)
#define RGBDW(X) SHRB((X & 0x000000ff) << 24 | (X & 0x0000ff00) << 8 | (X & 0x00ff0000) >> 8 | (X & 0xff000000) >> 24)
#define DWTOCOLORREF(X) (((X & 0x000000FF) << 16) | (X & 0x0000FF00) | ((X & 0x00FF0000) >> 16))
#define wsizeof(array) (sizeof(array)/sizeof(array[0]))
#define RGB_DARKER(X) (X & 0xfefefe) >> 1
#define RGB_LIGHTER(X) (X & 0x7f7f7f) << 1

#define ROUND(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define BYTE_first(x) ((x >> 24) & 0xff000000)
#define BYTE_second(x) ((x >> 16) & 0x00ff0000)
#define BYTE_third(x) ((x >> 8) & 0x0000ff00)
#define BYTE_fourth(x) (x & 0x000000ff)
#define RGB_MANIPULATE(color, factor) ( ROUND(BYTE_first(color) * (float)factor) | ROUND(BYTE_second(color) * (float)factor) | ROUND(BYTE_third(color) * (float)factor) | ROUND(BYTE_fourth(color) * (float)factor) )

/* ------------------------------------------------------ Shared data segment  ------------------------------------------------------------ */
//The #pragma data_seg compiler directive asks the compiler to create a data segment which can be shared by all instances of the dll.
#pragma data_seg("SHARED")

//HWND hWndNotepad = nullptr;
HMODULE hModuleDLL = NULL;  // Set when DLL is loaded
HWND hWndMain = NULL;       // Set from CreateWindowEx hook

HWND hAdditionsWindow = NULL;                    // The splitter window parent to additions
HWND hWnd_ActiveFitting_SysListView32 = NULL;
HWND hWnd_ColorFitBySlot_CheckBox = NULL;

//BOOL MessageHookInstalled = false;
BOOL SubclassInstalled = false;

HHOOK hGetMsgHook;

#pragma data_seg()
#pragma comment(linker, "/section:SHARED,RWS")

/* ------------------------------------------------------ Declarations ------------------------------------------------------------ */

LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void UpdateWndCtrls(HWND hWnd);

LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT WindowProc_Static(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WindowProc_ListView(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool NotChildOfWindow(HWND hWnd, const wchar_t* className, const wchar_t* windowTitle);


extern "C" __declspec(dllexport) BOOL CALLBACK SetHook(HWND hWND, BOOL bInstall);


/* ------------------------------------------------------ Definitions ------------------------------------------------------------ */


class ThemePalette {
public:
    COLORREF dark = NULL;
    COLORREF midDark = NULL;
    COLORREF mid = NULL;
    COLORREF midLight = NULL;
    COLORREF light = NULL;

    COLORREF white = RGB(255, 255, 255);
    COLORREF black = RGB(0, 0, 0);

    HBRUSH dark_brush = NULL;
    HBRUSH midDark_brush = NULL;
    HBRUSH mid_brush = NULL;
    HBRUSH midLight_brush = NULL;
    HBRUSH light_brush = NULL;

    ThemePalette(DWORD(&colors)[5]) {
        int size = wsizeof(colors);
        if (size == 5) {
            this->dark = DWTOCOLORREF(colors[0]);
            this->midDark = DWTOCOLORREF(colors[1]);
            this->mid = DWTOCOLORREF(colors[2]);
            this->midLight = DWTOCOLORREF(colors[3]);
            this->light = DWTOCOLORREF(colors[4]);

            this->dark_brush = CreateSolidBrush(this->dark);
            this->midDark_brush = CreateSolidBrush(this->midDark);
            this->mid_brush = CreateSolidBrush(this->mid);
            this->midLight_brush = CreateSolidBrush(this->midLight);
            this->light_brush = CreateSolidBrush(this->light);

        }
    }
};

// array [dark, middark, mid, midlight, light]
DWORD Palette_BlackBeauty[] = { 0x26262a, 0x3c3c3f, 0x515155, 0x67676a, 0x7d7d7f };
DWORD Palette_Licorice[] = { 0x1a1110, 0x312928, 0x484140, 0x5f5858, 0x767070 };
DWORD Palette_LicoriceBlue[] = { 0x2e3749, 0x434b5b, 0x585f6d, 0x6d7380, 0x828792 };
DWORD Palette_Daintree[] = { 0x012731, 0x1a3d46, 0x34525a, 0x4d686f, 0x677d83 };


struct configuration_s {
    bool UseExperimentalDarkmode = true;
    bool EnableCustomControls = true;
    bool EnableLogging = false;

    // Must instantiate this, as there might be no ini file present!
    ThemePalette* Palette = new ThemePalette(Palette_BlackBeauty);

    const LPCWSTR _dllfile = L"oleacc.dll";
    const char* _logfile = "oleacc.log";
    const char* _inifile = "oleacc.ini";
    
    const wchar_t* _wndTitleStrPartial = L"Python Fitting Assistant";
    #define _WNDTITLEMODIFIED L" [Darkmode v" STR(VERSION_MAJOR) "." STR(VERSION_MINOR) "." STR(VERSION_MAJOR_UPDATE) "]"
    const wchar_t* _wndTitleModified = _WNDTITLEMODIFIED;

    bool _windowMainWindowFixed = false;
    bool _ColorFitBySlotEnabled = false;
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
        case 4:
            return Configuration.Palette->dark;
        case 5:
            return Configuration.Palette->dark;
        case 15:
        case 22: //alternating lines in compare list
            return Configuration.Palette->midDark;
        case 8:
            return Configuration.Palette->white;
        case 18:
            return Configuration.Palette->white;
        case 13:
            return org_GetSysColor(nIndex);
        case 14:
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

    if (!Configuration._windowMainWindowFixed && hWndParent == NULL) {
        if (hWND != NULL && lpClassName != NULL && lpWindowName != NULL) {
            hWndMain = hWND;

            WCHAR buf[1024];
            int len = wsizeof(buf) - (sizeof(Configuration._wndTitleModified) / sizeof(wchar_t)) - 2;
            GetWindowTextW(hWND, buf, len);
            wcscat_s(buf, Configuration._wndTitleModified);
            SetWindowTextW(hWND, buf);

            if (Configuration.UseExperimentalDarkmode) {
                Log() << "Setting experimental window attribute for window: " << buf;
                // Undocumented behavior w. constant 20 (This will have DWM draw the title bar black, so using the dark color scheme:)
                // https://gist.github.com/valinet/6afb524426635df9dbe2a9035701fcf4
                BOOL value = TRUE;
                DwmSetWindowAttribute(hWND, 20, &value, sizeof(BOOL));

                SetWindowTheme(hWND, L"Explorer", NULL);
            }

            Configuration._windowMainWindowFixed = true;

        }
    }

    // Modify specific controls, 
    if (Configuration.EnableCustomControls && hWndParent != NULL) {
        UpdateWndCtrls(hWND);
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
    Log() << " - EnableLogging: " << std::boolalpha << Configuration.EnableLogging;

    if (reader.HasValue("settings", "UseExperimentalDarkmode")) {
        Configuration.UseExperimentalDarkmode = reader.GetBoolean("settings", "UseExperimentalDarkmode", true);
        Log() << " - UseExperimentalDarkmode: " << std::boolalpha << Configuration.UseExperimentalDarkmode;
    }
    if (reader.HasValue("settings", "EnableCustomControls")) {
        Configuration.EnableCustomControls = reader.GetBoolean("settings", "EnableCustomControls", true);
        Log() << " - EnableCustomControls: " << std::boolalpha << Configuration.EnableCustomControls;
    }

    bool CustomThemeActive = false;
    if (reader.HasValue("settings", "OverrideThemePalette")) {
        auto x = reader.GetString("settings", "OverrideThemePalette", "");

        DWORD customPalette[5] = { 0 };
        auto pattern = R"([0-9a-fA-F]{6})";
        try {
            auto r = std::regex(pattern);
            std::smatch matches;
            int i = 0;
            while (regex_search(x, matches, r) && i < wsizeof(customPalette)) {
                customPalette[i] = std::stoul(matches[0].str(), nullptr, 16);
                i++;
                x = matches.suffix().str();
            }
            if (i == wsizeof(customPalette)) {
                Configuration.Palette = new ThemePalette(customPalette);
                CustomThemeActive = true;
                Log() << string_format(" - OverrideThemePalette: 0x%06X, 0x%06X, 0x%06X, 0x%06X, 0x%06X", customPalette[0], customPalette[1], customPalette[2], customPalette[3], customPalette[4]);
            }
        }
        catch (std::exception e) {
            //catch (const std::regex_error& e) {
            Log() << "Exception in OverrideThemePalette:";
            Log() << e.what();
        }
    }
    if (!CustomThemeActive && reader.HasValue("settings", "ActiveThemePalette")) {
        switch (reader.GetInteger("settings", "ActiveThemePalette", 0)) {
            case 1: // Palette_BlackBeauty
                // Default, and already instantiated
                Log() << " - ActiveThemePalette: BlackBeauty";
                break;
            case 2: // Palette_Licorice
                Configuration.Palette = new ThemePalette(Palette_Licorice);
                Log() << " - ActiveThemePalette: Licorice";
                break;
            case 3: // Palette_LicoriceBlue
                Configuration.Palette = new ThemePalette(Palette_LicoriceBlue);
                Log() << " - ActiveThemePalette: LicoriceBlue";
                break;
            case 4: // Palette_Daintree
                Configuration.Palette = new ThemePalette(Palette_Daintree);
                Log() << " - ActiveThemePalette: Daintree";
                break;

            default:
                // Default, and already instantiated
                Log() << " - ActiveThemePalette: BlackBeauty";
        }
    }
    else if (!CustomThemeActive) {
        // Default, and already instantiated
        Log() << " - ActiveThemePalette: BlackBeauty";
    }


}


bool onAttach(HMODULE hModule) {

    DisableThreadLibraryCalls(hModule);
    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (Configuration.UseExperimentalDarkmode || Configuration.EnableCustomControls) {
        DetourAttach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    }
    DetourAttach(&(PVOID&)org_GetSysColor, _GetSysColor);

    DetourTransactionCommit();

    return true;
}

void onDetach() {

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (Configuration.UseExperimentalDarkmode || Configuration.EnableCustomControls) {
        DetourDetach(&(PVOID&)org_CreateWindowExW, _CreateWindowExW);
    }
    DetourDetach(&(PVOID&)org_GetSysColor, _GetSysColor);

    DetourTransactionCommit();

    if (SubclassInstalled) {
        RemoveWindowSubclass(hWndMain, SubclassProc, 0);
        SubclassInstalled = FALSE;
    }

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
            hModuleDLL = hModule;
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

// Calculate best contrasting Black or White based on specified dominant background color
COLORREF GetContrastColorBW(COLORREF color) {
    int R = GetRValue(color);
    int G = GetGValue(color);
    int B = GetBValue(color);

    double luminance = (0.299 * R + 0.587 * G + 0.114 * B) / 255;

    return (luminance > 0.5 ? Configuration.Palette->black : Configuration.Palette->white);
}
COLORREF GetContrastColorBW(int R, int G, int B) {
    return GetContrastColorBW(RGB(R, G, B));
}


int GetWindowZIndex(HWND hWnd) {
    int z = 0;
    for (HWND h = hWnd; h != NULL; h = GetWindow(h, GW_HWNDPREV)) z++;
    return z;
}

bool NotChildOfWindow(HWND hWnd, const wchar_t* className, const wchar_t* windowTitle) {
    WCHAR winClassName[MAX_PATH * 2 + 1] = { 0 };
    WCHAR winTitle[MAX_PATH * 2 + 1] = { 0 };

    HWND curhWnd = hWnd;

    while (true) {
        curhWnd = GetParent(curhWnd);
        if (curhWnd == NULL)    // We reached the end, must be valid
            return true;

        if (className != NULL) {
            GetClassName(curhWnd, winClassName, MAX_PATH);
        }

        if (windowTitle != NULL) {
            GetWindowText(curhWnd, winTitle, MAX_PATH);
        }

        if (className != NULL && (wcsstr(winClassName, className) != NULL)) {
            return false;
        }

        if (windowTitle != NULL && (wcsstr(winTitle, windowTitle) != NULL)) {
            return false;
        }
    }
    return true;
}

bool WithinWindowDepth(HWND hTopWnd, HWND hWnd, UINT maxDepth) {
    unsigned int curDepth = 0;

    HWND curParenthWnd = hWnd;
    while(true) {
        curParenthWnd = GetParent(curParenthWnd);
        curDepth += 1;
        if (curParenthWnd == NULL || curParenthWnd == hTopWnd)  // we reached the top
            return true;
        if (curDepth > maxDepth)
            return false;
    }
    return true;
}


LRESULT WindowProc_DBG(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
    case WM_DESTROY:
    case WM_NCDESTROY:
    {
        // Remove own subclass when contrl is destroyed
        if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_DBG)) {
            //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
        }
    } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WindowProc_CheckBox(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    // Update based on the "visual" state....
    case BM_SETSTATE:
    {
        //Log() << "Setstate triggered: " << wParam;
        Configuration._ColorFitBySlotEnabled = !(BOOL)wParam;
    } break;
    case WM_DESTROY:
    case WM_NCDESTROY:
    {
        // Remove own subclass when contrl is destroyed
        if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_CHECKBOX)) {
            //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
        }
    } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT WindowProc_Dialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLOREDIT:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->white);
            return (LRESULT)GetStockObject(DC_BRUSH);
        } break;
        case WM_CTLCOLORDLG:
        {

            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->white);
            SetBkColor(hdc, Configuration.Palette->midDark);
            return (INT_PTR)Configuration.Palette->midDark_brush;
        } break;

        case WM_CTLCOLORSTATIC:
        {
            HWND hwndCtrl = (HWND)lParam;
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->white);
            SetBkColor(hdc, Configuration.Palette->midDark);
            return (INT_PTR)Configuration.Palette->midDark_brush;
        } break;
        case WM_ERASEBKGND:
        {
            RECT rc;
            HDC hdc = (HDC)wParam;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, Configuration.Palette->midDark_brush);
            return 1L;
        } break;
        case WM_DESTROY:
        case WM_NCDESTROY:
        {
            // Remove own subclass when contrl is destroyed
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_DIALOG)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
        } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WindowProc_Panel(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CTLCOLORSTATIC:
        {
            HWND hwndCtrl = (HWND)lParam;
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->dark);
            SetBkColor(hdc, Configuration.Palette->light);
            return (INT_PTR)Configuration.Palette->light_brush;
        } break;
        // https://learn.microsoft.com/en-us/windows/win32/gdi/drawing-a-custom-window-background
        case WM_ERASEBKGND:
        {
            RECT rc;
            HDC hdc = (HDC)wParam;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, Configuration.Palette->light_brush);
            return 1L;
        } break;
        case WM_DESTROY:
        case WM_NCDESTROY:
        {
            // Remove own subclass when contrl is destroyed
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_PANEL)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
        } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT WindowProc_ListViewSpecial(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code) {

            case NM_CUSTOMDRAW:
            {
                HWND hWndDlg = ((LPNMHDR)lParam)->hwndFrom;
                int DlgCtrlID = GetDlgCtrlID(hWndDlg);
                if (hWndDlg == hWnd_ActiveFitting_SysListView32 && DlgCtrlID != 0) {

                    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

                    switch (lplvcd->nmcd.dwDrawStage) {

                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;

                        case CDDS_ITEMPREPAINT:
                            return CDRF_NOTIFYSUBITEMDRAW;

                        case (CDDS_SUBITEM | CDDS_ITEMPREPAINT):
                        {
                            // Assume default font color is white, and only change it to black based on background contrasting color
                            // Reason for only updating font color to black, is to allow "other" non-controlled subclasses to do their drawing without consuming a specific notification msg
                            //  - Example: "Color fitting view by slot" draws different row background colors, that we cant directly control
                            if (ListView_GetItemCount(hWndDlg) > 0 && GetContrastColorBW(lplvcd->clrTextBk) == Configuration.Palette->black) {
                                lplvcd->clrText = Configuration.Palette->black;
                                return CDRF_NEWFONT;
                            }
                        } break;

                    }

                }
            } break;
            }
        } break;
        case WM_DESTROY:
        case WM_NCDESTROY:
        {
            // Remove own subclass when contrl is destroyed
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_LISTVIEW_SPECIAL)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
        } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT WindowProc_ListView(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
        case WM_DESTROY:
        case WM_NCDESTROY:
        {
            // Remove own subclass when contrl is destroyed
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_LISTVIEW)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_LISTVIEW_HEADER)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
        } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WindowProc_Static(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    
    
    if (!NotChildOfWindow(hWnd, L"#32770", NULL)) {
        switch (uMsg) {
            case WM_CTLCOLOREDIT:
            {
                return (LRESULT)GetStockObject(DC_BRUSH);
            } break;
        }
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }

    switch (uMsg) {
        case WM_CTLCOLORDLG:
        {

        } break;

        case WM_CTLCOLOREDIT:
        {
            HWND hwndCtrl = (HWND)lParam;

            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->white);

            SetBkColor(hdc, Configuration.Palette->mid);                     // When has focus
            SetDCBrushColor(hdc, Configuration.Palette->mid);                // "normal" background color
            return (LRESULT)GetStockObject(DC_BRUSH);
        } break;

        case WM_CTLCOLORSTATIC:
        {
            HWND hwndCtrl = (HWND)lParam;

            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Configuration.Palette->white);
            SetBkMode(hdc, TRANSPARENT);

            return (INT_PTR)Configuration.Palette->dark_brush;
        } break;
        case WM_DESTROY:
        case WM_NCDESTROY:
        {
            // Remove own subclass when contrl is destroyed
            if (RemoveWindowSubclass(hWnd, SubclassProc, WSC_STATIC)) {
                //Log() << string_format("RemoveWindowsSubclass uninstalled: 0x%08X", hWnd);
            }
        } break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {

    switch (uIdSubclass)
    {
        case WSC_STATIC:
            return WindowProc_Static(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_LISTVIEW:
            return WindowProc_ListView(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_LISTVIEW_HEADER:
            return WindowProc_ListView(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_LISTVIEW_SPECIAL:
            return WindowProc_ListViewSpecial(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_PANEL:
            return WindowProc_Panel(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_DIALOG:
            return WindowProc_Dialog(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_CHECKBOX:
            return WindowProc_CheckBox(hWnd, uMsg, wParam, lParam);
            break;
        case WSC_DBG:
            return WindowProc_DBG(hWnd, uMsg, wParam, lParam);
            break;
    }
     
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void UpdateWndCtrls(HWND hWnd) {

    WCHAR winClassName[MAX_PATH * 2 + 1];
    WCHAR winTitle[MAX_PATH * 2 + 1];

    if (GetClassName(hWnd, winClassName, MAX_PATH) > 0) {
        GetWindowText(hWnd, winTitle, MAX_PATH);

        if (wcscmp(winClassName, L"SysListView32") == 0) {

            HWND hParent = GetParent(hWnd);
            HWND hParent2 = GetParent(hParent);
            HWND hParent3 = GetParent(hParent2);

            if ((GetWindowText(hParent, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"panel") == 0)) {
                if ((GetWindowText(hParent2, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"splitterWindow") == 0)) {
                    if ((GetWindowText(hParent3, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"splitterWindow") == 0)) {
                        if (NotChildOfWindow(hWnd, L"#32770", NULL)) {

                            hWnd_ActiveFitting_SysListView32 = hWnd;

                            if (!GetWindowSubclass(hParent, SubclassProc, WSC_LISTVIEW_SPECIAL, 0)) {
                                if (SetWindowSubclass(hParent, SubclassProc, WSC_LISTVIEW_SPECIAL, 0)) {
                                    //Log() << string_format("SetWindowsSubclass in LV_VIEW_DETAILS installed: 0x%08X", hParent);
                                }
                            }


                        }

                    }
                }
            }

        }
        // "SysHeader32"
        else if (wcscmp(winClassName, L"SysHeader32") == 0) {

            // Remove theme
            SetWindowTheme(hWnd, L"", L"");

            // Adds flat style to header window
            DWORD org_style = GetWindowLong(hWnd, GWL_STYLE);
            SetWindowLong(hWnd, GWL_STYLE, org_style | WS_BORDER | HDS_FLAT);

            HWND hParent = GetParent(hWnd);
            if (!GetWindowSubclass(hParent, SubclassProc, WSC_LISTVIEW, 0)) {
                if (SetWindowSubclass(hParent, SubclassProc, WSC_LISTVIEW, 0)) {
                    //Log() << string_format("SetWindowsSubclass in LV_VIEW_DETAILS installed: 0x%08X", hParent);
                }
            }

        }
        // "SysTreeView32"
        else if (wcscmp(winClassName, L"SysTreeView32") == 0) {

        }
        // "Button"
        else if (wcscmp(winClassName, L"Button") == 0) {
            //https://stackoverflow.com/questions/7293212/getwindowlong-to-check-button-style
            DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
            if ((dwStyle & BS_TYPEMASK) == BS_PUSHBUTTON) {

            }
            else
            if ((dwStyle & BS_TYPEMASK) == BS_RADIOBUTTON) {
                // Remove theme
                SetWindowTheme(hWnd, L"", L"");

                if (!GetWindowSubclass(hWnd, SubclassProc, WSC_STATIC, 0)) {
                    if (SetWindowSubclass(hWnd, SubclassProc, WSC_STATIC, (DWORD_PTR)hWnd)) {
                        //Log() << string_format("SetWindowsSubclass in BS_RADIOBUTTON installed: 0x%08X", hWnd);
                    }
                }

                // Subclass parent window, to handle notifications?
                HWND hParent = GetParent(hWnd);
                SetWindowTheme(hWnd, L"", L"");
                if (!GetWindowSubclass(hParent, SubclassProc, WSC_STATIC, 0)) {
                    if (SetWindowSubclass(hParent, SubclassProc, WSC_STATIC, (DWORD_PTR)hWnd)) {
                        //Log() << string_format("SetWindowsSubclass in BS_RADIOBUTTON parent installed: 0x%08X", hParent);
                    }
                }
            }
            else
            if ((dwStyle & BS_TYPEMASK) == BS_CHECKBOX) {

                // Color fitting by view checkbox
                if (!NotChildOfWindow(hWnd, L"#32770", NULL)) {
                    HWND hParent = GetParent(hWnd);
                    if ((GetWindowText(hParent, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"panel") == 0)) {
                        HWND hPrevWin = GetWindow(hParent, GW_HWNDPREV);
                        if ((GetClassName(hPrevWin, winClassName, MAX_PATH) > 0) && (wcscmp(winClassName, L"SysListView32") == 0)) {
                            if (GetWindowZIndex(hWnd) == 14) {

                                hWnd_ColorFitBySlot_CheckBox = hWnd;
                                if (!GetWindowSubclass(hWnd, SubclassProc, WSC_CHECKBOX, 0)) {
                                    if (SetWindowSubclass(hWnd, SubclassProc, WSC_CHECKBOX, 0)) {
                                        //Log() << string_format("SetWindowsSubclass in BS_CHECKNOX (ColorByFittingBySlot) installed: 0x%08X", hWnd);
                                    }
                                }

                            }
                        }
                    }
                }

            }
            else
            if ((dwStyle & BS_TYPEMASK) == BS_AUTOCHECKBOX) {

            }
            else
            if ((dwStyle & BS_TYPEMASK) == BS_GROUPBOX) {

            }
            else
            {

            }
        }
        // "Edit"
        else if (wcscmp(winClassName, L"Edit") == 0) {
            
                SetWindowTheme(hWnd, L"", L"");

                HWND hParent = GetParent(hWnd);
                SetWindowTheme(hParent, L"", L"");

                if (!GetWindowSubclass(hParent, SubclassProc, WSC_STATIC, 0)) {
                    if (SetWindowSubclass(hParent, SubclassProc, WSC_STATIC, (DWORD_PTR)hParent)) {
                        //Log() << string_format("SetWindowsSubclass in BS_RADIOBUTTON installed: 0x%08X", hWnd);
                    }
                }

        }
        // "ComboBox"
        else if (wcscmp(winClassName, L"ComboBox") == 0) {

        }
        // "Static" - https://learn.microsoft.com/en-us/windows/win32/controls/static-control-styles
        else if (wcscmp(winClassName, L"Static") == 0) {

            // Handle the "Additions" Panel bar
            if (WithinWindowDepth(hWndMain, hWnd, 4)) {
                HWND hParent = GetParent(hWnd);
                HWND hParent2 = GetParent(hParent);
                HWND hParent3 = GetParent(hParent2);
                if ((GetWindowText(hParent2, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"panel") == 0)) {
                    if ((GetWindowText(hParent3, winTitle, MAX_PATH) > 0) && (wcscmp(winTitle, L"splitterWindow") == 0)) {

                        hAdditionsWindow = hParent3;

                        if (!GetWindowSubclass(hParent, SubclassProc, WSC_PANEL, 0)) {
                            if (SetWindowSubclass(hParent, SubclassProc, WSC_PANEL, 0)) {
                                //Log() << string_format("SetWindowsSubclass in WSC_PANEL installed: 0x%08X", hWnd);
                            }
                        }
                        if (!GetWindowSubclass(hParent2, SubclassProc, WSC_PANEL, 0)) {
                            if (SetWindowSubclass(hParent2, SubclassProc, WSC_PANEL, 0)) {
                                //Log() << string_format("SetWindowsSubclass in WSC_PANEL installed: 0x%08X", hWnd);
                            }
                        }
                        
                    }
                }
            }
        }
        // Check window text
        else if (GetWindowText(hWnd, winTitle, MAX_PATH) > 0) {
            // panel
            if ((wcscmp(winClassName, L"wxWindowNR") == 0) && (wcscmp(winTitle, L"panel") == 0)) {

                /*
                    This is a dodgy way to determine "correct" window
                    1. Title panel and class wxWindowNR
                    2. Must not have the extended style WS_EX_CONTROLPARENT
                    3. Must be within 3 deep of "main" window
                */
                DWORD dwStyleExt = GetWindowLong(hWnd, GWL_EXSTYLE);
                
                if (dwStyleExt != WS_EX_CONTROLPARENT && WithinWindowDepth(hWndMain, hWnd, 3)) {

                    // Remove theme
                    SetWindowTheme(hWnd, L"", L"");
                    // Adds WS_BORDER style
                    DWORD org_style = GetWindowLong(hWnd, GWL_STYLE);
                    SetWindowLong(hWnd, GWL_STYLE, org_style | WS_BORDER);
                }

            }
            // splitterWindow
            else if ((wcscmp(winClassName, L"wxWindowNR") == 0) && (wcscmp(winTitle, L"splitterWindow") == 0)) {

            }
        }
        else {

        }
    }
}

