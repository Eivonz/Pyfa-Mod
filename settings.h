#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Windows.h>
#include "resource.h"
#include "MouseEvents.h"

#include "mINI.h"


extern HMODULE hModuleDLL;

#define DWTOCOLORREF(X) (((X & 0x000000FF) << 16) | (X & 0x0000FF00) | ((X & 0x00FF0000) >> 16))


#define DARKMODE_SETTINGSMNU_BASEID 9090
#define DARKMODE_SETTINGSMENU_SETTINGSID DARKMODE_SETTINGSMNU_BASEID + 1
#define DARKMODE_SETTINGSMENU_UPDATES DARKMODE_SETTINGSMNU_BASEID + 2
#define DARKMODE_SETTINGSMENU_INFO DARKMODE_SETTINGSMNU_BASEID + 3

#define DARKMODE_SETTINGS_LB_CUSTOMTHEMEID 999


typedef struct {
	int ID;
	wchar_t Name[MAX_PATH];
	DWORD Colors[5];
} Theme;
RECT CustomThemeColorClickZones[5] = {};

extern Theme DlgColThemes[];
extern const size_t DlgColThemes_size;


const std::string ININame_section = "settings";
const std::string ININame_ActiveThemePalette = "ActiveThemePalette";
const std::string ININame_OverrideThemePalette = "OverrideThemePalette";
const std::string ININame_EnableCustomControls = "EnableCustomControls";
const std::string ININame_UseExperimentalDarkmode = "UseExperimentalDarkmode";

const mINI::INIFile file("oleacc.ini");
mINI::INIStructure ini;



class Settings
{
public:
	Settings() {
		IsMenuUpdated = false;
	};
	~Settings() {};

	static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	static void SetupSettingsMenu(HWND hMainWnd, HMENU hMenu, HMODULE hModuleDLL);
	static void DisplaySettingsDialog(HWND hWnd);
//	static void CheckForUpdates(HWND hWnd);

	static INT_PTR CALLBACK DlgProcSettings(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK SubclassProc_ListBox(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	static BOOL IsMenuUpdated;
	static HWND hDlgSettings;

private:

};

BOOL Settings::IsMenuUpdated;
HWND Settings::hDlgSettings = NULL;

MouseTrackEvents mouseTrack;


// Actions for added menu items
LRESULT CALLBACK Settings::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);
			switch (wmId) {
				case DARKMODE_SETTINGSMENU_SETTINGSID: {
					DisplaySettingsDialog(hWnd);
				} break;

			}
		} break;
		case WM_DESTROY:
		case WM_NCDESTROY:
		{
			if (RemoveWindowSubclass(hWnd, SubclassProc, DARKMODE_SETTINGSMNU_BASEID)) {
			}
		} break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void Settings::SetupSettingsMenu(HWND hMainWnd, HMENU hMenu, HMODULE hModuleDLL) {

	// Setup Menu item
	HMENU hSettingsMenu = CreatePopupMenu();
	if (hSettingsMenu == NULL) {
//		Log() << "Err: CreatePopupMenu: " << GetLastErrorMsg();
		return;
	}

//	InsertMenu(hSettingsMenu, 0, MF_SEPARATOR, 0, nullptr);
	InsertMenu(hSettingsMenu, 1, MF_STRING, DARKMODE_SETTINGSMENU_SETTINGSID, L"Settings");
//	InsertMenu(hSettingsMenu, 2, MF_STRING, DARKMODE_SETTINGSMENU_UPDATES, L"Check for updates");
//	InsertMenu(hSettingsMenu, 3, MF_SEPARATOR, 0, nullptr);
//	InsertMenu(hSettingsMenu, 4, MF_STRING, DARKMODE_SETTINGSMENU_INFO, L"Info");

	MENUITEMINFO mitem = {0};
	mitem.cbSize = sizeof(MENUITEMINFO);
	mitem.fMask = MIIM_STRING | MIIM_DATA | MIIM_SUBMENU;
	mitem.fType = MFT_STRING;
	mitem.dwTypeData = const_cast<wchar_t*>(L"[Darkmode]");
	mitem.hSubMenu = hSettingsMenu;
	if (!InsertMenuItem(hMenu, GetMenuItemCount(hMenu), true, &mitem)) {
//		Log() << "Err: InsertMenuItem: " << GetLastErrorMsg();
		return;
	}
	
	if (!GetWindowSubclass(hMainWnd, SubclassProc, DARKMODE_SETTINGSMNU_BASEID, 0)) {
		if (SetWindowSubclass(hMainWnd, SubclassProc, DARKMODE_SETTINGSMNU_BASEID, 0)) {

		}
	}

	Settings::IsMenuUpdated = true;
}


int GetSelectedThemeIndex() {
	HWND hThemeListBox = GetDlgItem(Settings::hDlgSettings, IDC_LIST1);
	return ListBox_GetCurSel(hThemeListBox);
}
int GetSelectedThemeID() {
	HWND hThemeListBox = GetDlgItem(Settings::hDlgSettings, IDC_LIST1);
	int selIndx = ListBox_GetCurSel(hThemeListBox);
	if (selIndx >= 0) {
		Theme* curTheme = (Theme*)ListBox_GetItemData(hThemeListBox, selIndx);
		return curTheme->ID;
	}
	return -1;
}

void ColorPickerDialog() {
	
	COLORREF cRef[16] = {
		RGB(0,     5,   5),
		RGB(0,    15,  55),
		RGB(0,    25, 155),
		RGB(0,    35, 255),
		RGB(10,    0,   5),
		RGB(10,   20,  55),
		RGB(10,   40, 155),
		RGB(10,   60, 255),
		RGB(100,   5,   5),
		RGB(100,  25,  55),
		RGB(100,  50, 155),
		RGB(100, 125, 255),
		RGB(200, 120,   5),
		RGB(200, 150,  55),
		RGB(200, 200, 155),
		RGB(200, 250, 255)
	};

	DWORD rgbCurrent = RGB(200, 250, 255);

	//
	CHOOSECOLOR ccColour = {};
	ccColour.lStructSize = sizeof(ccColour);
	ccColour.hwndOwner = Settings::hDlgSettings;
	ccColour.lpCustColors = (LPDWORD)cRef;
	ccColour.rgbResult = rgbCurrent;
	ccColour.Flags = 
		CC_FULLOPEN |		// Show "advanced" color picker
		CC_RGBINIT |		// Open dialog with rgbResult color selected
		CC_SOLIDCOLOR
	;
	if (ChooseColor(&ccColour) == TRUE)
	{
		rgbCurrent = ccColour.rgbResult;
		Log() << "Custom color 0: " << ccColour.lpCustColors[0];
	}

}

void LoadINIFile() {


	if (ini.has(ININame_section)) {				// check for section name
	}
	else {
		// Generate a default .ini file
		//ini[ININame_section];
		ini[ININame_section].set({
			{ININame_ActiveThemePalette, "1"},
			//{OverrideThemePalette, "#521ecc, #e68ca1, #1a2070, #a85294, #aaccb5"},
			{ININame_EnableCustomControls, "true"},
			{ININame_UseExperimentalDarkmode, "true"},
			});

	}
}

void SaveINIFile() {

	if (ini.has(ININame_section)) {				// check for section name
		auto& settings = ini[ININame_section];

		// Selected color theme
		if (settings.has(ININame_ActiveThemePalette)) {
			int ID = GetSelectedThemeID();
			if (ID != DARKMODE_SETTINGS_LB_CUSTOMTHEMEID) {
				settings[ININame_ActiveThemePalette] = std::to_string(ID);
			}
			else {
				//TODO: Handle custom theme
			}
		}

		//EnableCustomControls
		if (settings.has(ININame_EnableCustomControls)) {
			HWND hChkBx = GetDlgItem(Settings::hDlgSettings, IDC_CHECK_ENABLECUSTOMCONTROLS);
			if (Button_GetCheck(hChkBx) == BST_CHECKED) {
				settings[ININame_EnableCustomControls] = "true";
			} else {
				settings[ININame_EnableCustomControls] = "false";
			}
		}

		//UseExperimentalDarkmode
		if (settings.has(ININame_UseExperimentalDarkmode)) {
			HWND hChkBx = GetDlgItem(Settings::hDlgSettings, IDC_CHECK_USEEXPERIMENTALDARKMODE);
			if (Button_GetCheck(hChkBx) == BST_CHECKED) {
				settings[ININame_UseExperimentalDarkmode] = "true";
			} else {
				settings[ININame_UseExperimentalDarkmode] = "false";
			}
		}
	}

	// Ghetto method to determine if file is to be created or updated
	mINI::INIStructure tmp;
	bool fileExists = file.read(tmp);
	if (fileExists) {
		file.write(ini);
	}
	else {
		file.generate(ini, true);
	}

}

void INIAddOrUpdateKey(std::string key, std::string value) {
	if (ini.has(ININame_section)) {				// check for section name
	}
}

bool INIHasKey(std::string key) {
	if (ini.has(ININame_section)) {				// check for section name
		return ini[ININame_section].has(key);
	}
	return false;
}
bool INIGetBool(std::string key) {
	if (ini.has(ININame_section)) {				// check for section name
		auto& settings = ini[ININame_section];

		if (settings.has(key)) {
			auto str = settings.get(key);

			std::transform(str.begin(), str.end(), str.begin(), ::tolower);
			if (str == "true" || str == "yes" || str == "on" || str == "1")
				return true;
			else if (str == "false" || str == "no" || str == "off" || str == "0")
				return false;
		}
	}
	return false;
}
int INIGetInt(std::string key) {
	if (ini.has(ININame_section)) {				// check for section name
		auto& settings = ini[ININame_section];

		if (settings.has(key)) {
			auto str = settings.get(key);
			return std::stoi(str);
		}
	}
	return -1;
}
std::string INIGetStr(std::string key) {
	if (ini.has(ININame_section)) {				// check for section name
		auto& settings = ini[ININame_section];

		if (settings.has(key)) {
			auto str = settings.get(key);
			return str;
		}
	}
	return "";
}

void Settings::DisplaySettingsDialog(HWND hWnd) {

	// Make sure any available custom settings are loaded from  the ini file
	LoadINIFile();

	DialogBoxParam(hModuleDLL, MAKEINTRESOURCE(IDD_DIALOG_SETTINGS), hWnd, Settings::DlgProcSettings, 0);

}


int GetCustomThemeMouseIndex(HWND hWnd, POINT mpt) {

	POINT pt = {};
	GetCursorPos(&pt);

	int retVal = LBItemFromPt(hWnd, pt, false);
	if (retVal != LB_ERR) {
		Theme* curTheme = (Theme*)ListBox_GetItemData(hWnd, retVal);

		if (curTheme->ID == DARKMODE_SETTINGS_LB_CUSTOMTHEMEID) {

			for (int i = 0; i < ARRAYSIZE(CustomThemeColorClickZones) - 1; i++) {
				if (PtInRect(&CustomThemeColorClickZones[i], mpt)) {
					Log() << "Painting zone found";
					return i;
				}
			}

		}

	}
	return -1;
}

INT_PTR CALLBACK Settings::DlgProcSettings(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

	Settings::hDlgSettings = hwnd;

	PMEASUREITEMSTRUCT pmis;
	PDRAWITEMSTRUCT pdis;
	HDC hdcMem;
	HDC pDC;
	HBITMAP hbmp;
	TCHAR achBuffer[MAX_PATH];
	TEXTMETRIC tm;

	switch (Message) {
	case WM_INITDIALOG: {
		/* ------ Setup window to configuration ------- */

		// CHECKBOX: IDC_CHECK_ENABLECUSTOMCONTROLS
		if (INIHasKey(ININame_EnableCustomControls)) {
			HWND hChkBx = GetDlgItem(hwnd, IDC_CHECK_ENABLECUSTOMCONTROLS);
			if (hChkBx != NULL) {
				Button_SetCheck(hChkBx, INIGetBool(ININame_EnableCustomControls) ? BST_CHECKED : BST_UNCHECKED);
			}
		}
		// CHECKBOX: IDC_CHECK_USEEXPERIMENTALDARKMODE
		if (INIHasKey(ININame_UseExperimentalDarkmode)) {
			HWND hChkBx = GetDlgItem(hwnd, IDC_CHECK_USEEXPERIMENTALDARKMODE);
			if (hChkBx != NULL) {
				Button_SetCheck(hChkBx, INIGetBool(ININame_UseExperimentalDarkmode) ? BST_CHECKED : BST_UNCHECKED);
			}
		}


		// LISTBOX
		// Get handle, and subclass it
		HWND hListBox = GetDlgItem(hwnd, IDC_LIST1);
		if (!GetWindowSubclass(hListBox, Settings::SubclassProc_ListBox, IDC_LIST1, 0)) {
			SetWindowSubclass(hListBox, Settings::SubclassProc_ListBox, IDC_LIST1, 0);
		}

		HWND hListBoxParent = GetParent(hwnd);
		if (!GetWindowSubclass(hListBoxParent, Settings::SubclassProc_ListBox, IDC_LIST1, 0)) {
			SetWindowSubclass(hListBoxParent, Settings::SubclassProc_ListBox, IDC_LIST1, 0);
		}

		// Populate it with items
		for (int i = 0; i < DlgColThemes_size; i++) {
			ListBox_AddItemData(hListBox, &DlgColThemes[i]);
		}
		// Set selected element
		if (!INIHasKey(ININame_OverrideThemePalette)) {			// if no alternative override palette is present in the ini file, use ActiveThemePalette
			if (INIHasKey(ININame_ActiveThemePalette)) {			// not -1 or 0
				int selIndx = INIGetInt(ININame_ActiveThemePalette);
				selIndx -= 1;
				ListBox_SetCurSel(hListBox, selIndx);
			}
		} else {
			// TODO:
			std::string customColors = INIGetStr(ININame_OverrideThemePalette);
		}

		return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHECK_ENABLECUSTOMCONTROLS: {
		} break;
		case IDC_CHECK_USEEXPERIMENTALDARKMODE: {
		} break;


		// ListboxCtrl
		case IDC_LIST1: {
			switch (HIWORD(wParam)) {
				// Handle change of listitem	
				case LBN_SELCHANGE: {
					
				} break;
				case LBN_DBLCLK: {
					if (GetSelectedThemeID() == DARKMODE_SETTINGS_LB_CUSTOMTHEMEID) {
						ColorPickerDialog();
					}
				} break;
			}
		} break;

		case IDOK: {
			SaveINIFile();
			EndDialog(hwnd, IDOK);
			} break;
		case IDCANCEL: {
			EndDialog(hwnd, IDCANCEL);
			} break;
		}
		break;

	case WM_MEASUREITEM:
		switch (LOWORD(wParam)) {
			case IDC_LIST1: {
				pmis = (PMEASUREITEMSTRUCT)lParam;

				// Set the height of the list box items. 
				pmis->itemHeight = 30;	// we should probably calculate this instead eg. height of parent box / # of themes available
			}
		}
		break;
	case WM_DRAWITEM:
		switch (LOWORD(wParam)) {
			case IDC_LIST1: {

				pdis = (PDRAWITEMSTRUCT)lParam;

				// If there are no list box items, skip this message. 
				if (pdis->itemID == -1) {
					RECT cr;
					GetClientRect(hwnd, &cr);
					FillRect(pdis->hDC, &cr, GetSysColorBrush(COLOR_BACKGROUND));
					break;
				}
				if (pdis->itemAction == ODA_FOCUS) {
					break;
				}
				if (pdis->itemAction == ODA_SELECT) {

				}
				if (pdis->itemAction == ODS_SELECTED) {

				}

				if (pdis->itemAction == ODS_HOTLIGHT) {

				}

				if (pdis->itemState & ODS_SELECTED) {

				}

				// Create a compatible device context. 
				pDC = CreateCompatibleDC(pdis->hDC);


				// Get Item custom data
				Theme* curTheme = (Theme*)pdis->itemData;

				// Draw radio button
				int selChange = pdis->itemAction & ODA_SELECT;
				int focusChange = pdis->itemAction & ODA_FOCUS;
				int drawEntire = pdis->itemAction & ODA_DRAWENTIRE;
				BOOL sel = pdis->itemState & ODS_SELECTED;

				DWORD style = GetWindowLong(hwnd, GWL_EXSTYLE);
				FillRect(
					pdis->hDC,
					&pdis->rcItem,
					GetSysColorBrush((style& WS_EX_TRANSPARENT) ? COLOR_BTNFACE : COLOR_WINDOW)
				);

				int radioSize = 24;
				int topOfs = ((pdis->rcItem.bottom - pdis->rcItem.top) / 2) + pdis->rcItem.top - radioSize / 2;
				int botOfs = topOfs + radioSize;
				RECT radioBox = {	// left, top, right, bottom
					pdis->rcItem.left + 2,
					topOfs + 2,
					pdis->rcItem.left + 2 + radioSize,
					botOfs - 3,
				};
				DrawFrameControl(pdis->hDC, &radioBox, DFC_BUTTON, DFCS_BUTTONRADIO | (sel ? DFCS_CHECKED : 0));

				// Get the metrics for the current font.
				GetTextMetrics(pdis->hDC, &tm);
				// Calculate the vertical position for the item string 
				// so that the string will be vertically centered in the 
				// item rectangle.
				int yPos = (pdis->rcItem.bottom + pdis->rcItem.top - tm.tmHeight) / 2;
				// Draw text
				TextOut(pdis->hDC, 5+20, yPos - 2, curTheme->Name, wcslen(curTheme->Name));

				int cBoxLeftOffset = 100;
				int cBoxWidth = ceil(abs(pdis->rcItem.right - pdis->rcItem.left - cBoxLeftOffset) / ARRAYSIZE(curTheme->Colors));					//int cBoxWidth = 55;
				int cBoxHeight = abs(pdis->rcItem.bottom - pdis->rcItem.top);

				// Draw a rectangle for each of the colors in a given theme
				for (int i = 0; i < ARRAYSIZE(curTheme->Colors); i++) {

					HBRUSH hbr = CreateSolidBrush(DWTOCOLORREF(curTheme->Colors[i]));

					RECT cBox = {	// left, top, right, bottom
						pdis->rcItem.left + cBoxLeftOffset + (i * cBoxWidth),
						pdis->rcItem.top,
						pdis->rcItem.left + cBoxLeftOffset + (i * cBoxWidth) + cBoxWidth,
						pdis->rcItem.bottom,
					};


					FillRect(pdis->hDC, &cBox, hbr);
					DeleteObject(hbr);

					if (curTheme->ID == DARKMODE_SETTINGS_LB_CUSTOMTHEMEID) {
						HBRUSH dbghbr = CreateSolidBrush(RGB(255, 0, 0));
						FrameRect(pdis->hDC, &cBox, dbghbr);
						DeleteObject(dbghbr);
					}


					if (curTheme->ID == DARKMODE_SETTINGS_LB_CUSTOMTHEMEID) {
						RECT cRc;
						GetClientRect(hwnd, &cRc);
						
						MapWindowPoints(pdis->hwndItem, hwnd, (LPPOINT)&cBox, 2);
						CustomThemeColorClickZones[i] = cBox;
					}
				}

				switch (pdis->itemAction) {
					case ODA_SELECT: {

					}
				}


				break;


			}
		}
		break;
	case WM_CTLCOLOR: {
		DWORD style = GetWindowLong(hwnd, GWL_EXSTYLE);

		return NULL;
	} break;

	case WM_DESTROY:

		break;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK Settings::SubclassProc_ListBox(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (uMsg) {
	case WM_CTLCOLOR: {

	} break;

	case WM_MOUSEMOVE: {
/*
		// lParam The (x,y) coordinates of the mouse cursor is relative to the window/dialog.
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		int clickZone = GetCustomThemeMouseIndex(hWnd, pt);
		if (clickZone != -1) {
			Log() << string_format("OutLining a click zone: %d", clickZone);

			HDC hdc = GetDC(hWnd);
			HBRUSH bg = CreateSolidBrush(RGB(250, 0, 0));
			
			FrameRect(hdc, &CustomThemeColorClickZones[clickZone], bg);
			
			DeleteObject(bg);
			ReleaseDC(hWnd, hdc);
		}
*/
		mouseTrack.OnMouseMove(hWnd);
	} break;

	case WM_MOUSEHOVER: {									// TrackMouseEvent message
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		mouseTrack.Reset(hWnd);
	} break;
	case WM_MOUSELEAVE: {									// TrackMouseEvent message

		RECT rc = {};
		GetClientRect(hWnd, &rc);
		InvalidateRect(hWnd, &rc, false);
		UpdateWindow(hWnd);

		mouseTrack.Reset(hWnd);
	} break;
		
			
	case WM_DESTROY:
	case WM_NCDESTROY:
	{
		if (RemoveWindowSubclass(hWnd, SubclassProc, NULL)) {

		}
	} break;
	}
	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


#endif
